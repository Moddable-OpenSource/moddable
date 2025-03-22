## psram_allocator

This example detects the PSRAM available on the board, and adds it to an allocator, which manages the *allocation* of the PSRAM. PSRAM is accessed (allocated) using a provided API, which mimics the standard malloc/free functionality.  

A "Two-Level Segregated Fit" (flsf) allocator is used from [here](https://github.com/espressif/tlsf).
