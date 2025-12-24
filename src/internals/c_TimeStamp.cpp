
#include "crt_Time.h"
#include "c_TimeStamp.h"

uint32_t c_TimeStamp_now(void)
{
    return (uint32_t)crt::Time::getTimeSeconds();
}