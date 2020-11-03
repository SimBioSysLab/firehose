//TODO: credits to Clockwork

#ifndef _FIREHOSE_THREAD_H_
#define _FIREHOSE_THREAD_H_

#include <thread>
#include <vector>

namespace firehose {
namespace threading {

// Initializes a firehose process
void initProcess();

// Initializes a network thread
void initNetworkThread(std::thread &thread);

// Initializes a logger thread
void initLoggerThread(std::thread &thread);

// Initializes a high priority CPU thread
void initHighPriorityThread(std::thread &thread);
void initHighPriorityThread(std::thread &thread, int num_cores);

// Initializes a low priority CPU thread
void initLowPriorityThread(std::thread &thread);

unsigned coreCount();
void setCore(unsigned core);
void setCores(std::vector<unsigned> cores, pthread_t thread);
void setAllCores();
void addCore(unsigned core);
void removeCore(unsigned core);
std::vector<unsigned> currentCores();
unsigned getCurrentCore();

int minPriority(int scheduler);
int maxPriority(int scheduler);

void setDefaultPriority();
void setDefaultPriority(pthread_t thread);
void setMaxPriority();
void setPriority(int scheduler, int priority, pthread_t thread);

int currentScheduler();
int currentPriority();
}
}

#endif
