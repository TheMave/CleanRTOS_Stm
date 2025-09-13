// by Marius Versteegen, 2025

// This file contains the code of multiple tasks that run concurrently and notify eachother using flags.
#include "crt_DemoTime.h"

extern "C" {
	// put c includes here
	#include "crt_stm_hal.h"
    #include "main.h"
	#include "cmsis_os2.h"
	#include <inttypes.h>
}

// put c++ includes here
#include <cstdio>
#include "crt_CleanRTOS.h"
#include <crt_Time.h>
#include "crt_PrintTools.h"

using namespace crt;

namespace crt_demoTime
{
	static constexpr auto& print_u64 = PrintTools::print_u64; // short hand fp as alias.


	class DemoTimeTask : public Task
	{

	public:
		DemoTimeTask(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes) :
			Task(taskName, taskPriority, taskSizeBytes)
		{
			start();
		}

	private:
		void main() override
		{
			// Timestamps
			uint64_t s1 = 0;
			uint64_t us1 = 0;
			uint64_t s2 = 0;
			uint64_t us2 = 0;
			uint64_t s3 = 0;
			uint64_t us3 = 0;
			uint64_t us4 = 0;

			osDelay(800); // wait for other threads to have started up as well.
			printf("\r\n----------------\r\n");
			printf("DemoTime_started\r\n");
			printf("----------------\r\n");
			osDelay(100);

			while (true)
			{
				dumpStackHighWaterMarkIfIncreased(); 		// This function call takes about 0.25ms! It should be called while debugging only.

				s1 = Time::getTimeSeconds();
                us1 = Time::getTimeMicroseconds();

                osDelay(1);

                us2 = Time::getTimeMicroseconds();
				s2 = Time::getTimeSeconds();

				osDelay(5000);

				s3 = Time::getTimeSeconds();
				us3 = Time::getTimeMicroseconds();

				us4 = Time::getTimeMicroseconds();

				// print achteraf om de timing van de 1 ms niet te beinvloeden.
				printf("--------------------------------\r\n");

				print_u64("osDelay(1) - measured in us:", us2-us1);
				print_u64("osDelay(1) - measured in sec:", s2-s1);
				print_u64("osDelay(5000) - measured in us:", us3-us2);
				print_u64("osDelay(5000) - measured in sec:", s3-s2);
				print_u64("diff of 2 consecutive us measurements:", us4-us3);

				//printf("--------------------------------\r\n");
                osDelay(1000);
			}
		}
	}; // end class DemoTimeTask

};// end namespace crt_demoTime

extern "C" {
	void demoTime_init()
	{
		// kernal not started yet: printf werkt hier nog niet..
		// Create a "global" logger object withing namespace crt.
		//const unsigned int pinButtonDump = 34; // Pressing a button connected to this pin dumps the latest logs to serial monitor.

		//Logger<100> theLogger("Logger", 2 /*priority*/, ARDUINO_RUNNING_CORE, pinButtonDump);
		//ILogger& logger = theLogger;	// This is the global object. It can be accessed without knowledge of the template parameter of theLogger.

		cleanRTOS_init();
		//static crt::Time stmTime("Time", osPriorityNormal /*priority*/, 1000 /*stackBytes*/);
		static crt_demoTime::DemoTimeTask demoTimeTask   ("DemoTimeTask"   , osPriorityNormal /*priority*/, 1000 /*stackBytes*/);
	}
}
