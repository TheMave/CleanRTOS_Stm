// by Marius Versteegen, 2025

// Test-suite for crt::Time class.
#include "crt_TestTime.h"

#include <cstdio>

extern "C" {
	#include "crt_stm_hal.h"
	#include "main.h"
	#include "cmsis_os2.h"
	#include <inttypes.h>
}

#include <crt_CleanRTOS.h>
#include "crt_Logger.h"

using namespace crt;
using namespace crt_logger;

namespace crt_testtime
{
	class TestTime : public Task
	{
	private:
		void printTitle(const char* title)
		{
			printf("--------------------------------------------------\r\n");
			printf("              %s\r\n", title);
			printf("--------------------------------------------------\r\n");
			osDelay(100);
		}

		static void print_u64(const char* label, uint64_t v)
		{
			char buf[32]; char* p = buf + sizeof(buf); *--p = '\0';
			if (v == 0) *--p = '0';
			while (v) { *--p = (char)('0' + (v % 10)); v /= 10; }
			printf("%s: %s\r\n", label, p);
		}

	public:
		TestTime(const char *taskName, osPriority_t taskPriority, uint32_t taskSizeBytes)
		: Task(taskName, taskPriority, taskSizeBytes)
		{
			start();
		}

		// -------- tests -----------------------------------------------------

		// Test basic getTimeMicroseconds functionality
		void test_getTimeMicroseconds_basic()
		{
			printTitle("test_getTimeMicroseconds_basic");
			osDelay(500);

			printf("Calling Time::getTimeMicroseconds() 10 times:\r\n");
			for (int i = 0; i < 10; ++i)
			{
				uint64_t t = Time::getTimeMicroseconds();
				print_u64("  us", t);
				osDelay(100);
			}
			printf("Basic test completed.\r\n");
			osDelay(500);
		}

		// Test that time is monotonically increasing
		void test_monotonic_increase()
		{
			printTitle("test_monotonic_increase");
			osDelay(500);

			uint64_t prev = Time::getTimeMicroseconds();
			bool monotonic = true;
			int failures = 0;

			printf("Testing monotonic increase over 100 iterations:\r\n");
			for (int i = 0; i < 100; ++i)
			{
				uint64_t curr = Time::getTimeMicroseconds();
				if (curr < prev)
				{
					printf("  FAIL: time went backwards! prev=");
					print_u64("", prev);
					printf(" curr=");
					print_u64("", curr);
					monotonic = false;
					failures++;
				}
				prev = curr;
				osDelay(1);
			}

			if (monotonic)
			{
				printf("PASS: Time is monotonically increasing.\r\n");
			}
			else
			{
				printf("FAIL: %d failures detected.\r\n", failures);
			}
			osDelay(500);
		}

		// Test timing accuracy with osDelay
		void test_timing_accuracy()
		{
			printTitle("test_timing_accuracy");
			osDelay(500);

			printf("Testing timing accuracy with osDelay:\r\n");

			// Test 100ms delay
			{
				uint64_t t0 = Time::getTimeMicroseconds();
				osDelay(100);
				uint64_t t1 = Time::getTimeMicroseconds();
				uint64_t elapsed = t1 - t0;
				printf("  osDelay(100) expected ~100000us, measured: ");
				print_u64("", elapsed);
			}

			// Test 500ms delay
			{
				uint64_t t0 = Time::getTimeMicroseconds();
				osDelay(500);
				uint64_t t1 = Time::getTimeMicroseconds();
				uint64_t elapsed = t1 - t0;
				printf("  osDelay(500) expected ~500000us, measured: ");
				print_u64("", elapsed);
			}

			// Test 1000ms delay
			{
				uint64_t t0 = Time::getTimeMicroseconds();
				osDelay(1000);
				uint64_t t1 = Time::getTimeMicroseconds();
				uint64_t elapsed = t1 - t0;
				printf("  osDelay(1000) expected ~1000000us, measured: ");
				print_u64("", elapsed);
			}

			osDelay(500);
		}

		// Test getTimeMilliseconds
		void test_getTimeMilliseconds()
		{
			printTitle("test_getTimeMilliseconds");
			osDelay(500);

			printf("Calling Time::getTimeMilliseconds() 5 times:\r\n");
			for (int i = 0; i < 5; ++i)
			{
				uint64_t t = Time::getTimeMilliseconds();
				print_u64("  ms", t);
				osDelay(200);
			}
			osDelay(500);
		}

		// Test getTimeSeconds
		void test_getTimeSeconds()
		{
			printTitle("test_getTimeSeconds");
			osDelay(500);

			printf("Calling Time::getTimeSeconds() every second:\r\n");
			for (int i = 0; i < 5; ++i)
			{
				uint64_t t = Time::getTimeSeconds();
				print_u64("  sec", t);
				osDelay(1000);
			}
			osDelay(500);
		}

		// Test rapid consecutive calls (stress test for sequence counter)
		void test_rapid_calls()
		{
			printTitle("test_rapid_calls");
			osDelay(500);

			printf("Testing 1000 rapid consecutive calls:\r\n");
			uint64_t t0 = Time::getTimeMicroseconds();

			volatile uint64_t dummy = 0;
			for (int i = 0; i < 1000; ++i)
			{
				dummy = Time::getTimeMicroseconds();
			}

			uint64_t t1 = Time::getTimeMicroseconds();
			printf("  1000 calls completed.\r\n");
			printf("  Total time for 1000 calls: ");
			print_u64("us", t1 - t0);
			printf("  Average per call: ");
			print_u64("ns", ((t1 - t0) * 1000) / 1000);
			printf("  Last value: ");
			print_u64("", dummy);
			osDelay(500);
		}

		// Test consistency between different time functions
		void test_consistency()
		{
			printTitle("test_consistency");
			osDelay(500);

			printf("Testing consistency between us, ms, and seconds:\r\n");

			uint64_t us = Time::getTimeMicroseconds();
			uint64_t ms = Time::getTimeMilliseconds();
			uint64_t s  = Time::getTimeSeconds();

			printf("  Microseconds: ");
			print_u64("", us);
			printf("  Milliseconds: ");
			print_u64("", ms);
			printf("  Seconds: ");
			print_u64("", s);

			// Check rough consistency
			uint64_t ms_from_us = us / 1000;
			uint64_t s_from_us = us / 1000000;

			printf("  ms from us/1000: ");
			print_u64("", ms_from_us);
			printf("  s from us/1000000: ");
			print_u64("", s_from_us);

			if (ms_from_us >= ms - 1 && ms_from_us <= ms + 1)
			{
				printf("  PASS: ms consistency OK\r\n");
			}
			else
			{
				printf("  WARN: ms values differ significantly\r\n");
			}

			if (s_from_us >= s - 1 && s_from_us <= s + 1)
			{
				printf("  PASS: seconds consistency OK\r\n");
			}
			else
			{
				printf("  WARN: seconds values differ significantly\r\n");
			}

			osDelay(500);
		}

		// Test Time under critical section (to check for hangs)
		void test_with_critical_section()
		{
			printTitle("test_with_critical_section");
			osDelay(500);

			printf("Testing Time calls around critical sections:\r\n");

			for (int i = 0; i < 10; ++i)
			{
				uint64_t t0 = Time::getTimeMicroseconds();

				taskENTER_CRITICAL();
				// Do some work in critical section
				volatile uint32_t x = 0;
				for (int j = 0; j < 1000; ++j) x += j;
				taskEXIT_CRITICAL();

				uint64_t t1 = Time::getTimeMicroseconds();

				printf("  Iteration %d, elapsed: ", i);
				print_u64("us", t1 - t0);
			}

			printf("Critical section test completed.\r\n");
			osDelay(500);
		}

		// Test that calling Time from different priority contexts works
		void test_instance_access()
		{
			printTitle("test_instance_access");
			osDelay(500);

			printf("Testing Time::instance() access:\r\n");

			Time* inst = Time::instance();
			if (inst != nullptr)
			{
				printf("  PASS: Time::instance() returned non-null pointer.\r\n");
			}
			else
			{
				printf("  FAIL: Time::instance() returned nullptr!\r\n");
			}

			osDelay(500);
		}

		// Test with LONG critical sections (simulating LoRaWAN-like scenario)
		void test_long_critical_section()
		{
			printTitle("test_long_critical_section");
			osDelay(500);

			printf("Testing Time calls around LONG critical sections:\r\n");
			printf("(Simulating LoRaWAN processing scenario)\r\n");

			for (int i = 0; i < 10; ++i)
			{
				printf("  Iteration %d: ", i);

				uint64_t t0 = Time::getTimeMicroseconds();

				taskENTER_CRITICAL();
				// Simulate longer work in critical section (like LoRaWAN processing)
				volatile uint32_t x = 0;
				for (int j = 0; j < 50000; ++j) x += j;  // Much longer than before
				taskEXIT_CRITICAL();

				uint64_t t1 = Time::getTimeMicroseconds();

				print_u64("elapsed us", t1 - t0);
				osDelay(100);
			}

			printf("Long critical section test completed.\r\n");
			osDelay(500);
		}

		// Test pattern: Time -> critical -> Time (exactly like LoRaWAN test)
		void test_time_critical_time_pattern()
		{
			printTitle("test_time_critical_time_pattern");
			osDelay(500);

			printf("Testing Time->Critical->Time pattern (LoRaWAN pattern):\r\n");

			for (int iteration = 0; iteration < 20; ++iteration)
			{
				printf("  Iter %d: ", iteration);

				// This is exactly the pattern from TestLoraInterface
				uint64_t startTime = Time::getTimeMicroseconds();

				taskENTER_CRITICAL();
				// Simulate work (varying amounts)
				volatile uint32_t x = 0;
				for (int j = 0; j < 10000 * (1 + (iteration % 5)); ++j) x += j;
				taskEXIT_CRITICAL();

				uint64_t endTime = Time::getTimeMicroseconds();
				uint64_t timeSpent = endTime - startTime;

				print_u64("timeSpent us", timeSpent);

				osDelay(10);  // Short delay between iterations (like osDelay(1) in LoRa test)
			}

			printf("Pattern test completed.\r\n");
			osDelay(500);
		}

		// Test repeated rapid Time->Critical->Time without delays
		void test_rapid_critical_pattern()
		{
			printTitle("test_rapid_critical_pattern");
			osDelay(500);

			printf("Testing rapid Time->Critical->Time (no delays between):\r\n");

			uint64_t totalTime = 0;
			int successCount = 0;

			uint64_t testStart = Time::getTimeMicroseconds();

			for (int i = 0; i < 100; ++i)
			{
				uint64_t t0 = Time::getTimeMicroseconds();

				taskENTER_CRITICAL();
				volatile uint32_t x = 0;
				for (int j = 0; j < 5000; ++j) x += j;
				taskEXIT_CRITICAL();

				uint64_t t1 = Time::getTimeMicroseconds();

				if (t1 >= t0)
				{
					totalTime += (t1 - t0);
					successCount++;
				}
				// NO osDelay here - continuous hammering
			}

			uint64_t testEnd = Time::getTimeMicroseconds();

			printf("  Completed %d iterations.\r\n", successCount);
			printf("  Total measured time: ");
			print_u64("us", totalTime);
			printf("  Actual elapsed: ");
			print_u64("us", testEnd - testStart);
			printf("  Average per iteration: ");
			print_u64("us", totalTime / successCount);

			if (successCount == 100)
			{
				printf("  PASS: All iterations completed.\r\n");
			}
			else
			{
				printf("  FAIL: Only %d/100 iterations completed.\r\n", successCount);
			}

			osDelay(500);
		}

		// Test with very long critical section (stress test)
		void test_very_long_critical_section()
		{
			printTitle("test_very_long_critical_section");
			osDelay(500);

			printf("Testing VERY LONG critical section (10ms+ each):\r\n");
			printf("WARNING: This blocks interrupts for extended periods!\r\n");

			for (int i = 0; i < 5; ++i)
			{
				printf("  Iteration %d: ", i);

				uint64_t t0 = Time::getTimeMicroseconds();

				taskENTER_CRITICAL();
				// Very long work - approximately 10ms at 48MHz
				volatile uint32_t x = 0;
				for (uint32_t j = 0; j < 200000; ++j) x += j;
				taskEXIT_CRITICAL();

				uint64_t t1 = Time::getTimeMicroseconds();

				print_u64("elapsed us", t1 - t0);
				osDelay(500);  // Give system time to recover
			}

			printf("Very long critical section test completed.\r\n");
			osDelay(500);
		}

		// Test interleaved with osDelay(1) like LoRaWAN loop
		void test_lorawan_like_loop()
		{
			printTitle("test_lorawan_like_loop");
			osDelay(500);

			printf("Simulating LoRaWAN-like loop pattern:\r\n");
			printf("(Time measurement + critical section + osDelay(1) repeated)\r\n");

			int counter = 0;
			bool success = true;

			for (int i = 0; i < 50 && success; ++i)
			{
				uint64_t startTime = Time::getTimeMicroseconds();

				taskENTER_CRITICAL();
				// Simulate MX_LoRaWAN_Process work
				volatile uint32_t x = 0;
				for (int j = 0; j < 20000; ++j) x += j;
				taskEXIT_CRITICAL();

				uint64_t endTime = Time::getTimeMicroseconds();
				uint64_t timeSpent = endTime - startTime;

				counter++;

				// Print every 10th iteration
				if (i % 10 == 0)
				{
					printf("  Iter %d, timeSpent: ", i);
					print_u64("us", timeSpent);
				}

				// Check for anomalies
				if (endTime < startTime)
				{
					printf("  FAIL at iter %d: time went backwards!\r\n", i);
					success = false;
				}

				osDelay(1);  // Like in the LoRaWAN test
			}

			if (success)
			{
				printf("  PASS: Completed %d iterations successfully.\r\n", counter);
			}

			osDelay(500);
		}

		// Test with LOGGER inside critical section (exactly like LoRaWAN)
		void test_with_logger_in_critical()
		{
			printTitle("test_with_logger_in_critical");
			osDelay(500);

			printf("Testing Time + Logger inside critical section:\r\n");
			printf("(This mimics MX_LoRaWAN_Process with logging)\r\n");

			int counter = 0;
			bool success = true;

			for (int i = 0; i < 50 && success; ++i)
			{
				// Log before (like in TestLoraInterface)
				logger.logStringConst("Call Count = ");
				logger.logInt32(counter);

				uint64_t startTime = Time::getTimeMicroseconds();

				taskENTER_CRITICAL();

				// Simulate MX_LoRaWAN_Process with internal logging
				logger.logStringConst("MX_LoRaWAN_Process: enter");

				volatile uint32_t x = 0;
				for (int j = 0; j < 10000; ++j) x += j;

				logger.logStringConst("LmHandlerProcess: executing");

				for (int j = 0; j < 10000; ++j) x += j;

				logger.logStringConst("MX_LoRaWAN_Process: exit");

				taskEXIT_CRITICAL();

				uint64_t endTime = Time::getTimeMicroseconds();
				uint64_t timeSpent = endTime - startTime;

				logger.logStringConst("Time Spent = ");
				logger.logUint64(timeSpent);

				counter++;

				// Check for anomalies
				if (endTime < startTime)
				{
					printf("  FAIL at iter %d: time went backwards!\r\n", i);
					success = false;
				}

				// Dump every 10th iteration
				if (i % 10 == 0)
				{
					printf("  Iter %d - dumping log:\r\n", i);
					logger.dump();
				}

				osDelay(1);
			}

			// Final dump
			logger.dump();

			if (success)
			{
				printf("  PASS: Completed %d iterations with logger.\r\n", counter);
			}

			osDelay(500);
		}

		// Stress test: rapid Time + Logger + Critical (no delays)
		void test_rapid_time_logger_critical()
		{
			printTitle("test_rapid_time_logger_critical");
			osDelay(500);

			printf("Stress test: rapid Time + Logger + Critical:\r\n");

			int successCount = 0;

			for (int i = 0; i < 100; ++i)
			{
				logger.logStringConst("Iter");
				logger.logInt32(i);

				uint64_t t0 = Time::getTimeMicroseconds();

				taskENTER_CRITICAL();
				logger.logStringConst("In critical");
				volatile uint32_t x = 0;
				for (int j = 0; j < 5000; ++j) x += j;
				logger.logStringConst("Done");
				taskEXIT_CRITICAL();

				uint64_t t1 = Time::getTimeMicroseconds();

				logger.logUint64(t1 - t0);

				if (t1 >= t0)
				{
					successCount++;
				}

				// NO osDelay - continuous hammering
			}

			// Dump accumulated logs
			printf("  Dumping logs...\r\n");
			logger.dump();

			if (successCount == 100)
			{
				printf("  PASS: All 100 iterations completed.\r\n");
			}
			else
			{
				printf("  FAIL: Only %d/100 completed.\r\n", successCount);
			}

			osDelay(500);
		}

		// NEW TEST: Simulates LoRaWAN's PRIMASK-based critical section pattern
		// LoRaWAN uses: uint32_t primask_bit = __get_PRIMASK(); __disable_irq(); ... __set_PRIMASK(primask_bit);
		// This is different from FreeRTOS taskENTER_CRITICAL which uses BASEPRI
		void test_primask_critical_section()
		{
			printTitle("test_primask_critical_section");
			osDelay(500);

			printf("Testing LoRaWAN-style PRIMASK critical sections:\r\n");
			printf("(Uses __get_PRIMASK/__disable_irq/__set_PRIMASK pattern)\r\n");

			bool success = true;

			for (int i = 0; i < 50 && success; ++i)
			{
				uint64_t t0 = Time::getTimeMicroseconds();

				// LoRaWAN-style PRIMASK critical section (UTIL_SEQ_ENTER_CRITICAL_SECTION)
				uint32_t primask_bit = __get_PRIMASK();
				__disable_irq();

				// Simulate work inside PRIMASK-disabled section
				volatile uint32_t x = 0;
				for (int j = 0; j < 10000; ++j) x += j;

				// LoRaWAN-style exit (UTIL_SEQ_EXIT_CRITICAL_SECTION)
				__set_PRIMASK(primask_bit);

				uint64_t t1 = Time::getTimeMicroseconds();

				if (t1 < t0)
				{
					printf("  FAIL at iter %d: time went backwards!\r\n", i);
					success = false;
				}

				if (i % 10 == 0)
				{
					printf("  Iter %d, elapsed: ", i);
					print_u64("us", t1 - t0);
				}

				osDelay(1);
			}

			if (success)
			{
				printf("  PASS: PRIMASK critical section test completed.\r\n");
			}

			osDelay(500);
		}

		// NEW TEST: Nested BASEPRI (FreeRTOS) + PRIMASK (LoRaWAN) critical sections
		// This is EXACTLY what happens in TestLoraInterface:
		// - taskENTER_CRITICAL (BASEPRI) wrapping MX_LoRaWAN_Process
		// - Inside MX_LoRaWAN_Process, UTIL_SEQ uses PRIMASK
		void test_nested_basepri_primask()
		{
			printTitle("test_nested_basepri_primask");
			osDelay(500);

			printf("Testing NESTED BASEPRI + PRIMASK critical sections:\r\n");
			printf("(Outer: FreeRTOS taskENTER_CRITICAL using BASEPRI)\r\n");
			printf("(Inner: LoRaWAN-style PRIMASK manipulation)\r\n");

			bool success = true;

			for (int i = 0; i < 50 && success; ++i)
			{
				logger.logStringConst("Iter");
				logger.logInt32(i);

				uint64_t startTime = Time::getTimeMicroseconds();

				// OUTER: FreeRTOS critical section (uses BASEPRI)
				taskENTER_CRITICAL();

				// INNER: LoRaWAN-style PRIMASK critical section (multiple times, like sequencer)
				for (int k = 0; k < 3; ++k)
				{
					uint32_t primask_bit = __get_PRIMASK();
					__disable_irq();

					// Simulate work
					volatile uint32_t x = 0;
					for (int j = 0; j < 5000; ++j) x += j;

					logger.logStringConst("inner work done");

					__set_PRIMASK(primask_bit);
				}

				taskEXIT_CRITICAL();

				uint64_t endTime = Time::getTimeMicroseconds();
				uint64_t timeSpent = endTime - startTime;

				logger.logStringConst("timeSpent");
				logger.logUint64(timeSpent);

				if (endTime < startTime)
				{
					printf("  FAIL at iter %d: time went backwards!\r\n", i);
					success = false;
				}

				if (i % 10 == 0)
				{
					printf("  Iter %d, elapsed: ", i);
					print_u64("us", timeSpent);
					logger.dump();
				}

				osDelay(1);
			}

			logger.dump();

			if (success)
			{
				printf("  PASS: Nested BASEPRI+PRIMASK test completed.\r\n");
			}

			osDelay(500);
		}

		// NEW TEST: Rapid nested critical sections without delays
		// Stress test for the exact LoRaWAN pattern
		void test_rapid_nested_critical()
		{
			printTitle("test_rapid_nested_critical");
			osDelay(500);

			printf("Stress test: rapid nested BASEPRI+PRIMASK (no delays):\r\n");

			int successCount = 0;
			uint64_t totalTime = 0;

			uint64_t testStart = Time::getTimeMicroseconds();

			for (int i = 0; i < 100; ++i)
			{
				uint64_t t0 = Time::getTimeMicroseconds();

				taskENTER_CRITICAL();

				// Multiple PRIMASK sections inside (like LoRaWAN sequencer tasks)
				for (int k = 0; k < 5; ++k)
				{
					uint32_t primask_bit = __get_PRIMASK();
					__disable_irq();

					volatile uint32_t x = 0;
					for (int j = 0; j < 2000; ++j) x += j;

					__set_PRIMASK(primask_bit);
				}

				taskEXIT_CRITICAL();

				uint64_t t1 = Time::getTimeMicroseconds();

				if (t1 >= t0)
				{
					totalTime += (t1 - t0);
					successCount++;
				}

				// NO osDelay - continuous hammering
			}

			uint64_t testEnd = Time::getTimeMicroseconds();

			printf("  Completed %d iterations.\r\n", successCount);
			printf("  Total measured time: ");
			print_u64("us", totalTime);
			printf("  Actual elapsed: ");
			print_u64("us", testEnd - testStart);
			if (successCount > 0)
			{
				printf("  Average per iteration: ");
				print_u64("us", totalTime / successCount);
			}

			if (successCount == 100)
			{
				printf("  PASS: All 100 iterations completed.\r\n");
			}
			else
			{
				printf("  FAIL: Only %d/100 completed.\r\n", successCount);
			}

			osDelay(500);
		}

		// NEW TEST: Long duration nested critical section
		// Simulates a long MX_LoRaWAN_Process call
		void test_long_nested_critical()
		{
			printTitle("test_long_nested_critical");
			osDelay(500);

			printf("Testing LONG nested BASEPRI+PRIMASK critical section:\r\n");
			printf("(Simulating long MX_LoRaWAN_Process calls)\r\n");

			bool success = true;

			for (int i = 0; i < 10 && success; ++i)
			{
				uint64_t t0 = Time::getTimeMicroseconds();

				taskENTER_CRITICAL();

				// Simulate many sequencer tasks with PRIMASK (like real LoRaWAN)
				for (int k = 0; k < 20; ++k)
				{
					uint32_t primask_bit = __get_PRIMASK();
					__disable_irq();

					volatile uint32_t x = 0;
					for (uint32_t j = 0; j < 10000; ++j) x += j;

					__set_PRIMASK(primask_bit);
				}

				taskEXIT_CRITICAL();

				uint64_t t1 = Time::getTimeMicroseconds();

				if (t1 < t0)
				{
					printf("  FAIL at iter %d: time went backwards!\r\n", i);
					success = false;
				}

				printf("  Iter %d, elapsed: ", i);
				print_u64("us", t1 - t0);

				osDelay(100);  // Give system time to recover
			}

			if (success)
			{
				printf("  PASS: Long nested critical section test completed.\r\n");
			}

			osDelay(500);
		}

	private:
		void main() override
		{
			osDelay(1000); // wait for other threads to have started up

			printf("\r\n");
			printf("==================================================\r\n");
			printf("           crt::Time Test Suite\r\n");
			printf("==================================================\r\n");
			osDelay(500);

			while (true)
			{
				dumpStackHighWaterMarkIfIncreased();
				osDelay(500);

				test_instance_access();
				osDelay(1000);

				test_getTimeMicroseconds_basic();
				osDelay(1000);

				test_monotonic_increase();
				osDelay(1000);

				test_timing_accuracy();
				osDelay(1000);

				test_getTimeMilliseconds();
				osDelay(1000);

				test_getTimeSeconds();
				osDelay(1000);

				test_rapid_calls();
				osDelay(1000);

				test_consistency();
				osDelay(1000);

				test_with_critical_section();
				osDelay(1000);

				test_long_critical_section();
				osDelay(1000);

				test_time_critical_time_pattern();
				osDelay(1000);

				test_rapid_critical_pattern();
				osDelay(1000);

				test_very_long_critical_section();
				osDelay(1000);

				test_lorawan_like_loop();
				osDelay(1000);

				test_with_logger_in_critical();
				osDelay(1000);

				test_rapid_time_logger_critical();
				osDelay(1000);

				// NEW: PRIMASK-based tests (simulating LoRaWAN patterns)
				test_primask_critical_section();
				osDelay(1000);

				test_nested_basepri_primask();
				osDelay(1000);

				test_rapid_nested_critical();
				osDelay(1000);

				test_long_nested_critical();
				osDelay(1000);

				printf("\r\n");
				printf("==================================================\r\n");
				printf("           All tests completed. Repeating...\r\n");
				printf("==================================================\r\n");
				osDelay(5000);
			}
		}
	}; // end class TestTime
}; // end namespace crt_testtime


extern "C" {
	void testTime_init()
	{
		crt::cleanRTOS_init();
		static crt_testtime::TestTime testTime("TestTime", osPriorityNormal, 2000);
	}
}
