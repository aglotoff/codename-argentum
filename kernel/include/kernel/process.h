#ifndef __KERNEL_INCLUDE_KERNEL_PROCESS_H__
#define __KERNEL_INCLUDE_KERNEL_PROCESS_H__

#ifndef __ARGENTUM_KERNEL__
#error "This is a kernel header; user programs should not #include it"
#endif

#include <limits.h>
#include <signal.h>
#include <sys/times.h>
#include <sys/types.h>

#include <kernel/cpu.h>
#include <kernel/spinlock.h>
#include <kernel/list.h>
#include <kernel/vm.h>
#include <kernel/thread.h>
#include <kernel/trap.h>
#include <kernel/waitqueue.h>
#include <time.h>

struct File;
struct KSpinLock;
struct Inode;
struct Process;
struct PathNode;
struct Signal;

struct FileDesc {
  struct File *file;
  int          flags;
};

/**
 * Process descriptor.
 */
struct Process {
  struct KListLink       link;
  /** The process' address space */
  struct VMSpace        *vm;

  /** Main process thread */
  struct KThread        *thread;

  /** Unique thread identifier */
  pid_t                 pid;
  /** Link into the PID hash table */
  struct KListLink      pid_link;
  /** Process group ID */
  pid_t                 pgid;

  /** The parent process */
  struct Process       *parent;
  /** List of child processes */
  struct KListLink      children;
  /** Link into the siblings list */
  struct KListLink      sibling_link;
  struct tms            times;
  char                  name[64];

  /** Queue to sleep waiting for children */
  struct KWaitQueue     wait_queue;
  /** Whether the process is a zombie */
  int                   state;
  /** Exit code */
  int                   status;
  int                   flags;

  uintptr_t             signal_stub;
  struct sigaction      signal_actions[NSIG];
  struct Signal        *signal_pending[NSIG];
  struct KListLink      signal_queue;
  sigset_t              signal_mask;

  /** Real user ID */
  uid_t                 ruid;
  /** Effective user ID */
  uid_t                 euid;
  /** Real group ID */
  gid_t                 rgid;
  /** Effective group ID */
  gid_t                 egid;
  /** File mode creation mask */
  mode_t                cmask;

  /** Current working directory */
  struct PathNode      *cwd;

  /** Open file descriptors */
  struct FileDesc       fd[OPEN_MAX];
  /** Lock protecting the file descriptors */
  struct KSpinLock      fd_lock;
};

enum {
  PROCESS_STATE_NONE = 0,
  PROCESS_STATE_ACTIVE = 1,
  PROCESS_STATE_ZOMBIE = 2,
  PROCESS_STATE_STOPPED = 3,
};

enum {
  PROCESS_STATUS_AVAILABLE = (1 << 0),
};

extern struct KSpinLock __process_lock;
extern struct KListLink __process_list;

struct Signal {
  struct KListLink link;
  siginfo_t        info;
};

static inline struct Process *
process_current(void)
{
  struct KThread *task = k_thread_current();
  return task != NULL ? task->process : NULL;
}

static inline void
process_lock(void)
{
  k_spinlock_acquire(&__process_lock);
}

static inline void
process_unlock(void)
{
  k_spinlock_release(&__process_lock);
}

void           signal_init_system(void);
void           process_init(void);
int            process_create(const void *, struct Process **);
void           process_destroy(int);
void           process_free(struct Process *);
pid_t          process_copy(int);
pid_t          process_wait(pid_t, int *, int);
int            process_exec(const char *, char *const[], char *const[]);
void          *process_grow(ptrdiff_t);
void           arch_trap_frame_init(struct Process *, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
void           process_update_times(struct Process *, clock_t, clock_t);
void           process_get_times(struct Process *, struct tms *);

void           signal_init(struct Process *);
int            signal_generate(pid_t, int, int);
void           signal_clone(struct Process *, struct Process *);
void           signal_deliver_pending(void);
int            signal_action(int, uintptr_t, struct sigaction *, struct sigaction *);
int            signal_return(void);
int            signal_pending(sigset_t *);
int            signal_mask(int, const sigset_t *, sigset_t *);
int            signal_suspend(const sigset_t *);

int            process_nanosleep(const struct timespec *, struct timespec *);
pid_t          process_get_gid(pid_t);
int            process_set_gid(pid_t, pid_t);
int            process_match_pid(struct Process *, pid_t);

#endif  // __KERNEL_INCLUDE_KERNEL_PROCESS_H__
