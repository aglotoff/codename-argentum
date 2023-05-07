#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

#include <argentum/cprintf.h>
#include <argentum/drivers/console.h>
#include <argentum/drivers/rtc.h>
#include <argentum/fs/buf.h>
#include <argentum/fs/fs.h>
#include <argentum/process.h>

#include "ext2.h"

static struct {
  struct Inode    buf[INODE_CACHE_SIZE];
  struct SpinLock lock;
  struct ListLink head;
} inode_cache;

void
fs_inode_cache_init(void)
{
  struct Inode *ip;
  
  spin_init(&inode_cache.lock, "inode_cache");
  list_init(&inode_cache.head);

  for (ip = inode_cache.buf; ip < &inode_cache.buf[INODE_CACHE_SIZE]; ip++) {
    kmutex_init(&ip->mutex, "inode");
    list_add_back(&inode_cache.head, &ip->cache_link);
  }
}

struct Inode *
fs_inode_get(ino_t ino, dev_t dev)
{
  struct ListLink *l;
  struct Inode *ip, *empty;

  spin_lock(&inode_cache.lock);

  empty = NULL;
  LIST_FOREACH(&inode_cache.head, l) {
    ip = LIST_CONTAINER(l, struct Inode, cache_link);
    if ((ip->ino == ino) && (ip->dev == dev)) {
      ip->ref_count++;
      spin_unlock(&inode_cache.lock);

      return ip;
    }

    if (ip->ref_count == 0)
      empty = ip;
  }

  if (empty != NULL) {
    empty->ref_count = 1;
    empty->ino       = ino;
    empty->dev       = dev;
    empty->flags     = 0;

    spin_unlock(&inode_cache.lock);

    return empty;
  }

  spin_unlock(&inode_cache.lock);

  return NULL;
}

/**
 * Increment the reference counter of the given inode.
 * 
 * @param inode Pointer to the inode
 * 
 * @return Pointer to the inode.
 */
struct Inode *
fs_inode_duplicate(struct Inode *inode)
{
  spin_lock(&inode_cache.lock);
  inode->ref_count++;
  spin_unlock(&inode_cache.lock);

  return inode;
}

/**
 * Release pointer to an inode.
 * 
 * @param inode Pointer to the inode to be released
 */
void
fs_inode_put(struct Inode *inode)
{   
  kmutex_lock(&inode->mutex);

  if (inode->flags & FS_INODE_DIRTY)
    panic("inode dirty");

  // If the link count reaches zero, delete inode from the filesystem before
  // returning it to the cache
  if ((inode->flags & FS_INODE_VALID) && (inode->nlink == 0)) {
    int ref_count;

    spin_lock(&inode_cache.lock);
    ref_count = inode->ref_count;
    spin_unlock(&inode_cache.lock);

    // If this is the last reference to this inode
    if (ref_count == 1) {
      ext2_delete_inode(inode);
      inode->flags &= ~FS_INODE_VALID;
    }
  }

  kmutex_unlock(&inode->mutex);

  // Return the inode to the cache
  spin_lock(&inode_cache.lock);
  if (--inode->ref_count == 0) {
    list_remove(&inode->cache_link);
    list_add_front(&inode_cache.head, &inode->cache_link);
  }
  spin_unlock(&inode_cache.lock);
}

int
fs_inode_can_read(struct Inode *inode)
{
  struct Process *my_process = process_current();

  if (my_process->uid == 0)
    return 1;
  if ((my_process->uid == inode->uid) && (inode->mode & S_IRUSR))
    return 1;
  if ((my_process->gid == inode->gid) && (inode->mode & S_IRGRP))
    return 1;
  return inode->mode & S_IROTH;
}

int
fs_inode_can_write(struct Inode *inode)
{
  struct Process *my_process = process_current();

  if (my_process->uid == 0)
    return 1;
  if ((my_process->uid == inode->uid) && (inode->mode & S_IWUSR))
    return 1;
  if ((my_process->gid == inode->gid) && (inode->mode & S_IWGRP))
    return 1;
  return inode->mode & S_IWOTH;
}

int
fs_inode_can_execute(struct Inode *inode)
{
  struct Process *my_process = process_current();

  if (my_process->uid == 0)
    return inode->mode & (S_IXUSR | S_IXGRP | S_IXOTH);
  if ((my_process->uid == inode->uid) && (inode->mode & S_IXUSR))
    return 1;
  if ((my_process->gid == inode->gid) && (inode->mode & S_IXGRP))
    return 1;
  return inode->mode & S_IXOTH;
}

static int
fs_inode_holding(struct Inode *ip)
{
  return kmutex_holding(&ip->mutex);
}

/**
 * Lock the given inode. Read the inode meta info, if necessary.
 * 
 * @param ip Pointer to the inode.
 */
void
fs_inode_lock(struct Inode *ip)
{
  kmutex_lock(&ip->mutex);

  if (ip->flags & FS_INODE_VALID)
    return;

  if (ip->flags & FS_INODE_DIRTY)
    panic("inode dirty");

  ext2_read_inode(ip);

  ip->flags |= FS_INODE_VALID;
}

void
fs_inode_unlock(struct Inode *ip)
{  
  if (!(ip->flags & FS_INODE_VALID))
    panic("inode not valid");

  if (ip->flags & FS_INODE_DIRTY) {
    ext2_write_inode(ip);
    ip->flags &= ~FS_INODE_DIRTY;
  }

  kmutex_unlock(&ip->mutex);
}

/**
 * Common pattern: unlock inode and then put.
 *
 * @param ip Pointer to the inode.
 */
void
fs_inode_unlock_put(struct Inode *ip)
{  
  fs_inode_unlock(ip);
  fs_inode_put(ip);
}

ssize_t
fs_inode_read(struct Inode *ip, void *buf, size_t nbyte, off_t *off)
{
  ssize_t ret;
  
  if (!fs_inode_holding(ip))
    panic("not locked");

  if (!fs_inode_can_read(ip))
    return -EPERM;

  // Read from the corresponding device
  if (S_ISCHR(ip->mode) || S_ISBLK(ip->mode)) {
    fs_inode_unlock(ip);

    // TODO: support other devices
    ret = console_read(buf, nbyte);

    fs_inode_lock(ip);
    return ret;
  }

  if ((*off + nbyte) < *off)
    return -EINVAL;

  if ((ret = ext2_inode_read(ip, buf, nbyte, *off)) < 0)
    return ret;

  *off += ret;

  return ret;
}

ssize_t
fs_inode_write(struct Inode *ip, const void *buf, size_t nbyte, off_t *off)
{
  ssize_t total;

  if (!fs_inode_holding(ip))
    panic("not locked");

  if (!fs_inode_can_write(ip))
    return -EPERM;

  // Write to the corresponding device
  if (S_ISCHR(ip->mode) || S_ISBLK(ip->mode)) {
    fs_inode_unlock(ip);

    // TODO: support other devices
    total = console_write(buf, nbyte);

    fs_inode_lock(ip);
    return total;
  }

  if ((*off + nbyte) < *off)
    return -EINVAL;

  total = ext2_inode_write(ip, buf, nbyte, *off);

  if (total > 0)
    *off += total;

  return total;
}

static int
fs_filldir(void *buf, const char *name, uint16_t name_len, off_t off,
           ino_t ino, uint8_t type)
{
  struct dirent *dp = (struct dirent *) buf;

  dp->d_reclen  = name_len + offsetof(struct dirent, d_name);
  dp->d_ino     = ino;
  dp->d_off     = off;
  dp->d_namelen = name_len;
  dp->d_type    = type;
  memmove(&dp->d_name[0], name, name_len);

  return dp->d_reclen;
}

ssize_t
fs_inode_read_dir(struct Inode *ip, void *buf, size_t nbyte, off_t *off)
{
  char *dst = (char *) buf;
  ssize_t total = 0;

  if (!fs_inode_holding(ip))
    panic("not locked");

  if (!fs_inode_can_read(ip))
    return -EPERM;

  while (nbyte > 0) {
    struct dirent *de = (struct dirent *) dst;
    ssize_t nread;

    if ((nread = ext2_readdir(ip, de, fs_filldir, *off)) < 0)
      return nread;

    if (nread == 0)
      break;
    
    if (de->d_reclen > nbyte) {
      if (total == 0) {
        return -EINVAL;
      }
      break;
    }

    *off += nread;

    dst   += de->d_reclen;
    total += de->d_reclen;
    nbyte -= de->d_reclen;
  }

  return total;
}

int
fs_inode_stat(struct Inode *ip, struct stat *buf)
{
  if (!fs_inode_holding(ip))
    panic("not locked");

  // TODO: check permissions

  buf->st_mode  = ip->mode;
  buf->st_ino   = ip->ino;
  buf->st_dev   = ip->dev;
  buf->st_nlink = ip->nlink;
  buf->st_uid   = ip->uid;
  buf->st_gid   = ip->gid;
  buf->st_size  = ip->size;
  buf->st_atime = ip->atime;
  buf->st_mtime = ip->mtime;
  buf->st_ctime = ip->ctime;

  return 0;
}

int
fs_inode_truncate(struct Inode *inode)
{
  if (!fs_inode_holding(inode))
    panic("not locked");

  if (!fs_inode_can_write(inode))
    return -EPERM;

  ext2_inode_trunc(inode);

  return 0;
}

int
fs_inode_create(struct Inode *dir, char *name, mode_t mode, dev_t dev,
                struct Inode **istore)
{
  if (!fs_inode_holding(dir))
    panic("directory not locked");
  
  if (!S_ISDIR(dir->mode))
    return -ENOTDIR;
  if (!fs_inode_can_write(dir))
    return -EPERM;

  if (ext2_inode_lookup(dir, name) != NULL)
    return -EEXISTS;

  switch (mode & S_IFMT) {
  case S_IFDIR:
    return ext2_inode_mkdir(dir, name, mode, istore);
  case S_IFREG:
    return ext2_inode_create(dir, name, mode, istore);
  default:
    return ext2_inode_mknod(dir, name, mode, dev, istore);
  }
}

int
fs_inode_link(struct Inode *inode, struct Inode *dir, char *name)
{
  if (!fs_inode_holding(inode))
    panic("inode not locked");
  if (!fs_inode_holding(dir))
    panic("directory not locked");
  
  if (!S_ISDIR(dir->mode))
    return -ENOTDIR;
  if (!fs_inode_can_write(dir))
    return -EPERM;
  
  // TODO: Allow links to directories?
  if (S_ISDIR(inode->mode))
    return -EPERM;
  if (inode->nlink >= LINK_MAX)
    return -EMLINK;

  if (dir->dev != inode->dev)
    return -EXDEV;

  return ext2_inode_link(dir, name, inode);
}

int
fs_inode_lookup(struct Inode *dir, const char *name, struct Inode **istore)
{
  struct Inode *inode;

  if (!fs_inode_holding(dir))
    panic("not locked");
  
  if (!S_ISDIR(dir->mode))
    return -ENOTDIR;
  if (!fs_inode_can_read(dir))
    return -EPERM;

  inode = ext2_inode_lookup(dir, name);

  if (istore != NULL)
    *istore = inode;
  else if (inode == NULL)
    fs_inode_put(inode);

  return 0;
}

int
fs_inode_unlink(struct Inode *dir, struct Inode *inode)
{
  if (!fs_inode_holding(inode))
    panic("inode not locked");
  if (!fs_inode_holding(dir))
    panic("directory not locked");
  
  if (!S_ISDIR(dir->mode))
    return -ENOTDIR;
  if (!fs_inode_can_write(dir))
    return -EPERM;

  // TODO: Allow links to directories?
  if (S_ISDIR(inode->mode))
    return -EPERM;

  return ext2_inode_unlink(dir, inode);
}

int
fs_inode_rmdir(struct Inode *dir, struct Inode *inode)
{
  if (!fs_inode_holding(inode))
    panic("inode not locked");
  if (!fs_inode_holding(dir))
    panic("directory not locked");
  
  if (!S_ISDIR(dir->mode))
    return -ENOTDIR;
  if (!fs_inode_can_write(dir))
    return -EPERM;

  // TODO: Allow links to directories?
  if (S_ISDIR(inode->mode))
    return -EPERM;

  return ext2_inode_rmdir(dir, inode);
}

int
fs_create(const char *path, mode_t mode, dev_t dev, struct Inode **istore)
{
  struct Inode *dir, *ip;
  char name[NAME_MAX + 1];
  int r;

  if ((r = fs_path_lookup(path, name, NULL, &dir)) < 0)
    return r;

  mode &= ~process_current()->cmask;

  fs_inode_lock(dir);

  if ((r = fs_inode_create(dir, name, mode, dev, &ip)) == 0) {
    if (istore == NULL) {
      fs_inode_unlock_put(ip);
    } else {
      *istore = ip;
    }
  }

  fs_inode_unlock_put(dir);
  return r;
}

static void
fs_inode_lock_two(struct Inode *inode1, struct Inode *inode2)
{
  if (inode1 < inode2) {
    fs_inode_lock(inode1);
    fs_inode_lock(inode2);
  } else {
    fs_inode_lock(inode2);
    fs_inode_lock(inode1);
  }
}

static void
fs_inode_unlock_two(struct Inode *inode1, struct Inode *inode2)
{
  if (inode1 < inode2) {
    fs_inode_unlock(inode2);
    fs_inode_unlock(inode1);
  } else {
    fs_inode_unlock(inode1);
    fs_inode_unlock(inode2);
  }
}

int
fs_link(char *path1, char *path2)
{
  struct Inode *ip, *dirp;
  char name[NAME_MAX + 1];
  int r;

  if ((r = fs_name_lookup(path1, &ip)) < 0)
    return r;

  if ((r = fs_path_lookup(path2, name, NULL, &dirp)) < 0)
    goto out1;

  // TODO: check for the same node?

  // Always lock inodes in a specific order to avoid deadlocks
  fs_inode_lock_two(dirp, ip);

  r = fs_inode_link(ip, dirp, name);

  fs_inode_unlock_two(dirp, ip);
  fs_inode_put(dirp);
out1:
  fs_inode_put(ip);
  return r;
}

int
fs_unlink(const char *path)
{
  struct Inode *dir, *ip;
  char name[NAME_MAX + 1];
  int r;

  if ((r = fs_path_lookup(path, name, &ip, &dir)) < 0)
    return r;

  if (ip == NULL) {
    fs_inode_put(dir);
    return -ENOENT;
  }

  fs_inode_lock_two(dir, ip);
  r = fs_inode_unlink(dir, ip);
  fs_inode_unlock_two(dir, ip);

  fs_inode_put(dir);
  fs_inode_put(ip);

  return r;
}

int
fs_rmdir(const char *path)
{
  struct Inode *dir, *ip;
  char name[NAME_MAX + 1];
  int r;

  if ((r = fs_path_lookup(path, name, &ip, &dir)) < 0)
    return r;

  if (ip == NULL) {
    fs_inode_put(dir);
    return -ENOENT;
  }

  fs_inode_lock_two(dir, ip);
  r = fs_inode_rmdir(dir, ip);
  fs_inode_unlock_two(dir, ip);

  fs_inode_put(dir);
  fs_inode_put(ip);

  return r;
}

int
fs_set_pwd(struct Inode *inode)
{
  struct Process *current = process_current();

  fs_inode_lock(inode);

  if (!S_ISDIR(inode->mode)) {
    fs_inode_unlock(inode);
    return -ENOTDIR;
  }
  if (!fs_inode_can_execute(inode)) {
    fs_inode_unlock(inode);
    return -EPERM;
  }

  fs_inode_unlock(inode);

  fs_inode_put(current->cwd);
  current->cwd = inode;

  return 0;
}

int
fs_chdir(const char *path)
{
  struct Inode *ip;
  int r;

  if ((r = fs_name_lookup(path, &ip)) < 0)
    return r;

  if ((r = fs_set_pwd(ip)) != 0)
    fs_inode_put(ip);

  return r;
}

int
fs_inode_chmod(struct Inode *ip, mode_t mode)
{
  struct Process *current = process_current();

  if (!fs_inode_holding(ip))
    panic("not holding");

  if ((current->uid != 0) && (ip->uid != current->uid))
    return -EPERM;

  // TODO: additional permission checks

  ip->mode  = mode;
  ip->ctime = rtc_get_time();
  ip->flags |= FS_INODE_DIRTY;

  return 0;
}

int
fs_chmod(const char *path, mode_t mode)
{
  struct Inode *ip;
  int r;

  if ((r = fs_name_lookup(path, &ip)) < 0)
    return r;
  
  fs_inode_lock(ip);

  r = fs_inode_chmod(ip, mode);

  fs_inode_unlock_put(ip);

  return r;
}



int
fs_permissions(struct Inode *inode, mode_t mode)
{
  struct Process *proc = process_current();

  if (proc->uid == inode->uid)
    mode <<= 6;
  else if (proc->gid == inode->gid)
    mode <<= 3;

  return (inode->mode & mode) == mode;
}
