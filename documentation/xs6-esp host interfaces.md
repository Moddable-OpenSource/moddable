# XS6-ESP host interfaces

Copyright 2016 Moddable Tech, Inc.

December 3, 2016 revision

This document defines the interfaces between xs6-esp and the host operating system. This document contains preliminary information and is subject to change.

The XS6-ESP runtime is single threaded. Asynchronous operations (e.g. sockets, SPI output, screen update) occur in the native code with notifications delivered serially to the virtual machine's thread.

> **Note**: The use of "ESP" in this document references to the ESP8266 chip by Espressif. The ESP8266 is used to stress the performance and memory footprint of xs6 on microcontrollers. A future revision of this document will replace the ESP references with a general name.

## Initialization and run loop

The run loop for xs6-esp consists of two phases. The first is initialization, which creates the xs6 virtual machine and executes its `main` module. The second is the run loop, which performs the following:

- Service any timers eligible to run
- Service pending JavaScript promises
- Check for incoming asynchronous requests from xsbug (set breakpoint, clean breakpoint, step, or stop).
- Sleep, if no timer eligible to run

## Standard C Library

xs6-esp expects the fundamentals of Standard C library are available. Functions that access memory should support both RAM and Flash ROM.

#### Memory
- `malloc`
- `calloc`
- `realloc`
- `free`
- `memcpy`

#### Strings

- `strcpy`
- `strncpy`
- `strcat`
- `strncat`
- `strchr`
- `strrchr`
- `strstr`
- `strcmp`
- `strncmp`

#### printf

xs6-esp uses the printf family of functions to format output and to direct output to the console. The printf implementation should support redirection so that its interaction with xsbug debugging can be managed. See "Console log and debugging" section for details.

The following `printf` functions are used by xs6-esp:

- `vprintf`
- `printf`
- `vsnprintf`
- `snprintf`
- `fprintf`

#### Math

xs6-esp relies on standard floating point math libraries to support the floating point math capabilities of ECMAScript. These include:

- `acos`
- `acosh`
- `asin`
- `asinh`
- `atan`
- `atanh`
- `atan2`
- `cbrt`
- `ceil`
- `clz`
- `cos`
- `cosh`
- `exp`
- `expm1`
- `fabs`
- `floor`
- `fmod`
- `fpclassify`
- `hypot`
- `isfinite`
- `isnormal`
- `isnan`
- `llround`
- `log`
- `log1p`
- `log10`
- `log2`
- `pow`
- `rand`
- `round`
- `signbit`
- `sin`
- `sinh`
- `sqrt`
- `srand`
- `tan`
- `tanh`
- `trunc`

## Time

#### `delay`

	void delay(ms);

Sleep for the specified number of milliseconds.

#### `espMilliseconds`

	uint32_t espMilliseconds(void);

A continuous millisecond counter, for example milliseconds since the device powered-up.

#### `espGetTimeOfDay`

	void espGetTimeOfDay(struct espTimeVal *tv, struct espTimeZone *tz)

Returns the current time and/or time zone. Functionally equivalent to the POSIX function `gettimeofday`.

#### `espSetTime`

	void espSetTime(uint32_t seconds)

Set the current time. 

## Network

The xs6-esp package contains an optional `socket` module which implements TCP, UDP, and DNS protocol support using the [lwip](https://en.wikipedia.org/wiki/LwIP) (lightweight IP) network stack. In addition, the debugging implementation supports connecting to xsbug using TCP through lwip.

Only the lowest layer of the lwip API is used, e.g. `tcp_new`, `udp_new`, `tcp_bind`, etc.

## File system

The xs6-esp package contains an optional `files` module which provides access to the host file system. The file system interface assumes a single volume.

The file system may be implemented using various APIs, including [SPIFFS](https://github.com/pellepl/spiffs). The following is a list of the SPIFFS APIs used which may be useful as a guide to the functions required when integrating another host file system.

- `SPIFFS_open`
- `SPIFFS_close`
- `SPIFFS_stat`
- `SPIFFS_fstat`
- `SPIFFS_lseek`
- `SPIFFS_read`
- `SPIFFS_write`
- `SPIFFS_remove`
- `SPIFFS_rename`
- `SPIFFS_opendir`
- `SPIFFS_closedir`
- `SPIFFS_readdir`
- `SPIFFS_info`

## Console log and debugging

The console log and debugging support (xsbug protocol) share a single serial interface. When xsbug debugging is active, console log messages are redirected to xsbug.

The following functions are implemented by the host to support console logging and debugging with xsbug.

#### `ESP_putc`

	void ESP_putc(int c)

Output a single character to the serial output. 

#### `ESP_getc`

	int ESP_getc(void)

Returns a single byte from the serial input. If no byte is available immediately, returns -1.

#### `ESP_isReadable`

	uint8_t ESP_isReadable()

Returns 1 if one or more characters is pending on serial input. Returns 0 if no character is available.

## Display

The screen API assumes a memory mapped frame buffer. It supports both single and double buffer display.

#### `ScreenInit`

	void ScreenInit(int *width, int *height, int *pixelFormat)

Initializes the screen for use. The width and height of the display in pixels are returned. This is the physical dimensions that the display will scan out to the hardware. The pixel format is the depth of the display (1, 2, 4, 8, 16, 24, 32 for color, and 1 + 32, 2 + 32, 4 + 32, and 8 + 32 for gray).

#### `ScreenTerminate`

	void ScreenTerminate()

Shuts down the display, freeing any associated memory.

#### `ScreenFrameBegin`

	void ScreenFrameBegin(int *pixels, int *stride, int x, int y, int width, int height);

Begins the rendering of a new frame. The area to be updated is indicated in pixels with the x, y, width, and height parameters. The pixels must fit within the bounds returned by `ScreenInit`. The coordinate {0, 0} is the top left corner of the display.

The address of the pixel at coordinates {0, 0} is returned in the `pixels` parameter, and the byte length of each scan line (including the pixels and any slop at the end of the line) is returned in the `stride` parameter.

In a display system with two screen buffers, alternate calls to ScreenFrameBegin return alternate buffers.

#### `ScreenFrameEnd`

	void ScreenFrameEnd(void);

Indicates that rendering of the current frame is complete and it is ready for display. No rendering will occur to the screen until the next call to `ScreenFrameBegin`.

#### `ScreenColorTableSet`

	void ScreenColorTableSet(uint16_t *colors, int count);

Sets the color table to apply to the pixels on the next call to `ScreenFrameEnd`. Each color is a 16 bit triple of {r, g, b} values. The count parameter indicates the number of colors which must be less than or equal to the total number of colors available at the screen's bit depth.

## Touch

Begin, move, and end touch events are delivered to native C functions. Multi-touch is supported through the `index` parameter, which begins counting at zero.

The touch driver provides the system time stamp in milliseconds for each event. The accuracy of these time stamps impacts the ability to recognize gestures.

#### `onTouchBegin`

	void onTouchBegin(int index, int x, int y, uint32_t ms)

#### `onTouchMove`

	void onTouchMove(int index, int x, int y, uint32_t ms)

#### `onTouchEnd`

	void onTouchEnd(int index, int x, int y, uint32_t ms)
