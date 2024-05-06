#include <future>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <pthread.h>
#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <assert.h>
#include <cstring>

#include <ThreadSafeQueue.hpp>
#include <ThreadSafeLockFreeQueue.hpp>
#include <ThreadPools.hpp>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>
#include <iostream>

/*
 * OPTIONS
 */
//#define RUNTIME_PROFILE
//#define RUNTIME_PRINT
//#define DSWP_STATS

using namespace arcana::virgil;

#define CACHE_LINE_SIZE 64

#ifdef DSWP_STATS
static int64_t numberOfPushes8 = 0;
static int64_t numberOfPushes16 = 0;
static int64_t numberOfPushes32 = 0;
static int64_t numberOfPushes64 = 0;
#endif

typedef struct {
  void (*parallelizedLoop)(void *, int64_t, int64_t, int64_t, void *);
  void *env;
  int64_t coreID;
  int64_t numCores;
  int64_t chunkSize;
  void *scylaxPerCoreData;
  pthread_spinlock_t endLock;
} DOALL_args_t;

// about 15M for starting size
#define OUTPUT_BUFFER_SIZE 1 << 24

typedef struct {
  void *outputQueues;
  int64_t numCores;
  int64_t chunkSize;
  pthread_spinlock_t printLock;
} NOELLE_Scylax_PrinterArgs_t;

typedef struct {
  void *outputQueue;
  char *outputBuffer;
  size_t outputBufferPos;
  size_t outputBufferSize;
  FILE *currentOutputDest;
  int64_t coreNum;
} NOELLE_Scylax_PerCoreArgs_t;

enum OutputMessageType {
  PRINT,
  CHUNK_DONE,
  PRINT_CHUNK_DONE,
  CORE_DONE,
  PRINT_CORE_DONE
};

typedef struct {
  OutputMessageType messageType;
  FILE *outputDest;
  char *str;
  size_t len;
} NOELLE_OutputMessage_t;

typedef BlockingReaderWriterQueue<NOELLE_OutputMessage_t *>
    NOELLE_OutputQueue_t;

class NoelleRuntime {
public:
  NoelleRuntime();

  uint32_t reserveCores(uint32_t coresRequested);

  void releaseCores(uint32_t coresReleased);

  uint32_t getAvailableCores(void);

  DOALL_args_t *getDOALLArgs(uint32_t cores, uint32_t *index);

  void releaseDOALLArgs(uint32_t index);

  ThreadPoolForCSingleQueue *virgil;

  ~NoelleRuntime(void);

private:
  mutable pthread_spinlock_t doallMemoryLock;
  std::vector<uint32_t> doallMemorySizes;
  std::vector<bool> doallMemoryAvailability;
  std::vector<DOALL_args_t *> doallMemory;

  uint32_t getMaximumNumberOfCores(void);

  /*
   * Current number of idle cores.
   */
  int32_t NOELLE_idleCores;

  /*
   * Maximum number of cores.
   */
  uint32_t maxCores;

  mutable pthread_spinlock_t spinLock;
};

#ifdef RUNTIME_PROFILE
uint64_t clocks_starts[64];
uint64_t clocks_ends[64];
uint64_t clocks_dispatch_starts[64];
uint64_t clocks_dispatch_ends[64];
#endif

/*
 * Synchronize printing
 */
#ifdef RUNTIME_PROFILE
#  define RUNTIME_PRINT_LOCK
#endif
#ifdef RUNTIME_PRINT
#  define RUNTIME_PRINT_LOCK
#endif
#ifdef RUNTIME_PRINT_LOCK
pthread_spinlock_t printLock;
#endif

static NoelleRuntime runtime{};

extern "C" {

/************************************ NOELLE public APIs **************/
class DispatcherInfo {
public:
  int32_t numberOfThreadsUsed;
  int64_t unusedVariableToPreventOptIfStructHasOnlyOneVariable;
};

/*
 * Dispatch tasks to run a DOALL loop.
 */
DispatcherInfo NOELLE_DOALLDispatcher(
    void (*parallelizedLoop)(void *, int64_t, int64_t, int64_t, void *),
    void *env,
    int64_t maxNumberOfCores,
    int64_t chunkSize,
    int8_t useScylax);

/*
 * Dispatch tasks to run a HELIX loop.
 */
DispatcherInfo NOELLE_HELIX_dispatcher_sequentialSegments(
    void (*parallelizedLoop)(void *,    // env
                             void *,    // loopCarriedArray
                             void *,    // ssPastArray
                             void *,    // ssFutureArray
                             void *,    // scylaxData
                             int64_t,   // coreID
                             int64_t,   // numCores
                             uint64_t * // loopIsOverFlag
                             ),
    void *env,
    void *loopCarriedArray,
    int64_t numCores,
    int64_t numOfsequentialSegments,
    int8_t useScylax);

DispatcherInfo NOELLE_HELIX_dispatcher_criticalSections(
    void (*parallelizedLoop)(void *,
                             void *,
                             void *,
                             void *,
                             void *,
                             int64_t,
                             int64_t,
                             uint64_t *),
    void *env,
    void *loopCarriedArray,
    int64_t numCores,
    int64_t numOfsequentialSegments);

DispatcherInfo NOELLE_DSWPDispatcher(void *env,
                                     int64_t *queueSizes,
                                     void *stages,
                                     int64_t numberOfStages,
                                     int64_t numberOfQueues);

/******************************************* Utils ********************/
void NOELLE_Scylax(void *args);

#ifdef RUNTIME_PROFILE
static __inline__ int64_t rdtsc_s(void) {
  unsigned a, d;
  asm volatile("cpuid" ::: "%rax", "%rbx", "%rcx", "%rdx");
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
  return ((unsigned long)a) | (((unsigned long)d) << 32);
}
static __inline__ int64_t rdtsc_e(void) {
  unsigned a, d;
  asm volatile("rdtscp" : "=a"(a), "=d"(d));
  asm volatile("cpuid" ::: "%rax", "%rbx", "%rcx", "%rdx");
  return ((unsigned long)a) | (((unsigned long)d) << 32);
}
#endif

/************************************* NOELLE API implementations ***/
typedef void (*stageFunctionPtr_t)(void *, void *);

void printReachedS(std::string s) {
  auto outS = "Reached: " + s;
  printf("%s\n", outS.c_str());
}

void printReachedI(int i) {
  printf("Reached: %d\n", i);
}

void printPushedP(int32_t *p) {
  printf("Pushed: %p\n", p);
}

void printPulledP(int32_t *p) {
  printf("Pulled: %p\n", p);
}

void queuePush8(ThreadSafeQueue<int8_t> *queue, int8_t *val) {
  queue->push(*val);

#ifdef DSWP_STATS
  numberOfPushes8++;
#endif

  return;
}

void queuePop8(ThreadSafeQueue<int8_t> *queue, int8_t *val) {
  queue->waitPop(*val);
  return;
}

void queuePush16(ThreadSafeQueue<int16_t> *queue, int16_t *val) {
  queue->push(*val);

#ifdef DSWP_STATS
  numberOfPushes16++;
#endif

  return;
}

void queuePop16(ThreadSafeQueue<int16_t> *queue, int16_t *val) {
  queue->waitPop(*val);
}

void queuePush32(ThreadSafeQueue<int32_t> *queue, int32_t *val) {
  queue->push(*val);

#ifdef DSWP_STATS
  numberOfPushes32++;
#endif

  return;
}

void queuePop32(ThreadSafeQueue<int32_t> *queue, int32_t *val) {
  queue->waitPop(*val);
}

void queuePush64(ThreadSafeQueue<int64_t> *queue, int64_t *val) {
  queue->push(*val);

#ifdef DSWP_STATS
  numberOfPushes64++;
#endif

  return;
}

void queuePop64(ThreadSafeQueue<int64_t> *queue, int64_t *val) {
  queue->waitPop(*val);

  return;
}

void queuePushOutputMessage(NOELLE_OutputQueue_t *queue,
                            NOELLE_OutputMessage_t *val) {
  queue->enqueue(val);
  return;
}

/**********************************************************************
 *                DOALL
 **********************************************************************/
static void NOELLE_DOALLTrampoline(void *args) {
#ifdef RUNTIME_PROFILE
  auto clocks_start = rdtsc_s();
#endif

  /*
   * Fetch the arguments.
   */
  auto DOALLArgs = (DOALL_args_t *)args;

  /*
   * Invoke
   */
  DOALLArgs->parallelizedLoop(DOALLArgs->env,
                              DOALLArgs->coreID,
                              DOALLArgs->numCores,
                              DOALLArgs->chunkSize,
                              DOALLArgs->scylaxPerCoreData);
#ifdef RUNTIME_PROFILE
  auto clocks_end = rdtsc_e();
  clocks_starts[DOALLArgs->coreID] = clocks_start;
  clocks_ends[DOALLArgs->coreID] = clocks_end;
#endif

  pthread_spin_unlock(&(DOALLArgs->endLock));
  return;
}

NOELLE_Scylax_PerCoreArgs_t *NOELLE_Scylax_CreatePerCoreArgs(
    int64_t coreNum,
    void *outputQueue) {
  NOELLE_Scylax_PerCoreArgs_t *args = (NOELLE_Scylax_PerCoreArgs_t *)malloc(
      sizeof(NOELLE_Scylax_PerCoreArgs_t));
  args->outputQueue = outputQueue;
  args->outputBuffer = (char *)malloc(OUTPUT_BUFFER_SIZE);
  args->outputBufferPos = 0;
  args->outputBufferSize = OUTPUT_BUFFER_SIZE;
  args->currentOutputDest = stdout;
  args->coreNum = coreNum;
  return args;
}

DispatcherInfo NOELLE_DOALLDispatcher(
    void (*parallelizedLoop)(void *, int64_t, int64_t, int64_t, void *),
    void *env,
    int64_t maxNumberOfCores,
    int64_t chunkSize,
    int8_t useScylax) {
#ifdef RUNTIME_PROFILE
  auto clocks_start = rdtsc_s();
#endif

  /*
   * Fetch VIRGIL
   */
  auto virgil = runtime.virgil;

  /*
   * Set the number of cores to use.
   */
  auto numCores = runtime.reserveCores(maxNumberOfCores);
#ifdef RUNTIME_PRINT
  std::cerr << "DOALL: Dispatcher: Start" << std::endl;
  std::cerr << "DOALL: Dispatcher:   Number of cores: " << numCores
            << std::endl;
  std::cerr << "DOALL: Dispatcher:   Chunk size: " << chunkSize << std::endl;
#endif

  /*
   * Allocate the memory to store the arguments.
   */
  uint32_t doallMemoryIndex;
  auto argsForAllCores = runtime.getDOALLArgs(numCores - 1, &doallMemoryIndex);

  /*
   * Allocate the output queues.
   */
  NOELLE_OutputQueue_t *outputQueues[numCores];
  NOELLE_Scylax_PerCoreArgs_t *scylaxPerCoreArgs[numCores];
  if (useScylax) {
    for (auto idx = 0; idx < numCores; idx++) {
      outputQueues[idx] =
          new BlockingReaderWriterQueue<NOELLE_OutputMessage_t *>();
    }
  }

  /*
   * Submit DOALL tasks.
   */
  for (auto i = 0; i < (numCores - 1); ++i) {

    /*
     * Prepare the arguments.
     */
    NOELLE_Scylax_PerCoreArgs_t *scylaxPerCore = nullptr;
    if (useScylax) {
      scylaxPerCore = NOELLE_Scylax_CreatePerCoreArgs(i, outputQueues[i]);
      scylaxPerCoreArgs[i] = scylaxPerCore;
    }

    auto argsPerCore = &argsForAllCores[i];
    argsPerCore->parallelizedLoop = parallelizedLoop;
    argsPerCore->env = env;
    argsPerCore->numCores = numCores;
    argsPerCore->chunkSize = chunkSize;
    argsPerCore->scylaxPerCoreData = scylaxPerCore;

#ifdef RUNTIME_PROFILE
    clocks_dispatch_starts[i] = rdtsc_s();
#endif

    /*
     * Submit
     */
    virgil->submitAndDetach(NOELLE_DOALLTrampoline, argsPerCore);

#ifdef RUNTIME_PROFILE
    clocks_dispatch_ends[i] = rdtsc_s();
#endif
  }
#ifdef RUNTIME_PRINT
  std::cerr << "DOALL: Dispatcher:   Submitted " << numCores
            << " task instances" << std::endl;
#endif
#ifdef RUNTIME_PROFILE
  auto clocks_after_fork = rdtsc_e();
#endif
  /*
   * Allocate and prepare print watchdog args
   */
  NOELLE_Scylax_PrinterArgs_t *scylaxArgs = nullptr;
  if (useScylax) {
    scylaxArgs = (NOELLE_Scylax_PrinterArgs_t *)malloc(
        sizeof(NOELLE_Scylax_PrinterArgs_t));
    scylaxArgs->outputQueues = (void *)outputQueues;
    scylaxArgs->numCores = numCores;
    scylaxArgs->chunkSize = chunkSize;
    pthread_spin_init(&scylaxArgs->printLock, PTHREAD_PROCESS_SHARED);
    pthread_spin_lock(&scylaxArgs->printLock);

    /*
     * Submit watchdog
     */
    virgil->submitAndDetach(NOELLE_Scylax, scylaxArgs);
  }
  /*
   * Run a task.
   */
  NOELLE_Scylax_PerCoreArgs_t scylaxDataForThisCore = {
    outputQueues[numCores - 1],
    (char *)malloc(OUTPUT_BUFFER_SIZE),
    0,
    OUTPUT_BUFFER_SIZE,
    nullptr,
    numCores - 1,
  };

  parallelizedLoop(env,
                   numCores - 1,
                   numCores,
                   chunkSize,
                   &scylaxDataForThisCore);
/*
 * Wait for the remaining DOALL tasks.
 */
#ifdef RUNTIME_PROFILE
  auto clocks_before_join = rdtsc_s();
#endif
  for (auto i = 0; i < (numCores - 1); ++i) {
    pthread_spin_lock(&(argsForAllCores[i].endLock));
  }

  /*
   * Wait for printer thread.
   */
  if (useScylax) {
    pthread_spin_lock(&(scylaxArgs->printLock));
  }

#ifdef RUNTIME_PRINT
  std::cerr << "DOALL: Dispatcher:   All task instances have completed"
            << std::endl;
#endif
#ifdef RUNTIME_PROFILE
  auto clocks_after_join = rdtsc_e();
  auto clocks_before_cleanup = rdtsc_s();
#endif

  /*
   * Free the cores and memory.
   */
  runtime.releaseCores(numCores);
  runtime.releaseDOALLArgs(doallMemoryIndex);
  if (useScylax) {
    free(scylaxArgs);
    for (auto idx = 0; idx < numCores; idx++) {
      delete outputQueues[idx];
    }
    for (auto idx = 0; idx < (numCores - 1); idx++) {
      free(scylaxPerCoreArgs[idx]->outputBuffer);
      free(scylaxPerCoreArgs[idx]);
    }
  }

  /*
   * Prepare the return value.
   */
  DispatcherInfo dispatcherInfo;
  dispatcherInfo.numberOfThreadsUsed = numCores;
#ifdef RUNTIME_PROFILE
  auto clocks_after_cleanup = rdtsc_s();
  pthread_spin_lock(&printLock);
  std::cerr << "XAN: Start         = " << clocks_start << "\n";
  std::cerr
      << "XAN: Setup overhead         = " << clocks_after_fork - clocks_start
      << " clocks\n";
  uint64_t totalDispatch = 0;
  for (auto i = 0; i < (numCores - 1); i++) {
    totalDispatch += (clocks_dispatch_ends[i] - clocks_dispatch_starts[i]);
  }
  std::cerr << "XAN:    Dispatch overhead         = " << totalDispatch
            << " clocks\n";
  std::cerr << "XAN: Start joining = " << clocks_after_fork << "\n";
  for (auto i = 0; i < (numCores - 1); i++) {
    std::cerr << "Thread " << i << ": Start = " << clocks_starts[i] << "\n";
    std::cerr << "Thread " << i << ": End   = " << clocks_ends[i] << "\n";
    std::cerr << "Thread " << i
              << ": Delta = " << clocks_ends[i] - clocks_starts[i] << "\n";
  }
  std::cerr << "XAN: Joined        = " << clocks_after_join << "\n";
  std::cerr << "XAN: Joining delta = " << clocks_after_join - clocks_before_join
            << "\n";

  uint64_t start_min = 0;
  uint64_t start_max = 0;
  for (auto i = 0; i < (numCores - 1); i++) {
    if (false || (start_min == 0) || (clocks_starts[i] < start_min)) {
      start_min = clocks_starts[i];
    }
    if (clocks_starts[i] > start_max) {
      start_max = clocks_starts[i];
    }
  }
  std::cerr << "XAN: Thread starts min = " << start_min << "\n";
  std::cerr << "XAN: Thread starts max = " << start_max << "\n";
  std::cerr << "XAN: Task starting overhead = " << start_max - start_min
            << "\n";

  uint64_t end_max = 0;
  uint64_t lastThreadID = 0;
  for (auto i = 0; i < (numCores - 1); i++) {
    if (clocks_ends[i] > end_max) {
      lastThreadID = i;
      end_max = clocks_ends[i];
    }
  }
  std::cerr << "XAN: Last thread ended = " << end_max << " (thread "
            << lastThreadID << ")\n";
  std::cerr << "XAN: Joining overhead       = " << clocks_after_join - end_max
            << "\n";

  pthread_spin_unlock(&printLock);
#endif
#ifdef RUNTIME_PRINT
  std::cerr << "DOALL: Dispatcher: Exit" << std::endl;
#endif

  return dispatcherInfo;
}

void NOELLE_Scylax_FlushBuffer(void *scylaxData, OutputMessageType type) {

  NOELLE_Scylax_PerCoreArgs_t *p_args =
      reinterpret_cast<NOELLE_Scylax_PerCoreArgs_t *>(scylaxData);
  NOELLE_OutputQueue_t *outputQueue =
      reinterpret_cast<NOELLE_OutputQueue_t *>(p_args->outputQueue);

  NOELLE_OutputMessage_t *chainLink =
      (NOELLE_OutputMessage_t *)malloc(sizeof(NOELLE_OutputMessage_t));

  // Coming from ChunkEnd/TaskEnd, PRINT_ only if applicable
  if ((type == PRINT_CHUNK_DONE || type == PRINT_CORE_DONE)
      && p_args->outputBufferPos == 0) {
    type = type == PRINT_CHUNK_DONE ? CHUNK_DONE : CORE_DONE;
  }
  chainLink->messageType = type;

  if (type == PRINT || type == PRINT_CHUNK_DONE || type == PRINT_CORE_DONE) {
    chainLink->outputDest = p_args->currentOutputDest;
    chainLink->str = (char *)malloc(p_args->outputBufferPos + 1);

    chainLink->len = p_args->outputBufferPos;
    strncpy(chainLink->str, p_args->outputBuffer, p_args->outputBufferPos);
    p_args->outputBufferPos = 0;
  }

  queuePushOutputMessage(outputQueue, chainLink);
  return;
}

void NOELLE_DOALL_Scylax_ChunkEnd(int8_t isChunkCompleted, void *scylaxData) {
  // This function will be called every iteration, doing the check here instead
  // // simplifies manual IR generation for the same result when optimized
  if (!isChunkCompleted) {
    return;
  }
  return NOELLE_Scylax_FlushBuffer(scylaxData, PRINT_CHUNK_DONE);
}

void NOELLE_DOALL_Scylax_TaskEnd(void *scylaxData) {
  return NOELLE_Scylax_FlushBuffer(scylaxData, PRINT_CORE_DONE);
}

char *NOELLE_Scylax_GetWritePtr(void *scylaxData, FILE *stream, int64_t size) {
  NOELLE_Scylax_PerCoreArgs_t *p_args =
      reinterpret_cast<NOELLE_Scylax_PerCoreArgs_t *>(scylaxData);

  if (p_args->currentOutputDest == nullptr) {
    p_args->currentOutputDest = stream;
  } else if (p_args->currentOutputDest != stream) {
    NOELLE_Scylax_FlushBuffer(scylaxData, PRINT);
    p_args->currentOutputDest = stream;
  }

  if (p_args->outputBufferPos + size > p_args->outputBufferSize) {
    int64_t newSize = p_args->outputBufferSize << 1;
    p_args->outputBuffer = (char *)realloc(p_args->outputBuffer, newSize);
    p_args->outputBufferSize = newSize;
  }

  return p_args->outputBuffer + p_args->outputBufferPos;
}

void NOELLE_Scylax_AdvanceOutputBufferPos(void *scylaxData, int64_t len) {
  NOELLE_Scylax_PerCoreArgs_t *p_args =
      reinterpret_cast<NOELLE_Scylax_PerCoreArgs_t *>(scylaxData);
  p_args->outputBufferPos += len;
  return;
}

int NOELLE_Scylax_printf(void *scylaxData, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int neededBytes = vsnprintf(0, 0, format, args) + 1; // for null
  va_end(args);

  char *writePtr = NOELLE_Scylax_GetWritePtr(scylaxData, stdout, neededBytes);

  va_start(args, format);
  vsprintf(writePtr, format, args);
  va_end(args);

  NOELLE_Scylax_AdvanceOutputBufferPos(scylaxData, neededBytes - 1);
  return neededBytes - 1;
}

int NOELLE_Scylax_printf_knownMaxLength(void *scylaxData,
                                        int64_t maxLen,
                                        const char *format,
                                        ...) {
  char *writePtr = NOELLE_Scylax_GetWritePtr(scylaxData, stdout, maxLen);

  va_list args;
  va_start(args, format);
  int writtenBytes = vsnprintf(writePtr, maxLen, format, args);
  va_end(args);

  NOELLE_Scylax_AdvanceOutputBufferPos(scylaxData, writtenBytes);
  return writtenBytes;
}

int NOELLE_Scylax_fprintf(void *scylaxData,
                          FILE *stream,
                          const char *format,
                          ...) {
  va_list args;
  va_start(args, format);
  int neededBytes = vsnprintf(0, 0, format, args) + 1; // for null
  va_end(args);

  char *writePtr = NOELLE_Scylax_GetWritePtr(scylaxData, stream, neededBytes);

  va_start(args, format);
  vsprintf(writePtr, format, args);
  va_end(args);

  NOELLE_Scylax_AdvanceOutputBufferPos(scylaxData, neededBytes - 1);
  return neededBytes - 1;
}

int NOELLE_Scylax_fprintf_knownMaxLength(void *scylaxData,
                                         int64_t maxLen,
                                         FILE *stream,
                                         const char *format,
                                         ...) {
  char *writePtr = NOELLE_Scylax_GetWritePtr(scylaxData, stream, maxLen);

  va_list args;
  va_start(args, format);
  int writtenBytes = vsnprintf(writePtr, maxLen, format, args);
  va_end(args);

  NOELLE_Scylax_AdvanceOutputBufferPos(scylaxData, writtenBytes);
  return writtenBytes;
}

int NOELLE_Scylax_putc(void *scylaxData, int character, FILE *stream) {
  char *writePtr = NOELLE_Scylax_GetWritePtr(scylaxData, stream, 2);

  *writePtr = character;
  *(writePtr + 1) = '\0';

  NOELLE_Scylax_AdvanceOutputBufferPos(scylaxData, 1);
  return character;
}

int NOELLE_Scylax_putchar(void *scylaxData, int character) {
  return NOELLE_Scylax_putc(scylaxData, character, stdout);
}

int NOELLE_Scylax_puts(void *scylaxData, const char *str) {
  int neededBytes = strlen(str) + 2; // for newline + null
  char *writePtr = NOELLE_Scylax_GetWritePtr(scylaxData, stdout, neededBytes);

  strcpy(writePtr, str);
  writePtr[neededBytes - 2] = '\n';
  writePtr[neededBytes - 1] = 0;

  NOELLE_Scylax_AdvanceOutputBufferPos(scylaxData, neededBytes - 1);
  return 27;
}

void NOELLE_Scylax_perror(void *scylaxData, const char *prefix) {
  bool usePrefix = prefix != 0 && *prefix != 0;
  char *errorString = strerror(errno);

  if (usePrefix) {
    int prefixLen = strlen(prefix);
    int neededBytes = prefixLen + 2 + strlen(errorString) + 2;
    char *writePtr = NOELLE_Scylax_GetWritePtr(scylaxData, stderr, neededBytes);
    strcpy(writePtr, prefix);
    writePtr[prefixLen] = ':';
    writePtr[prefixLen + 1] = ' ';
    strcpy(writePtr + prefixLen + 2, errorString);
    writePtr[neededBytes - 2] = '\n';
    writePtr[neededBytes - 1] = '0';

    NOELLE_Scylax_AdvanceOutputBufferPos(scylaxData, neededBytes - 1);
  }
  return;
}

void NOELLE_Scylax(void *args) {

  NOELLE_Scylax_PrinterArgs_t *p_args =
      reinterpret_cast<NOELLE_Scylax_PrinterArgs_t *>(args);
  NOELLE_OutputQueue_t **outputQueues =
      reinterpret_cast<NOELLE_OutputQueue_t **>(p_args->outputQueues);
  int64_t numCores = p_args->numCores;
  int64_t chunkSize = p_args->chunkSize;

  bool coreDone[numCores];
  for (auto idx = 0; idx < numCores; idx++)
    coreDone[idx] = 0;
  int numDoneCores = 0;

  bool nextCore;
  int coreIdx = 0;

  do {   // loop through active cores until all are done
    do { // handle messages for this core until the chunk/core is finished
      nextCore = false;

      if (coreDone[coreIdx]) {
        nextCore = true;

      } else {
        NOELLE_OutputMessage_t *currentMessage;
        outputQueues[coreIdx]->wait_dequeue(currentMessage);

        if (currentMessage->messageType == PRINT
            || currentMessage->messageType == PRINT_CHUNK_DONE
            || currentMessage->messageType == PRINT_CORE_DONE) {
          fputs(currentMessage->str, currentMessage->outputDest);
          free(currentMessage->str);
        }
        if (currentMessage->messageType == CHUNK_DONE
            || currentMessage->messageType == PRINT_CHUNK_DONE) {
          nextCore = true;
        } else if (currentMessage->messageType == CORE_DONE
                   || currentMessage->messageType == PRINT_CORE_DONE) {
          nextCore = true;
          coreDone[coreIdx] = true;
          numDoneCores++;
        }
        free(currentMessage);
      }
    } while (!nextCore);
    coreIdx = ++coreIdx % numCores;
  } while (numDoneCores != numCores);

  pthread_spin_unlock(&p_args->printLock);
  return;
}

/**********************************************************************
 *                HELIX
 **********************************************************************/
typedef struct {
  void (*parallelizedLoop)(void *,
                           void *,
                           void *,
                           void *,
                           void *,
                           int64_t,
                           int64_t,
                           uint64_t *);
  void *env;
  void *loopCarriedArray;
  void *ssArrayPast;
  void *ssArrayFuture;
  void *scylaxData;
  uint64_t coreID;
  uint64_t numCores;
  uint64_t *loopIsOverFlag;
  pthread_spinlock_t endLock;
} NOELLE_HELIX_args_t;

static void NOELLE_HELIXTrampoline(void *args) {

  /*
   * Fetch the arguments.
   */
  auto HELIX_args = (NOELLE_HELIX_args_t *)args;

  /*
   * Invoke
   */
  HELIX_args->parallelizedLoop(HELIX_args->env,
                               HELIX_args->loopCarriedArray,
                               HELIX_args->ssArrayPast,
                               HELIX_args->ssArrayFuture,
                               HELIX_args->scylaxData,
                               HELIX_args->coreID,
                               HELIX_args->numCores,
                               HELIX_args->loopIsOverFlag);

  pthread_spin_unlock(&(HELIX_args->endLock));
  return;
}

static void HELIX_helperThread(void *ssArray,
                               uint32_t numOfsequentialSegments,
                               uint64_t *theLoopIsOver) {

  while ((*theLoopIsOver) == 0) {

    /*
     * Prefetch all sequential segment cache lines of the current loop
     * iteration.
     */
    for (auto i = 0; ((*theLoopIsOver) == 0) && (i < numOfsequentialSegments);
         i++) {

      /*
       * Fetch the pointer.
       */
      auto ptr = (uint64_t *)(((uint64_t)ssArray) + (i * CACHE_LINE_SIZE));

      /*
       * Prefetch the cache line for the current sequential segment.
       */
      while (((*theLoopIsOver) == 0) && ((*ptr) == 0))
        ;
    }
  }

  return;
}

static DispatcherInfo NOELLE_HELIX_dispatcher(
    void (*parallelizedLoop)(void *,
                             void *,
                             void *,
                             void *,
                             void *,
                             int64_t,
                             int64_t,
                             uint64_t *),
    void *env,
    void *loopCarriedArray,
    int64_t maxNumberOfCores,
    int64_t numOfsequentialSegments,
    bool LIO,
    bool useScylax) {

  /*
   * Assumptions.
   */
  assert(parallelizedLoop != NULL);
  assert(env != NULL);
  assert(maxNumberOfCores > 1);

  /*
   * Fetch VIRGIL
   */
  auto virgil = runtime.virgil;

  /*
   * Reserve the cores.
   */
  auto numCores = runtime.reserveCores(maxNumberOfCores);
  assert(numCores >= 1);

#ifdef RUNTIME_PRINT
  pthread_spin_lock(&printLock);
  std::cerr << "HELIX: dispatcher: Start" << std::endl;
  std::cerr << "HELIX: dispatcher:   Number of sequential segments = "
            << numOfsequentialSegments << std::endl;
  std::cerr << "HELIX: dispatcher:   Number of cores = " << numCores
            << std::endl;
  pthread_spin_unlock(&printLock);
#endif

  /*
   * Allocate the sequential segment arrays.
   * We need numCores - 1 arrays.
   */
  auto numOfSSArrays = numCores;
  if (!LIO) {
    numOfSSArrays = 1;
  }
  void *ssArrays = nullptr;
  auto ssSize = CACHE_LINE_SIZE;
  auto ssArraySize = ssSize * numOfsequentialSegments;
  if (numOfsequentialSegments > 0) {

    /*
     * Allocate the sequential segment arrays.
     */
    posix_memalign(&ssArrays, CACHE_LINE_SIZE, ssArraySize * numOfSSArrays);
    if (ssArrays == NULL) {
      std::cerr << "HELIX: dispatcher: ERROR = not enough memory to allocate "
                << numCores << " sequential segment arrays" << std::endl;
      abort();
    }

    /*
     * Initialize the sequential segment arrays.
     */
    for (auto i = 0; i < numOfSSArrays; i++) {

      /*
       * Fetch the current sequential segment array.
       */
      auto ssArray = (void *)(((uint64_t)ssArrays) + (i * ssArraySize));

      /*
       * Initialize the locks.
       */
      for (auto lockID = 0; lockID < numOfsequentialSegments; lockID++) {

        /*
         * Fetch the pointer to the current lock.
         */
        auto lock =
            (pthread_spinlock_t *)(((uint64_t)ssArray) + (lockID * ssSize));

        /*
         * Initialize the lock.
         */
        pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE);

        /*
         * If the sequential segment is not for core 0, then we need to lock
         * it.
         */
        if (i > 0) {
          pthread_spin_lock(lock);
        }
      }
    }
  }

  /*
   * Allocate the arguments for the cores.
   */
  NOELLE_HELIX_args_t *argsForAllCores;
  posix_memalign((void **)&argsForAllCores,
                 CACHE_LINE_SIZE,
                 sizeof(NOELLE_HELIX_args_t) * (numCores - 1));
  /*
   * Allocate the output queues.
   */
  NOELLE_OutputQueue_t *outputQueues[numCores];
  NOELLE_Scylax_PerCoreArgs_t *scylaxPerCoreArgs[numCores];
  if (useScylax) {
    for (auto idx = 0; idx < numCores; idx++) {
      outputQueues[idx] =
          new BlockingReaderWriterQueue<NOELLE_OutputMessage_t *>();
    }
  }

  /*
   * Launch threads
   */
  uint64_t loopIsOverFlag = 0;
  cpu_set_t cores;
  for (auto i = 0; i < (numCores - 1); ++i) {

    /*
     * Identify the past and future sequential segment arrays.
     */
    auto pastID = i % numOfSSArrays;
    auto futureID = (i + 1) % numOfSSArrays;

    /*
     * Fetch the sequential segment array for the current thread.
     */
    auto ssArrayPast = (void *)(((uint64_t)ssArrays) + (pastID * ssArraySize));
    auto ssArrayFuture =
        (void *)(((uint64_t)ssArrays) + (futureID * ssArraySize));
#ifdef RUNTIME_PRINT
    pthread_spin_lock(&printLock);
    auto pastDelta = (int *)ssArrayPast - (int *)ssArrays;
    auto futureDelta = (int *)ssArrayFuture - (int *)ssArrays;
    std::cerr << "HELIX: dispatcher:   Task instance " << i << std::endl;
    std::cerr << "HELIX: dispatcher:     SS arrays: " << ssArrays << std::endl;
    std::cerr << "HELIX: dispatcher:       SS past: " << ssArrayPast
              << std::endl;
    std::cerr << "HELIX: dispatcher:       SS future: " << ssArrayFuture
              << std::endl;
    std::cerr
        << "HELIX: dispatcher:       SS past and future arrays: " << pastDelta
        << " " << futureDelta << std::endl;
    pthread_spin_unlock(&printLock);
#endif

    /*
     * Prepare the arguments.
     */
    NOELLE_Scylax_PerCoreArgs_t *scylaxPerCore = nullptr;
    if (useScylax) {
      scylaxPerCore = NOELLE_Scylax_CreatePerCoreArgs(i, outputQueues[i]);
      scylaxPerCoreArgs[i] = scylaxPerCore;
    }
    auto argsPerCore = &argsForAllCores[i];
    argsPerCore->parallelizedLoop = parallelizedLoop;
    argsPerCore->env = env;
    argsPerCore->loopCarriedArray = loopCarriedArray;
    argsPerCore->ssArrayPast = ssArrayPast;
    argsPerCore->ssArrayFuture = ssArrayFuture;
    argsPerCore->scylaxData = scylaxPerCore;
    argsPerCore->coreID = i;
    argsPerCore->numCores = numCores;
    argsPerCore->loopIsOverFlag = &loopIsOverFlag;
    pthread_spin_init(&(argsPerCore->endLock), PTHREAD_PROCESS_PRIVATE);
    pthread_spin_lock(&(argsPerCore->endLock));

    /*
     * Set the affinity for both the thread and its helper.
     */
    /*CPU_ZERO(&cores);
    auto physicalCore = i * 2;
    CPU_SET(physicalCore, &cores);
    CPU_SET(physicalCore + 1, &cores);
    */

    /*
     * Launch the thread.
     */
    virgil->submitAndDetach(NOELLE_HELIXTrampoline, argsPerCore);

    /*
     * Launch the helper thread.
     */
    continue;
    /*localFutures.push_back(pool.submitToCores(
      cores,
      HELIX_helperThread,
      ssArrayPast,
      numOfsequentialSegments,
      &loopIsOverFlag
    ));*/
  }
#ifdef RUNTIME_PRINT
  pthread_spin_lock(&printLock);
  std::cerr << "HELIX: dispatcher:   Submitted all task instances" << std::endl;
  pthread_spin_unlock(&printLock);
  int futureGotten = 0;
#endif

  /*
   * Prepare args and start watchdog thread
   */
  NOELLE_Scylax_PrinterArgs_t *scylaxArgs = nullptr;
  if (useScylax) {
    scylaxArgs = (NOELLE_Scylax_PrinterArgs_t *)malloc(
        sizeof(NOELLE_Scylax_PrinterArgs_t));
    scylaxArgs->outputQueues = (void *)outputQueues;
    scylaxArgs->numCores = numCores;
    pthread_spin_init(&scylaxArgs->printLock, PTHREAD_PROCESS_SHARED);
    pthread_spin_lock(&scylaxArgs->printLock);
    virgil->submitAndDetach(NOELLE_Scylax, scylaxArgs);
  }

  /*
   * Run a task.
   */
  auto pastID = (numCores - 1) % numOfSSArrays;
  auto futureID = 0;
  auto ssArrayPast = (void *)(((uint64_t)ssArrays) + (pastID * ssArraySize));
  auto ssArrayFuture = ssArrays;
  NOELLE_Scylax_PerCoreArgs_t *scylaxDataForThisCore =
      NOELLE_Scylax_CreatePerCoreArgs(numCores - 1, outputQueues[numCores - 1]);

  parallelizedLoop(env,
                   loopCarriedArray,
                   ssArrayPast,
                   ssArrayFuture,
                   scylaxDataForThisCore,
                   numCores - 1,
                   numCores,
                   &loopIsOverFlag);

  /*
   * Wait for the remaining HELIX tasks.
   */
  for (auto i = 0; i < (numCores - 1); ++i) {
    pthread_spin_lock(&(argsForAllCores[i].endLock));
  }

  /*
   * Wait for printer thread.
   */
  if (useScylax) {
    pthread_spin_lock(&(scylaxArgs->printLock));
  }

#ifdef RUNTIME_PRINT
  pthread_spin_lock(&printLock);
  std::cerr << "HELIX: dispatcher:   All task instances have completed"
            << std::endl;
  pthread_spin_unlock(&printLock);
#endif

  /*
   * Free the cores and memory.
   */
  runtime.releaseCores(numCores);

  /*
   * Free the memory.
   */
  free(argsForAllCores);
  free(ssArrays);

  /*
   * Exit
   */
#ifdef RUNTIME_PRINT
  pthread_spin_lock(&printLock);
  std::cerr << "HELIX: dispatcher: Exit" << std::endl;
  pthread_spin_unlock(&printLock);
#endif

  DispatcherInfo dispatcherInfo;
  dispatcherInfo.numberOfThreadsUsed = numCores;
  return dispatcherInfo;
}

DispatcherInfo NOELLE_HELIX_dispatcher_sequentialSegments(
    void (*parallelizedLoop)(void *,
                             void *,
                             void *,
                             void *,
                             void *,
                             int64_t,
                             int64_t,
                             uint64_t *),
    void *env,
    void *loopCarriedArray,
    int64_t numCores,
    int64_t numOfsequentialSegments,
    int8_t useScylax) {
  return NOELLE_HELIX_dispatcher(parallelizedLoop,
                                 env,
                                 loopCarriedArray,
                                 numCores,
                                 numOfsequentialSegments,
                                 true,
                                 useScylax);
}

DispatcherInfo NOELLE_HELIX_dispatcher_criticalSections(
    void (*parallelizedLoop)(void *,
                             void *,
                             void *,
                             void *,
                             void *,
                             int64_t,
                             int64_t,
                             uint64_t *),
    void *env,
    void *loopCarriedArray,
    int64_t numCores,
    int64_t numOfsequentialSegments) {
  return NOELLE_HELIX_dispatcher(parallelizedLoop,
                                 env,
                                 loopCarriedArray,
                                 numCores,
                                 numOfsequentialSegments,
                                 false,
                                 false);
}

void HELIX_wait(void *sequentialSegment) {

  /*
   * Fetch the spinlock
   */
  auto ss = (pthread_spinlock_t *)sequentialSegment;

#ifdef RUNTIME_PRINT
  assert(ss != NULL);
  pthread_spin_lock(&printLock);
  std::cerr << "HELIX: Waiting on sequential segment " << sequentialSegment
            << std::endl;
  pthread_spin_unlock(&printLock);
#endif

  /*
   * Wait
   */
  pthread_spin_lock(ss);

#ifdef RUNTIME_PRINT
  pthread_spin_lock(&printLock);
  std::cerr << "HELIX: Waited on sequential segment " << sequentialSegment
            << std::endl;
  pthread_spin_unlock(&printLock);
#endif

  return;
}

void HELIX_signal(void *sequentialSegment) {

  /*
   * Fetch the spinlock
   */
  auto ss = (pthread_spinlock_t *)sequentialSegment;

#ifdef RUNTIME_PRINT
  assert(ss != NULL);
  pthread_spin_lock(&printLock);
  std::cerr << "HELIX: Signaling on sequential segment " << sequentialSegment
            << std::endl;
  pthread_spin_unlock(&printLock);
#endif

  /*
   * Signal
   */
  pthread_spin_unlock(ss);

#ifdef RUNTIME_PRINT
  pthread_spin_lock(&printLock);
  std::cerr << "HELIX: Signaled on sequential segment " << sequentialSegment
            << std::endl;
  pthread_spin_unlock(&printLock);
#endif

  return;
}

void NOELLE_HELIX_Scylax_IterEnd(void *scylaxData) {
  return NOELLE_Scylax_FlushBuffer(scylaxData, PRINT_CHUNK_DONE);
}

void NOELLE_HELIX_Scylax_TaskEnd(void *scylaxData) {
  return NOELLE_Scylax_FlushBuffer(scylaxData, PRINT_CORE_DONE);
}

/**********************************************************************
 *                DSWP
 **********************************************************************/
typedef struct {
  stageFunctionPtr_t funcToInvoke;
  void *env;
  void *localQueues;
  pthread_mutex_t endLock;
} NOELLE_DSWP_args_t;

void stageExecuter(void (*stage)(void *, void *), void *env, void *queues) {
  return stage(env, queues);
}

static void NOELLE_DSWPTrampoline(void *args) {

  /*
   * Fetch the arguments.
   */
  auto DSWPArgs = (NOELLE_DSWP_args_t *)args;

  /*
   * Invoke
   */
  DSWPArgs->funcToInvoke(DSWPArgs->env, DSWPArgs->localQueues);

  pthread_mutex_unlock(&(DSWPArgs->endLock));
  return;
}

DispatcherInfo NOELLE_DSWPDispatcher(void *env,
                                     int64_t *queueSizes,
                                     void *stages,
                                     int64_t numberOfStages,
                                     int64_t numberOfQueues) {
#ifdef RUNTIME_PRINT
  std::cerr << "Starting dispatcher: num stages " << numberOfStages
            << ", num queues: " << numberOfQueues << std::endl;
#endif

  /*
   * Fetch VIRGIL
   */
  auto virgil = runtime.virgil;

  /*
   * Reserve the cores.
   */
  auto numCores = runtime.reserveCores(numberOfStages);
  assert(numCores >= 1);

  /*
   * Allocate the communication queues.
   */
  void *localQueues[numberOfQueues];
  for (auto i = 0; i < numberOfQueues; ++i) {
    switch (queueSizes[i]) {
      case 1:
        localQueues[i] = new ThreadSafeLockFreeQueue<int8_t>();
        break;
      case 8:
        localQueues[i] = new ThreadSafeLockFreeQueue<int8_t>();
        break;
      case 16:
        localQueues[i] = new ThreadSafeLockFreeQueue<int16_t>();
        break;
      case 32:
        localQueues[i] = new ThreadSafeLockFreeQueue<int32_t>();
        break;
      case 64:
        localQueues[i] = new ThreadSafeLockFreeQueue<int64_t>();
        break;
      default:
        std::cerr << "NOELLE: Runtime: QUEUE SIZE INCORRECT" << std::endl;
        abort();
        break;
    }
  }
#ifdef RUNTIME_PRINT
  std::cerr << "Made queues" << std::endl;
#endif

  /*
   * Allocate the memory to store the arguments.
   */
  auto argsForAllCores =
      (NOELLE_DSWP_args_t *)malloc(sizeof(NOELLE_DSWP_args_t) * numberOfStages);

  /*
   * Submit DSWP tasks
   */
  auto allStages = (void **)stages;
  for (auto i = 0; i < numberOfStages; ++i) {

    /*
     * Prepare the arguments.
     */
    auto argsPerCore = &argsForAllCores[i];
    argsPerCore->funcToInvoke = reinterpret_cast<stageFunctionPtr_t>(
        reinterpret_cast<long long>(allStages[i]));
    argsPerCore->env = env;
    argsPerCore->localQueues = (void *)localQueues;
    pthread_mutex_init(&(argsPerCore->endLock), NULL);
    pthread_mutex_lock(&(argsPerCore->endLock));

    /*
     * Submit
     */
    virgil->submitAndDetach(NOELLE_DSWPTrampoline, argsPerCore);
#ifdef RUNTIME_PRINT
    std::cerr << "Submitted stage" << std::endl;
#endif
  }
#ifdef RUNTIME_PRINT
  std::cerr << "Submitted pool" << std::endl;
#endif

  /*
   * Wait for the tasks to complete.
   */
  for (auto i = 0; i < numberOfStages; ++i) {
    pthread_mutex_lock(&(argsForAllCores[i].endLock));
  }
#ifdef RUNTIME_PRINT
  std::cerr << "Got all futures" << std::endl;
#endif

  /*
   * Free the cores and memory.
   */
  runtime.releaseCores(numCores);
  for (int i = 0; i < numberOfQueues; ++i) {
    switch (queueSizes[i]) {
      case 1:
        delete (ThreadSafeLockFreeQueue<int8_t> *)(localQueues[i]);
        break;
      case 8:
        delete (ThreadSafeLockFreeQueue<int8_t> *)(localQueues[i]);
        break;
      case 16:
        delete (ThreadSafeLockFreeQueue<int16_t> *)(localQueues[i]);
        break;
      case 32:
        delete (ThreadSafeLockFreeQueue<int32_t> *)(localQueues[i]);
        break;
      case 64:
        delete (ThreadSafeLockFreeQueue<int64_t> *)(localQueues[i]);
        break;
    }
  }
  free(argsForAllCores);

#ifdef DSWP_STATS
  std::cout << "DSWP: 1 Byte pushes = " << numberOfPushes8 << std::endl;
  std::cout << "DSWP: 2 Bytes pushes = " << numberOfPushes16 << std::endl;
  std::cout << "DSWP: 4 Bytes pushes = " << numberOfPushes32 << std::endl;
  std::cout << "DSWP: 8 Bytes pushes = " << numberOfPushes64 << std::endl;
#endif

  DispatcherInfo dispatcherInfo;
  dispatcherInfo.numberOfThreadsUsed = numberOfStages;
  return dispatcherInfo;
}

uint32_t NOELLE_getAvailableCores(void) {
  auto idleCores = runtime.getAvailableCores();

  return idleCores;
}
}

NoelleRuntime::NoelleRuntime() {
  this->maxCores = this->getMaximumNumberOfCores();
  this->NOELLE_idleCores = maxCores;

  pthread_spin_init(&this->spinLock, 0);
  pthread_spin_init(&this->doallMemoryLock, 0);
#ifdef RUNTIME_PRINT_LOCK
  pthread_spin_init(&printLock, 0);
#endif

  /*
   * Allocate VIRGIL
   */
  this->virgil = new ThreadPoolForCSingleQueue(false, maxCores);

  return;
}

DOALL_args_t *NoelleRuntime::getDOALLArgs(uint32_t cores, uint32_t *index) {
  DOALL_args_t *argsForAllCores = nullptr;

  /*
   * Check if we can reuse a previously-allocated memory region.
   */
  pthread_spin_lock(&this->doallMemoryLock);
  auto doallMemoryNumberOfChunks = this->doallMemoryAvailability.size();
  for (auto i = 0; i < doallMemoryNumberOfChunks; i++) {
    auto currentSize = this->doallMemorySizes[i];
    if (true && (this->doallMemoryAvailability[i]) && (currentSize >= cores)) {

      /*
       * Found a memory block that can be reused.
       */
      argsForAllCores = this->doallMemory[i];

      /*
       * Set the block as in use.
       */
      this->doallMemoryAvailability[i] = false;
      (*index) = i;
      pthread_spin_unlock(&this->doallMemoryLock);

      return argsForAllCores;
    }
  }

  /*
   * We couldn't find anything available.
   *
   * Allocate a new memory region.
   */
  this->doallMemorySizes.push_back(cores);
  this->doallMemoryAvailability.push_back(false);
  posix_memalign((void **)&argsForAllCores,
                 CACHE_LINE_SIZE,
                 sizeof(DOALL_args_t) * cores);
  this->doallMemory.push_back(argsForAllCores);
  pthread_spin_unlock(&this->doallMemoryLock);

  /*
   * Set the index.
   */
  (*index) = doallMemoryNumberOfChunks;

  /*
   * Initialize the memory.
   */
  for (auto i = 0; i < cores; ++i) {
    auto argsPerCore = &argsForAllCores[i];
    argsPerCore->coreID = i;
    pthread_spin_init(&(argsPerCore->endLock), 0);
    pthread_spin_lock(&(argsPerCore->endLock));
  }

  return argsForAllCores;
}

void NoelleRuntime::releaseDOALLArgs(uint32_t index) {
  pthread_spin_lock(&this->doallMemoryLock);
  this->doallMemoryAvailability[index] = true;
  pthread_spin_unlock(&this->doallMemoryLock);
  return;
}

uint32_t NoelleRuntime::reserveCores(uint32_t coresRequested) {

  /*
   * Reserve the number of cores available.
   */
  pthread_spin_lock(&this->spinLock);
  auto numCores = (this->NOELLE_idleCores >= coresRequested) ? coresRequested
                                                             : NOELLE_idleCores;
  if (numCores < 1) {
    numCores = 1;
  }
  this->NOELLE_idleCores -= numCores;
  pthread_spin_unlock(&this->spinLock);

  return numCores;
}

void NoelleRuntime::releaseCores(uint32_t coresReleased) {
  assert(coresReleased > 0);

  pthread_spin_lock(&this->spinLock);
  this->NOELLE_idleCores += coresReleased;
#ifdef DEBUG
  if (this->NOELLE_idleCores >= 0) {
    assert(this->NOELLE_idleCores <= ((uint32_t)this->maxCores));
  }
#endif
  pthread_spin_unlock(&this->spinLock);

  return;
}

uint32_t NoelleRuntime::getMaximumNumberOfCores(void) {
  static int cores = 0;

  /*
   * Check if we have already computed the number of cores.
   */
  if (cores == 0) {

    /*
     * Compute the number of cores.
     */
    auto envVar = getenv("NOELLE_CORES");
    if (envVar == nullptr) {
      cores = (std::thread::hardware_concurrency() / 2);
    } else {
      cores = atoi(envVar);
    }
  }

  return cores;
}

uint32_t NoelleRuntime::getAvailableCores(void) {

  /*
   * Get the number of cores available.
   */
  auto numCores = this->NOELLE_idleCores;
  if (numCores < 1) {
    numCores = 1;
  }

  return numCores;
}

NoelleRuntime::~NoelleRuntime(void) {
  delete this->virgil;
}
