//TODO: credits to Clockwork

#include "thread.h"

#include <sched.h>
#include <algorithm>
#include <sstream>
#include <thread>

#include "util.h"

namespace firehose {
namespace threading {

// The priority scheduler in use.  SCHED_FIFO or SCHED_RR
int scheduler = SCHED_FIFO;

/*
Globally manages assignment of threads to cores, since RT priority is fragile
*/
class CoreManager {
 public:
  const int init_pool_size = 2;
  const int default_pool_size = 2;

  std::vector<bool> in_use;
  std::vector<std::vector<unsigned>> gpu_affinity;

  std::vector<unsigned> init_pool;
  std::vector<unsigned> default_pool;

  CoreManager() : in_use(coreCount(), false) {
    in_use[0] = true;  // Don't use core 0
    in_use[1] = true;  // Don't use core 1
  }

  unsigned alloc(unsigned gpu_id) {
    if (gpu_id < gpu_affinity.size()) {
      for (unsigned i = 0; i < gpu_affinity[gpu_id].size(); i++) {
        unsigned core = gpu_affinity[gpu_id][i];
        if (!in_use[core]) {
          in_use[core] = true;
          return core;
        }
      }
    }
    // Couldn't get a core with GPU affinity; get a different core
    for (unsigned i = 0; i < in_use.size(); i++) {
      if (!in_use[i]) {
        in_use[i] = true;
        return i;
      }
    }
    std::cerr << "All cores exhausted for GPU " << gpu_id;
    exit(0);
    return 0;
  }

  std::vector<unsigned> alloc(unsigned count, unsigned gpu_id) {
    // std::cout << "Alloc " << count << " on " << gpu_id << " " << str() <<
    // std::endl;
    std::vector<unsigned> result;
    for (unsigned i = 0; i < count; i++) {
      result.push_back(alloc(gpu_id));
    }
    return result;
  }

  std::string str() {
    unsigned allocated = 0;
    for (unsigned i = 0; i < in_use.size(); i++) {
      allocated += in_use[i];
    }
    std::stringstream ss;
    ss << (in_use.size() - allocated) << "/" << in_use.size() << " cores free";
    return ss.str();
  }
};

bool init = false;
CoreManager manager;

// Initializes a firehose process
void initProcess() {
  // Bind to the init pool and set priority to max
  setCores(manager.init_pool, pthread_self());
  setPriority(scheduler, maxPriority(scheduler), pthread_self());
  init = true;
}

void initHighPriorityThread(int num_cores, int gpu_affinity,
                            std::thread &thread) {
  if (init) {
    auto cores = manager.alloc(num_cores, gpu_affinity);
    setCores(cores, thread.native_handle());
    setPriority(scheduler, maxPriority(scheduler), thread.native_handle());
  } else {
    // std::cout << "Warning: trying to initialize high priority thread without
    // threading initialized" << std::endl;
  }
}

void initHighPriorityThread(std::thread &thread) {
  initHighPriorityThread(1, 1, thread);
}

void initHighPriorityThread(std::thread &thread, int num_cores) {
  initHighPriorityThread(num_cores, 1, thread);
}

void initLowPriorityThread(std::thread &thread) {
  // setCores(manager.default_pool, thread.native_handle());
  initHighPriorityThread(1, 0, thread);
  setDefaultPriority(thread.native_handle());
}

void initNetworkThread(std::thread &thread) {
  initHighPriorityThread(1, 0, thread);
}

void initLoggerThread(std::thread &thread) { initLowPriorityThread(thread); }

unsigned coreCount() { return std::thread::hardware_concurrency(); }

void setCore(unsigned core) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (!(rc == 0)) {
    std::cerr << "Unable to set thread affinity: " << rc;
    exit(0);
  }
}

void setCores(std::vector<unsigned> cores, pthread_t thread) {
  if (!(cores.size() > 0)) {
    std::cerr << "Trying to bind to empty core set";
    exit(0);
  }
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  for (unsigned core : cores) {
    CPU_SET(core, &cpuset);
  }
  int rc = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  if (!(rc == 0)) {
    std::cerr << "Unable to set thread affinity: " << rc;
    exit(0);
  }
}

void setAllCores() {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  for (unsigned i = 0; i < coreCount(); i++) {
    CPU_SET(i, &cpuset);
  }
  int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (!(rc == 0)) {
    std::cerr << "Unable to set thread affinity: " << rc;
    exit(0);
  }
}

void addCore(unsigned core) {
  auto cores = currentCores();
  cores.push_back(core);
  setCores(cores, pthread_self());
}

void removeCore(unsigned core) {
  auto cores = currentCores();
  auto it = std::remove(cores.begin(), cores.end(), core);
  cores.erase(it, cores.end());
  setCores(cores, pthread_self());
}

std::vector<unsigned> currentCores() {
  cpu_set_t cpuset;
  int rc = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (!(rc == 0)) {
    std::cerr << "Unable to get thread affinity: " << rc;
    exit(0);
  }
  std::vector<unsigned> cores;
  for (unsigned i = 0; i < coreCount(); i++) {
    if (CPU_ISSET(i, &cpuset) > 0) {
      cores.push_back(i);
    }
  }
  return cores;
}

unsigned getCurrentCore() { return sched_getcpu(); }

int minPriority(int scheduler) { return sched_get_priority_min(scheduler); }

int maxPriority(int scheduler) { return sched_get_priority_max(scheduler); }

void setDefaultPriority() { setDefaultPriority(pthread_self()); }

void setDefaultPriority(pthread_t thread) {
  setPriority(SCHED_OTHER, 0, thread);
}

void setMaxPriority() {
  setPriority(SCHED_FIFO, maxPriority(SCHED_FIFO), pthread_self());
}

void setPriority(int scheduler, int priority, pthread_t thId) {
  struct sched_param params;
  params.sched_priority = sched_get_priority_max(scheduler);
  int ret = pthread_setschedparam(thId, scheduler, &params);
  if (!(ret == 0)) {
    std::cerr << "Unable to set thread priority.  Don't forget to set `rtprio` "
                 "to unlimited in `limits.conf`.  See firehose README for "
                 "instructions";
    exit(0);
  }

  int policy = 0;
  ret = pthread_getschedparam(thId, &policy, &params);
  if (!(ret == 0)) {
    std::cerr << "Unable to verify thread scheduler params";
    exit(0);
  }
  if (!(policy == scheduler)) {
    std::cerr << "Unable to verify thread scheduler params";
    exit(0);
  }
}

int currentScheduler() {
  pthread_t thId = pthread_self();

  struct sched_param params;
  int policy = 0;
  int ret = pthread_getschedparam(thId, &policy, &params);
  if (!(ret == 0)) {
    std::cerr << "Unable to get current thread scheduler params";
    exit(0);
  }

  return policy;
}

int currentPriority() {
  pthread_t thId = pthread_self();

  struct sched_param params;
  int policy = 0;
  int ret = pthread_getschedparam(thId, &policy, &params);
  if (!(ret == 0)) {
    std::cerr << "Unable to get current thread scheduler params";
    exit(0);
  }

  return params.sched_priority;
}
}
}
