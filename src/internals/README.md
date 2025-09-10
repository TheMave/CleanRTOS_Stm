# Internals

This folder contains some tools and helpers that CleanRTOS uses internally / under the hood. You won't need to use them directly, when using CleanRTOS.

**SimpleMutexSection**

An exception is SimpleMutexSection. It's okay to use it in situations that a section is
to be protected with a single mutex only.

**Warning**

Do **not** to use the time(r) related tools directly, as that may  interfere with proper CleanRTOS operations.