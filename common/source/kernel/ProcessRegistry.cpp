#include <mm/KernelMemoryManager.h>
#include "ProcessRegistry.h"
#include "Scheduler.h"
#include "UserProcess.h"
#include "kprintf.h"
#include "VfsSyscall.h"
#include "VirtualFileSystem.h"
#include "PageManager.h"
#include <uset.h>
#include <ustl/umap.h>
#include <ulist.h>


ProcessRegistry* ProcessRegistry::instance_ = 0;

ProcessRegistry::ProcessRegistry(FileSystemInfo *root_fs_info, char const *progs[]) :
    Thread(root_fs_info, "ProcessRegistry", Thread::KERNEL_THREAD),process_lock_("ProcessRegistry::process_lock_"), progs_(progs),
    progs_running_(0),
    counter_lock_("ProcessRegistry::counter_lock_"),
    all_processes_killed_(&counter_lock_, "ProcessRegistry::all_processes_killed_")
{
  instance_ = this; // instance_ is static! -> Singleton-like behaviour
}

ProcessRegistry::~ProcessRegistry()
{
}

ProcessRegistry* ProcessRegistry::instance()
{
  return instance_;
}

void ProcessRegistry::Run()
{
  if (!progs_ || !progs_[0])
    return;

  debug(PROCESS_REG, "mounting userprog-partition \n");

  debug(PROCESS_REG, "mkdir /usr\n");
  assert( !VfsSyscall::mkdir("/usr", 0) );
  debug(PROCESS_REG, "mount idea1\n");
  assert( !VfsSyscall::mount("idea1", "/usr", "minixfs", 0) );

  debug(PROCESS_REG, "mkdir /dev\n");
  assert( !VfsSyscall::mkdir("/dev", 0) );
  debug(PROCESS_REG, "mount devicefs\n");
  assert( !VfsSyscall::mount(NULL, "/dev", "devicefs", 0) );


  KernelMemoryManager::instance()->startTracing();

  for (uint32 i = 0; progs_[i]; i++)
  {
    createProcess(progs_[i]);
  }

  counter_lock_.acquire();

  while (progs_running_)
    all_processes_killed_.wait();

  counter_lock_.release();

  debug(PROCESS_REG, "unmounting userprog-partition because all processes terminated \n");

  VfsSyscall::umount("/usr", 0);
  VfsSyscall::umount("/dev", 0);
  vfs.rootUmount();

  Scheduler::instance()->printStackTraces();
  Scheduler::instance()->printThreadList();

  PageManager* pm = PageManager::instance();
  if(!DYNAMIC_KMM && pm->getNumFreePages() != pm->getNumPagesForUser())
  {
    PageManager::instance()->printBitmap();
    debug(PM, "WARNING: You might be leaking physical memory pages somewhere\n");
    debug(PM, "%u/%u free physical pages after unmounting detected\n",
          pm->getNumFreePages(),
          pm->getNumPagesForUser());
  }

  kill();
}

void ProcessRegistry::processExit()
{
  counter_lock_.acquire();
  if (--progs_running_ == 0)
    all_processes_killed_.signal();

  counter_lock_.release();
}

void ProcessRegistry::processStart()
{
  counter_lock_.acquire();
  ++progs_running_;
  counter_lock_.release();
}

size_t ProcessRegistry::processCount()
{
  ScopeLock lock(counter_lock_);
  return progs_running_;
}


void ProcessRegistry::createProcess(const char* path) {
    debug(PROCESS_REG, "create process %s\n", path);
    UserProcess *process = new UserProcess(path, new FileSystemInfo(*working_dir_));
    if (process) {
        debug(PROCESS_REG, "create process %s\n", path);
        process_count_++;
    }

}

size_t ProcessRegistry::fork()
{
    if(progs_running_ == 0)
        return -1;

    process_lock_.acquire();
    UserThread *current_thread = (UserThread*)currentThread;
    UserProcess *current_process = current_thread->getProcess();
    debug(PROCESS_REG, "TID= %ld, PID= %ld\n", current_thread->tid_, current_process->pid_);

    process_lock_.release();
    process_count_++;
    size_t pid = process_count_;
    UserProcess* new_process = new UserProcess(*current_process, *current_thread, pid);
    process_lock_.acquire();

    process_map_.push_back(ustl::make_pair(pid, current_process));

    if(new_process == nullptr)
    {
        process_lock_.release();
        return -1;
    }

    process_lock_.release();

    return pid;
}

void ProcessRegistry::updateExitCode(size_t code)
{
    process_lock_.acquire();
    UserThread *current_thread = (UserThread*)currentThread;
    UserProcess *current_process = current_thread->getProcess();
    process_retval_map_.push_back(ustl::make_pair(current_process->pid_, code));
    current_process->threads_lock_.acquire();
    for( auto it : current_process->threads_map_)
        if( it.first != current_thread->tid_)
           it.second->makeAsynchronousCancel();
    current_process->threads_lock_.release();
    process_lock_.release();
}