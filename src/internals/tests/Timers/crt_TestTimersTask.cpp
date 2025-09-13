// by Marius Versteegen, 2025

#include <crt_TestTimersTask.h>

uint32_t crt_testtimers::TestTimersTask::myUserArg = 42;
volatile uint32_t crt_testtimers::TestTimersTask::myUserArgReceived = 0;
volatile uint64_t crt_testtimers::TestTimersTask::us1 = 0;
volatile uint64_t crt_testtimers::TestTimersTask::diff1 = 0;
volatile uint64_t crt_testtimers::TestTimersTask::us2 = 0;
volatile uint64_t crt_testtimers::TestTimersTask::diff2 = 0;
volatile uint64_t crt_testtimers::TestTimersTask::us3 = 0;
volatile uint64_t crt_testtimers::TestTimersTask::diff3 = 0;
volatile int32_t crt_testtimers::TestTimersTask::myPeriodicTimerCount = 0;
