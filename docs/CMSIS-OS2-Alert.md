# CMSIS-OS2-Alert

Look at line 1179 in the CMSIS-OS2 wrapper (FreeRTOS\Source\CMSIS_RTOS_V2\cmsis_os2.c)

```c
if (flags != rflags) {    // ← BUG!
```

It should be:

```c
if ((flags & rflags) != flags) {
```

This is a bug in the STM32-provided CMSIS-OS2 wrapper. The standard reference implementation uses `(flags & rflags) != flags` (bitwise subset check), but this copy uses `flags != rflags` (exact equality). When any other bits (queues, flags) are set alongside the waited bits, it falsely fails.

Below, that error has been fixed:
```c
uint32_t osEventFlagsWait (osEventFlagsId_t ef_id, uint32_t flags, uint32_t options, uint32_t timeout) {
  EventGroupHandle_t hEventGroup = (EventGroupHandle_t)ef_id;
  BaseType_t wait_all;
  BaseType_t exit_clr;
  uint32_t rflags;

  if ((hEventGroup == NULL) || ((flags & EVENT_FLAGS_INVALID_BITS) != 0U)) {
    rflags = (uint32_t)osErrorParameter;
  }
  else if (IS_IRQ()) {
    rflags = (uint32_t)osErrorISR;
  }
  else {
    if (options & osFlagsWaitAll) {
      wait_all = pdTRUE;
    } else {
      wait_all = pdFAIL;
    }

    if (options & osFlagsNoClear) {
      exit_clr = pdFAIL;
    } else {
      exit_clr = pdTRUE;
    }

    rflags = xEventGroupWaitBits (hEventGroup, (EventBits_t)flags, exit_clr, wait_all, (TickType_t)timeout);

    if (options & osFlagsWaitAll) {
      if ((flags & rflags) != flags) {  // error fix on this line!
        if (timeout > 0U) {
          rflags = (uint32_t)osErrorTimeout;
        } else {
          rflags = (uint32_t)osErrorResource;
        }
      }
    }
    else {
      if ((flags & rflags) == 0U) {
        if (timeout > 0U) {
          rflags = (uint32_t)osErrorTimeout;
        } else {
          rflags = (uint32_t)osErrorResource;
        }
      }
    }
  }

  return (rflags);
}
```
