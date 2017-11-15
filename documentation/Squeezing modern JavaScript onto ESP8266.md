## Squeezing modern JavaScript onto ESP8266
#### November 17, 2016
#### Copyright 2016 Moddable Tech, Inc.
#### Confidential

Spoiler:

- 10910 bytes
- 1.5 ms boot
- Vast majority of standard JavaScript 2016

### Project boundaries
Mr. Barrett defined one boundary of the challenge by recommending the ESP8266 as an initial target. At 80 MHz, performance isn't an issue. With only about 44500 free bytes, memory is the challenge. Fortunately, the limited RAM is counterbalanced by 4 MB of flash. Beyond reducing overall memory use, the challenge then becomes migrating as much as possible from RAM to ROM. That's no small detail, as JavaScript is a dynamic language. That means that every object, including built-in functions, can be modified at any time. As one simple example:

	let d = new Date()
	Date.prototype.inOneMinute = function() {
		return this.now() + 60;
	}
	let inAMinute = d.inOneMinute();

The other boundary condition is maintaining compatibility with the JavaScript language specification. XS6, our JavaScript virtual machine, is a complete ES6 (JavaScript 2016) implementation. That's a significant benefit to developers, as it is the same language they know from the web and Node.js (actually, we are a bit ahead of those platforms in adopting new language features). That allows developers to apply their experience without unpleasant frustration of stumbling of subtle language differences. 

### Previous work on JavaScript for embedded
In our work at Marvell on Kinoma Element, we were able to run full XS6 comfortably on a device with 512 KB RAM (about 384 KB free beyond the RTOS). No other virtual machine has matched that with the same level of functionality. To achieve that was a considerable effort:

- Moved the symbol table for built-in and pre-compiled objects into ROM
- Run all byte code from ROM. Note that while byte code remains in ROM, each function creates an instance in RAM to be able to support the dynamic capabilities of the language.
- Keep all pre-defined and pre-compiled strings in ROM
- Optional elimination of two properties from function objects that were seldom use (but required for strict language compatibility)

To store these items in ROM, the JavaScript code is precompiled on a computer as part of the build process. It is then linked into an archive which is transferred to the device's flash. Pre-compilation has a number of benefits:

- Ability to store symbols table, byte code, and strings in ROM
- Option to optimize byte code beyond what would be practical on the target device. For example, a multi-pass compile and code generation is used which allows significant optimization to variable access inside a closure.
- No JavaScript source code stored on the target device
- Faster start-up since no code needs to be compiled when device starts
- Errors in scripts that can be detected by the JavaScript compiler are identified sooner, during the build phase on the computer instead of later when executed on the device.

### First pass at ESP8266
When moving to the ESP8266, the ROM optimizations could not be used as-is. The Xtensa instruction set used in the ESP8266 requires all ROM access to be long-aligned long word reads:

- String are usually read as bytes
- Byte code is usually read as bytes
- The symbol tables is made up of "slots", which are 16 byte JavaScript values that include 8 and 16 bit values.

To get XS6 running on ESP8266, changes were made:

- All standard C string functions (`strlen`, `strcat`, etc) and memory (`memcpy`) replaced with new implementations that read all data as long aligned long words
- Symbol APIs modified to allow safe access to ROM symbol slots
- Interpreter run loop modified to access byte code as long words

That allows a minimal virtual machine to run on the device. However, to make that fit many built-in objects were eliminated to save memory:

* Global functions
	* decodeURI
	* decodeURIComponent
	* encodeURI
	* encodeURIComponent
	* escape
	* unescape

* Objects
	* Date
	* Promise
	* Symbol
	* Proxy
	* Reflect
	* Map
	* WeakMap
	* Set
	* WeakSet
	* DataView

* TypedArray
	* Float32Array
	* Float64Array
	* Int8Array
	* Int16Array
	* Int32Array
	* Uint8Array
	* Uint16Array
	* Uint32Array
	* Uint8ClampedArray

* Difficult
	* RegExp
	* eval

With the exception of `RegExp` and `eval`, these all work on the ESP8266 when enabled.

To free additional memory, further optimizations to the core XS6 virtual machine have been implemented:

- Minimize memory used by globals. Because access to globals is common in JavaScript code, most virtual machines, including XS6, have a special case optimization to access to them. This can take considerable memory. This was reduced from approximately 4 KB to under 500 bytes by reorganizing symbol table id numbers. Performance is the same for built-in globals, and a bit slower for other globals (though due to the use of modules, they are exceedingly rare).
- Minimize memory used by native function instances. The functions on many objects, including all the language built-in objects, are implemented by native code, not byte code. A new function object type was implemented specifically for native function instance, reducing their memory requirements by 4 times.
- Eliminate permanent memory reserved for conversation between floating point values and strings. In most cases, this can be done with memory available on the virtual machine's stack, and otherwise with a temporary memory buffer. This saves about 2400 bytes.

The JavaScript language is moving towards wide of modules to organize projects. This began with Node.js and work is on-going to bring it to the web. To align with this direction and to take advantage of the code modularity, the XS6 runtime on ESP8266 uses modules to package all libraries (HTTP, Socket, I2C, etc). Even the application "main" is a module. Modules are loaded at runtime, like a DLL. XS6 keeps the byte code of the module in ROM, but the module must still be "linked" with the other modules which takes memory, both temporary memory used during linking and some memory that persists while the module is loaded. The module loader was significantly overhauled to reduce both types of memory, reducing the peak memory considerably which keep helps to keep the JavaScript slot heap from growing unnecessarily.

As a result of this work, the XS6 implementation scales well on embedded targets. There is no RAM cost for any of the following:

- Long variable names. This allows developers to use meaningful variable names.
- The total number of symbols
- Modules - unloaded modules use no RAM. Modules can be unloaded and garbage collected when no longer in use.
- Precompiled strings. This allows scripts to display descriptive error messages.
- Code. Byte code is always in ROM, so there can be far more script code than RAM holds.

Applying all of these techniques, starting up the virtual machine consumes approximately 22100 bytes of RAM. On the ESP8266, that leaves something over 22 KB available for scripts (their data and modules instances, since their byte code remains in ROM) Applications loading several modules for network and/or graphics, use another 5 to 15 KB of RAM. By comparison, before work on XS6 for ESP8266 began this summer, the equivalent scenario would have required approximately 60 KB of RAM, and loading several modules would push that over 120 KB of RAM.

Initializing the virtual machine on ESP8266 takes in about 9 milliseconds to be ready to run scripts. Module loading takes some time, depending on the complexity of the module, but typically milliseconds.

In addition, XS6 supports optional source level debugging, which accelerates development. Source level debugging of any language is rare on embedded devices, all the more so for scripting languages. XS6 debugging runs over USB and Wi-Fi. Enabling debugging support in the client consumes about 1400 bytes of RAM, primarily for the communication buffer.

### Second pass at ESP8266
With all of these changes in place, there is sufficient memory available to do useful work including network transactions (Wi-Fi configuration, HTTP, WebSockets, SNTP, DNS, etc), hardware pins (serial, gpio, i2c, spi, etc) and graphics (16-bit color, anti-aliased fonts, fully composited, 30 fps rendering). However, memory is still somewhat tight, the full language isn't available, and developers need to be careful about loading and unloading modules. These are manageable for advanced developers, but are an impediment to broader use of modern JavaScript on embedded devices.

The obvious way to free more RAM and to enable the full suite of built-in JavaScript 2016 objects is to move more virtual machine slots from RAM to ROM. Ignoring the challenge of building slots in ROM and linking to them from objects in RAM, there are two significant problems to solve.

First, the Xtensa instruction set requirement that ROM is always accessed as long-aligned long words conflicts with the structure of slots which contain 8 and 16-bit values. Modifying every access to such structures manually is impractical and challenging to maintain. Most ARM CPUs do not have this restriction, so moving to another platform is a possibility. But, supporting the ESP8266 and ESP32 is a goal, as they offer excellent price/performance together with a large, active developer community. Fortunately, that developer community provides a solution in the form of a patch to GCC code generation for the Xtensa CPU which forces all 8 and 16-bit memory reads to use aligned long-word read instructions. The patch generates bigger, slower code so it is applied only internal XS6 code which is the only place the contents of slots are accessed directly. The patch could be more efficient with some work, but the patch works, which is a small miracle. So, slots can be moved to ROM and safely read from there.

Second, the dynamic nature of JavaScript requires that objects be modifiable throughout their lifetime. Recent version of JavaScript have the option to "freeze" an object which makes it immutable (properties cannot be added, removed, or have their values changed). 

This could be useful, but automatically freezing all objects because they are built into ROM changes the nature of the language, which is not acceptable. A solution was found in the use of an alias table. Objects instances can have their slots in ROM. For each instance in ROM, there is an entry in a global alias table which is used to override the ROM instance with any changes applied by running scripts. This costs a pointer per instance in ROM. Objects that are frozen using `Object.freeze` are not included in the alias table.

With these issues addressed, the remaining problem is how to build the ROM. The virtual machine initializes itself by building the built-in objects directly on a live, running system. As the VM initializes itself, debugging is active and the garbage collector may run. Because the VM changes over time, building the VM image for ROM is not a one-time operation but an operation that needs to be easily repeatable. The solution used is to launch the VM on the build machine (e.g. computer) to initialize itself there. That builds all the slots in RAM. A build tool then decompiles that image to C source code that can be built into the ROM image, with the linker providing necessary address relocation and bindings to native functions. The details of the process are complex, but are hidden from the developer in the build process. Clearly there are limits to what can be initialized when running on the computer: anything related to the device itself (e.g. creating timers, initializing registers, etc) needs to be deferred. This has not proven to be a problem in practice.

Booting the device with a pre-built slot heap has many advantages:

- The virtual machine boots much faster, in just 1.5 ms on the ESP8266.
- The virtual machine can have all built-in objects enabled, providing maximum compatibility with the language
- Modules can be optionally pre-linked during the build. This eliminates the time required to link them, and moves all the module's slots to ROM

When launching the ESP8266 with the pre-built slot heap, the RAM needed to start the virtual machine is reduced to 10910 bytes from 22100 bytes without the pre-built slot heap. At the same time, considerably more language features are available (only `eval` and `RegExp` are unavailable), and the memory and time overhead of loading most modules is eliminated. The majority of the the RAM used, approximately 6 KB, is for the virtual machine's stack for scripts.

With so much moved to ROM, the size of ROM required is a fair question. On the ESP8266, the entire firmware - all the ESP RTOS code, the Arduino core emulation layer, the XS6 implementation, the pre-built slot heap, and all byte code - fits in under 900 KB. That seems reasonable, and opportunities remain to incrementally reduce that.

On devices that, unlike the ESP8266, don't have a full TCP/IP networking stack with Wi-Fi support, it seems potentially realistic to consider running an XS6 based system on hardware with as little as 32 KB RAM.

### Multiple virtual machines
Typically a virtual machine boots when the device starts-up and runs until it powers down. A virtual machine can be instantiated so quickly with relatively little memory, other scenarios become practical, especially on hardware with a little more free memory For example, a product might instantiate a virtual machine with a temporarily purposes such as performing a  certain operation, for example a security task, entirely separate from the main virtual machine. On a multi-core device, such as the ESP32, a virtual machine could be instantiated for each core, allowing them to run entirely in parallel. Or one long running virtual machine could be dedicated to a system tasks, while a separate virtual machine runs the application specific to a particular mode of operation for the device.

### Runtime objects
All of the work to reduce memory use by the virtual machine is of limited value if the runtime objects are not also memory aware. Here "runtime" refers to the JavaScript modules, objects, and classes that provide capabilities beyond what the core language defines, such as networking, graphics, timers, file system, cryptography, etc. The design of runtime objects for the web and for Node.js tends not to take memory consumption into account, certainly not as a top level concern. For XS6 on ESP8266, a new suite of runtime objects has been defined that attempts to minimize runtime memory use as much as practical, while providing developer-friendly APIs.

One example is the network socket API. The implementation takes many measures to eliminate unnecessary buffers. For example, scripts almost always access data directly from the network stack buffers as it is received, rather than the socket implementation copying and combining network buffers. The network protocols, including HTTP and WebSockets,  are implemented in JavaScript using the socket object. Beyond portability, security is the motivation for using script to implement the network protocols. Script code should not suffer catastrophic failure when parsing unexpected data or running out of memory, it should just throw an exception. This closes many common security holes.

Another example is the Commodetto graphics library, which is designed to render full screen user interfaces while only keeping a handful of scan lines (as few as one) in memory at a time. Similarly, the graphics library is able to render image and font assets directly from flash, without having loading them to RAM or spool them through RAM with a file system API. This combination allows real modern user interfaces to be rendered by embedded devices.

### Relocatable memory blocks
A common challenge in embedded development is fragmentation of memory. The ESP8266, like most microcontrollers, lacks an MMU which makes the risk of heap fragmentation very real. One solution to avoid fragmentation is the addition of a memory manager that supports relocatable blocks. That, however, adds further complexity to the system and requires a reserved memory heap for relocatable memory. Another solution is to preallocate non-relocatable memory for peak memory scenarios, but that tends to tie up a lot memory that is unused most of the time.

The XS6 JavaScript virtual machine already has a relocatable memory heap (the chunk heap), which is used to store `ArrayBuffers`, `Strings`, and other variable size data structures used by the language. XS6 has a memory allocator function that allows native objects to allocate relocatable memory in the XS6 chunk heap. This work provides fully relocatable memory to the runtime objects, without the need for another memory manager, while minimizing memory overhead by combining the relocatable heaps for native and JavaScript code.

