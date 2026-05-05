# XS FFI
Copyright 2026 Moddable Tech, Inc.<BR>
Revised: May 4, 2026

## About

**XS Foreign Function Interface** (FFI) is a mechanism for JavaScript modules to call functions implemented in other languages. Currently XS supports the mechanism for functions implemented in C.

XS has always provided a comprehensive C programming interface, [XS in C](./XS in C.md), for implementing JavaScript classes, functions, and objects in C. That is how the Moddable SDK itself is implemented. 

Using XS in C requires understanding how XS works (machine, slots, chunks, stack, heaps, garbage collection, etc.) in order to represent complex hardware and software features in JavaScript.

XS FFI enables C developers to define plain C functions by describing their signatures. Tools then generate efficient glue code to bind the C functions to JavaScript.

XS FFI is less expressive than XS in C, but is more convenient for C developers that want to supplement the Moddable SDK without learning a new programming interface.

The Moddable SDK provides FFI for apps (built with **mcconfig**) and, on some platforms, for [mods](./mods.md) (built with **mcrun**). Currently, the mechanism works for apps on all platforms, and for mods on Pebble and, in the simulator, on Linux, macOS and Windows

This document explains how to use XS FFI with the Moddable SDK.

## Caveat

XS and the Moddable SDK are tested extensively tested (manually and automatically) to find bugs and prevent exploits. The goal is that features should be safe to use, especially in mods executed by a host in a [compartment](./XS Compartment.md).  

By implementing functions in C, you can crash your device or inadvertently provide ways to attack your device. So, as usual, code defensively and test, again and again...

## Overview

Let us define a C function that adds two integers:

#### add.c

```c
#include "stdint.h"

extern int32_t add(int32_t arg0, int32_t arg1);

int32_t add(int32_t arg0, int32_t arg1)
{
	return arg0 + arg1;
}

```

The manifest reference its source code and describe its signature:

#### manifest.json

```json
{
	"ffi": {
		"sources": [
			"./add.c"
		],
		"functions": {
			"add": {
				"arguments": [ "int32_t", "int32_t" ],
				"returns": "int32_t"
			}
		}
	},
	"include": [ "$(MODDABLE)/examples/manifest_base.json" ],
	"modules": { "*": "./main" }
}
```

And finally, call the function from the main module.

#### main.js

```javascript
import FFI from "mc/ffi";

const test = new FFI;
const result = test.add(1, 2); // 3
debugger
```

In the directory which contains the three files here above, execute:

```shell
mcconfig -d -m
```

to build and run the app. That launches the simulator and breaks in **xsbug** where you can see that `result` is `3` indeed.

#### mc.ffi.c

If you are curious, have a look at the generated glue code in

`$MODDABLE/build/tmp/<platform>/mc/debug/<name>/mc.ffi.c`

where `<platform>` is your development platform: `lin`, `mac` or `win` and `<name>` is the name of the directory that contains the three files.

The generated glue code is internal to XS. Do not use similar code elsewhere!

## Examples

In the following sections, we will present supported C types and how they relate to JavaScript values.

All the code snippets in this document are available in `$MODDABLE/examples/ffi` directories:

### ffi-lib

This directory contains the C files and a manifest with the descriptions of the C functions. That manifest is included by both app and mod examples.

This directory also contains a module to test the C functions, which is used by both app and mod examples.

### ffi-app

This directory contains the app example.

To build and run the app, execute:

```shel
cd $MODDABLE/examples/ffi/ffi-app
mcconfig -d -m
```

### ffi-host

This directory contains the host for the mod example:

To build and run the host, execute:

```shell
cd $MODDABLE/examples/ffi/ffi-host
mcconfig -d -m
```

That launches the simulator and traces `No mod installed` in **xsbug**.

### ffi-mod

This directory contains the mod example. 

First, build and launch the host here above, then, to build the mod, execute:

```shell
cd $MODDABLE/examples/ffi/ffi-mod
mcrun -d -m
```

## Integer and Floating Point Types

In JavaScript, there are two types of primitive numeric values: `Number` and `BigInt`. Internally XS also uses integer values to avoid floating point operations as much as possible.

The glue code uses the function signature to convert from JavaScript values to standard C integer and floating point types, and vice versa: 

- signed integers: `int8_t`, `int16_t`, `int32_t`, `int64_t`
- unsigned integers:`uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`
- floating point numbers: `float`, `double`

Here are two C functions that add 32-bit and 64-bit integers: 

```c
int32_t add32_t(int32_t arg0, int32_t arg1)
{
	return arg0 + arg1;
}

int64_t add64_t(int64_t arg0, int64_t arg1)
{
	return arg0 + arg1;
}
```

Here are their description:

```jsonc
"add32_t": {
	"arguments": [ "int32_t", "int32_t" ],
	"returns": "int32_t"
},
"add64_t": {
	"arguments": [ "int64_t", "int64_t" ],
	"returns": "int64_t"
}
```

Here are their use from JavaScript:

```javascript
result = test.add32_t(1111_1111, 2222_2222)
trace(`add32_t ${ result }\n`);
result = test.add64_t(1111_1111_1111_1111n, 2222_2222_2222_2222n)
trace(`add64_t ${ result }\n`);
```
> Notice that the 64-bit version requires BigInt values.


## Strings

In JavaScript, strings are primitive values and their contents are read-only. That is especially true with the Moddable SDK where many JavaScript strings are stored in read-only memory. If you try to modify their contents in C, your code will crash.

Internally XS stores strings as UTF-8 encoded C strings. The zero character is escaped by the two bytes sequence `0xC0 0x80`. If your C code returns strings, it is your responsibility to ensure their proper encoding.

### Static Strings

C functions can return a static string:

```c
const char* days[7] = {
	"dimanche",
	"lundi",
	"mardi",
	"mercredi",
	"jeudi",
	"vendredi",
	"samedi",
};

const char* nameDay(uint8_t i)
{
	return days[i];
}
```

The function description must return `const char*`:

```jsonc
"nameDay": {
	"arguments": [ "uint8_t" ],
	"returns": "const char*"
}
```

In that case, the glue code does no allocation and JavaScript uses the static string itself.

```javascript
const date = new Date();
result = test.nameDay(date.getDay());
trace(`nameDay ${ result }\n`);
```

### New Strings

C functions can return a new string, created with `malloc`, `calloc` or `strdup`:

```c
char* catenate(char* arg0, char* arg1)
{
	size_t len0 = strlen(arg0);
	size_t len1 = strlen(arg1);
	char* result = malloc(len0 + len1 + 1);
	if (result) {
		memcpy(result, arg0, len0);
		memcpy(result + len0, arg1, len1);
		result[len0 + len1] = 0;
	}
	return result;
}
```

The function description must return `char*`:

```jsonc
"catenate": {
	"arguments": [ "char*", "char*" ],
	"returns": "char*"
}
```

In that case, the glue code will allocate memory in the XS heap, then copy the new string into the allocated memory, then delete the new string with `free`.

If the new string cannot be created, the C function must return `NULL` and the glue code will abort with `"not enough memory"`.

```javascript
result = test.catenate("a", "bc");
trace(`catenate ${ result }\n`);
```

## Buffers

In JavaScript, buffers are instances of `ArrayBuffer`. Their data are usually accessed through views, i.e. instances of `DataView` or `TypedArray`. Several views can share the same buffer with distinct offset and length. Buffers can also be detached, i.e. without data.

In C, buffers are accessed through pointers. The glue code  converts buffers into pointers to their data. 

### Blind Pointers

If the buffer is detached, the pointer is `NULL`. There are no guarantees on the data size.

You can of course check all that in JavaScript, see [Arguments](#arguments) here under.

#### Reading Bytes

Arguments can be pointers to all integer and floating point types, or `void*`. You can pass other arguments for view offset and length.

Here is a function that sums bytes:

```c
uint64_t sumBytes(uint8_t* buffer, uint32_t offset, uint32_t length)
{
	uint64_t result = 0;
	buffer += offset;
	uint8_t* limit = buffer + length;
	while (buffer < limit) {
		result += *buffer++;
	}
	return result;
}
```

Here is its description:

```jsonc
"sumBytes": {
	"arguments": [ "uint8_t*", "uint32_t", "uint32_t" ],
	"returns": "uint64_t"
}
```

Here is its usage:

```javascript
result = new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
result = test.sumBytes(result.buffer, result.byteOffset, result.byteLength);
trace(`sumBytes ${ result }\n`);
```

#### Writing Bytes

A C function cannot return a new pointer since its data size would be unknown. Instead pass a buffer that the C function will fill.

Here is a function that fills bytes with random integers:

```c
void fillRandom(uint8_t* buffer, uint32_t offset, uint32_t length)
{
	uint32_t i = 0;
	while (i < length) {
		buffer[offset + i] = (uint8_t)floor(((double)rand() / (double)RAND_MAX) * 255);
		i++;
	}
}
```

Here is its description:

```jsonc
"fillRandom": {
	"arguments": [ "uint8_t*", "uint32_t", "uint32_t" ],
	"returns": "void"
}
```

Here is its usage:

```javascript
result = new Uint8Array(new ArrayBuffer(10), 3, 5);
test.fillRandom(result.buffer, result.byteOffset, result.byteLength);
trace(`fillRandom ${ result }\n`);
```

<a id="arguments"></a>

### Pointers with Size

Descriptions can also use integer and floating point types followed by a positive integer between brackets. For instance `uint32_t[3]`.

- For arguments, the glue code will check that the buffer is not detached and that its data size is sufficient. 

- When the function description returns a pointer with size, the C function must return a pointer to new data allocated with `malloc`, `calloc`. The glue code will creare a new buffer with the new data, then delete the new data with `free`. If the new data cannot be created, the C function must return `NULL` and the glue code will abort with `"not enough memory"`.

Here is a function that returns a new buffer with incremented values:

```
uint32_t* newTriple(uint32_t* buffer, uint32_t delta)
{
	uint32_t* result = malloc(3 * sizeof(uint32_t));
	if (result) {
		result[0] = buffer[0] + delta;
		result[1] = buffer[1] + delta;
		result[2] = buffer[2] + delta;
	}
	return result;
}
```

Here is its description:

```
	"newTriple": {
		"arguments": [ "uint32_t[3]", "uint32_t" ],
		"returns": "uint32_t[3]"
	}
```

Here is its usage:

```
	result = new Uint32Array([0, 1, 2]);
	result = test.newTriple(result.buffer, 3);
	trace(`newTriple ${ new Uint32Array(result) }\n`);
```

> Notice that the signature of the C function itself still uses `uint32_t*`. 

## Arguments

All arguments are required. The glue code throw if any argument is omitted.

If you want to provide default arguments, you can easily patch functions in JavaScript:

```javascript
const add32_t = test.add32_t;
test.add32_t = function(a, b = 1) {
	return add32_t(a, b);
}
result = test.add32_t(4)
trace(`add32_t ${ result }\n`);
```

The same technique can be used to check arguments ranges and types. For instance:

```javascript
const fillRandom = test.fillRandom;
test.fillRandom = function(buffer, offset, length) {
	if (buffer.byteLength < offset + length)
		throw new RangeError("invalid buffer");
	return fillRandom(buffer, offset, length);
}
try {
	test.fillRandom(new ArrayBuffer(0), 0, 1);
}
catch (e) {
	trace(`fillRandom ${ e }\n`);
}
```

## Programming Patterns

Consider a sensor implemented in C that provides samples that consist of two `uint32_t` values and one `double`. Here is the C function that fill a buffer with the corresponding structure .

```c
typedef struct {
	uint32_t a;
	uint32_t b;
	double c;
} ABC;

void sampleABCSensor(void* ptr)
{
	ABC* abc = ptr;
	abc->a = 1;
	abc->b = 2;
	abc->c = 3.4;
}
```

Here is its description:

```jsonc
"sampleABCSensor": {
	"arguments": [ "void*" ],
	"returns": "void"
}
```

Here is its use in JavaScript:

```javascript
const view = new DataView(new ArrayBuffer(16));
test.sampleABCSensor(view.buffer);
trace(`${ view.getInt32(0, 1, true) }, ${ view.getInt32(4, 1, true) }, ${ view.getFloat64(8, 1, true) }\n`);
```

You can use such a low level function to implement a friendly programming pattern, conformant to the [ECMA-419 Sensor Class Pattern](https://419.ecma-international.org/#-13-sensor-class-pattern)

```javascript
class ABCSensor {
	#view;

	constructor() {
		this.#view = new DataView(new ArrayBuffer(16));
	}
	sample() {
		const view = this.#view;
		test.sampleABCSensor(view.buffer);
		return {
			a: view.getInt32(0, 1, true),
			b: view.getInt32(4, 1, true),
			c: view.getFloat64(8, 1, true),
		};
	}
}
```

> The `ABCSensor` class creates its buffer and view in its constructor to avoid allocations each time its `sample` method is called.

Now you can use the class the standard way:

```javascript
const abcSensor = new ABCSensor();
result = abcSensor.sample();
trace(`${ result.a }, ${ result.b }, ${ result.c }\n`);
```


 


