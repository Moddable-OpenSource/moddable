## all_allocator

This example detects the PSRAM available on the board, and adds it as well as the built in SRAM based heap to an allocator to provide an unified access to available memory. The allocator manages the *allocation* of the PSRAM and heap SRAM via a single API. The example also ```wraps``` the built in ```malloc``` and ```free``` suite of commands to integrate with existing examples and uses.

A "Two-Level Segregated Fit" (flsf) allocator is used from [here](https://github.com/espressif/tlsf).
