// by Marius Versteegen, 2025

#include <cstdio>
extern "C" {
	#include "crt_stm_hal.h"
    #include "main.h"  // bevat vaak GPIO-definities
	#include "cmsis_os2.h"
}

#include "crt_Task.h"
#include "crt_SortedArray.h"
#include "crt_TestSortedArray.h"

// Dit is een soort helloworld test.
// link naar de huart2 instance in main.c
// (it has been put there by the cubeide configurator after activating USART2 there)
extern UART_HandleTypeDef huart2;

using namespace crt;
namespace crt_testSortedArray
{
	class TestSortedArray : public Task
	{

	public:
		TestSortedArray(const char *taskName, osPriority_t taskPriority,
	             uint32_t taskStackSizeBytes)
		: Task(taskName, taskPriority, taskStackSizeBytes)
		{
			start();
		}

		// ---- voorbeeld type ----
		struct Reading {
			uint32_t timestamp;
			int16_t  value;

			friend bool operator<(const Reading& a, const Reading& b) {
				return a.timestamp < b.timestamp;
			}
		};

		static void dump(const SortedArray<Reading, 8>& arr)
		{
			printf("size=%u:\r\n", (unsigned int)arr.size());
			for (unsigned int i = 0; i < (unsigned int)arr.size(); ++i) {
				printf("  [%u] ts=%u val=%d\r\n",
					i,
					(unsigned int)arr[i].timestamp,
					(int)arr[i].value);
			}
		}


		void main() override {
			SortedArray<Reading, 8> readings;

			readings.addItem(Reading{ 100, 10 });
			readings.addItem(Reading{  90,  9 });
			readings.addItem(Reading{ 110, 11 });
			readings.addItem(Reading{ 100, 99 }); // duplicate timestamp; komt vóór bestaande gelijke door lower_bound insert

			dump(readings);

			// search: zoek eerste item met timestamp == 100
			if (const Reading* p = readings.search([](const Reading& r) { return r.timestamp == 100; })) {
				printf("Found ts=100 -> val=%d\n", (int)p->value);
			} else {
				printf("Not found\n");
			}

			// remove: verwijder eerste item met timestamp == 100 (dus degene die search ook vindt)
			bool removed = readings.remove([](const Reading& r) { return r.timestamp == 100; });
			printf("Removed? %s\n", removed ? "yes" : "no");

			dump(readings);
		}

	};
}// end namespace crt_testled

void testSortedArray_init()
{
	static crt_testSortedArray::TestSortedArray testSortedArray("testSortedArray", osPriorityNormal, 2000/*Bytes*/);
}
