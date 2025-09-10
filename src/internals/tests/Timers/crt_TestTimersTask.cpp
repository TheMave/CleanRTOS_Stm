#include <examples/Timers/crt_TestTimersTask.h>

uint32_t crt::TestTimersTask::myUserArg = 42;
volatile uint32_t crt::TestTimersTask::myUserArgReceived = 0;
volatile uint64_t crt::TestTimersTask::us1 = 0;
volatile uint64_t crt::TestTimersTask::diff1 = 0;
volatile uint64_t crt::TestTimersTask::us2 = 0;
volatile uint64_t crt::TestTimersTask::diff2 = 0;
volatile uint64_t crt::TestTimersTask::us3 = 0;
volatile uint64_t crt::TestTimersTask::diff3 = 0;
volatile int32_t crt::TestTimersTask::myPeriodicTimerCount = 0;
