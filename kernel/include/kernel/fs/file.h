#ifndef __KERNEL_INCLUDE_KERNEL_FS_FILE_H__
#define __KERNEL_INCLUDE_KERNEL_FS_FILE_H__

#ifndef __ARGENTUM_KERNEL__
#error "This is a kernel header; user programs should not #include it"
#endif

#include <sys/types.h>

struct Inode;
struct stat;
struct Pipe;

#define FD_INODE    0
#define FD_PIPE     1
#define FD_SOCKET   2

struct File {
  int              type;         ///< File type (inode, console, or pipe)
  int              ref_count;    ///< The number of references to this file
  int              flags;
  off_t            offset;       ///< Current offset within the file
  struct PathNode *node;        ///< Pointer to the corresponding inode
  int              socket;       ///< Socket ID
  struct Pipe     *pipe;         ///< Pointer to the correspondig pipe
};

int          file_alloc(struct File **);
void         file_init(void);
int          file_open(const char *, int, mode_t, struct File **);
struct File *file_dup(struct File *);
void         file_close(struct File *);
ssize_t      file_read(struct File *, void *, size_t);
ssize_t      file_write(struct File *, const void *, size_t);
ssize_t      file_getdents(struct File *, void *, size_t);
int          file_stat(struct File *, struct stat *);
int          file_chdir(struct File *);
off_t        file_seek(struct File *, off_t, int);
int          file_get_flags(struct File *);
int          file_set_flags(struct File *, int);
int          file_chmod(struct File *, mode_t);
int          file_ioctl(struct File *, int, int);

#endif  // !__KERNEL_INCLUDE_KERNEL_FS_FILE__
