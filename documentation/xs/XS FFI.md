# XS FFI
Copyright 2026 Moddable Tech, Inc.<BR>
Revised: May 1, 2026

## About

**XS Foreign Function Interface** is a mechanism for JavaScript modules to call functions implemented in other languages. Currently XS supports the mechanism for functions implemented in C.

XS always provided a comprehensive C programming interface, [XS in C](./XS in C.md), which allows to implement JavaScript classes, functions and objects in C. That is how the Moddable SDK itself is implemented. 

XS in C requires to understand how XS works (machine, slots, chunks, stack, heaps, garbage collection, etc) in order to represent complex hardware and software features in JavaScript. 

XS FFI enables C developers to define plain C functions and to describe their signatures. Then tools generate efficient glue code to bind such C functions to JavaScript.

XS FFI is of course less expressive than XS in C, but is more convenient for C developers that want to supplement the Moddable SDK without learning a new programming interface.

The Moddable SDK provides FFI for apps (buiilt with **mcconfig**) and, on some platforms, for [mods](./mods.md) (built with **mcrun**). Currently, the mechanism works for apps on all platforms, and for mods on Pebble and, in the simulator, on Linux, macOS and Windows

This document explains how to use XS FFI with the Moddable SDK.

## Caveat

XS and the Moddable SDK are extensively tested (manually and automatically) to find bugs and prevent exploits. Available features should be safe to use, especially in mods executed by a host in a [compartment](./XS Compartment.md).  

Once you define functions in C, you can crash your device or inadvertently provide ways to attack your device. So, as usual, code defensively and test again and again...

## Overview

Let us define a C function that adds two integers:

#### add.c
```
#include "stdint.h"

extern int32_t add(int32_t arg0, int32_t arg1);

int32_t add(int32_t arg0, int32_t arg1)
{
	return arg0 + arg1;
}

```

In the manifest, let us reference its source and describe its signature:

#### manifest.json
```
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

In the main module, let us call the function:

#### main.js
```
import FFI from "mc/ffi";
const test = new FFI;
const result = test.add(1, 2); // 3
debugger
```

In the folder which contains the three files here above, execute:

```
mcconfig -d -m
```

to build and run the app. That will launch the simulator and break in **xsbug** where you can see that `result` is `3` indeed.

#### mc.ffi.c

If you are curious, you can have a look at the generated glue code in

`$MODDABLE/build/tmp/<platform>/mc/debug/<name>/mc.ffi.c`

where `<platform>` is your development platform: `lin`, `mac` or `win` and `<name>` is the name of the folder that contains the three files.

The generated glue code is internal to XS. Do not use similar code elsewhere!

## Examples

In the following sections, we will present supported C types and how they relate to JavaScript values.

All the code snippets are available in `$MODDABLE/examples/ffi` folders:

### ffi-lib

This folder contains the C files and a manifest with the descriptions of the C functions.

That manifest is included by both app and mod examples.

### ffi-app

This folder contains the app example.

To build and run the app, execute:

```
cd $MODDABLE/examples/ffi/ffi-app
mcconfig -d -m
```

### ffi-host

This folder contains the host for the mod example:

To build and run the host, execute:

```
cd $MODDABLE/examples/ffi/ffi-host
mcconfig -d -m
```

That will launch the simulator and traces `no mods` in **xsbug**.

### ffi-mod

This folder contains the mod example. 

Firstly build and launch the host here above, then, to build the mod, execute:

```
cd $MODDABLE/examples/ffi/ffi-mod
mcrun -d -m
```
## Integer and Floating Point Types

In JavaScript, there are two types of primitive numeric values: `Number` and `BigInt`. Internally XS also uses integer values to avoid floating point operations as much as possible.

According to the signature description, the glue code ensures the conversion from JavaScript values to standard C integer and floating point types, and vice versa: 

- signed integers: `int8_t`, `int16_t`, `int32_t`, `int64_t`
- unsigned integers:`uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`
- floating point numbers: `float`, `double`

Here are two C functions that add 32-bit and 64-bit integers: 

```
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

```
	"add32_t": {
		"arguments": [ "int32_t", "int32_t" ],
		"returns": "int32_t"
	},
	"add64_t": {
		"arguments": [ "int64_t", "int64_t" ],
		"returns": "int64_t"
	}
```

Here are their usage:

```
	result = test.add32_t(1111_1111, 2222_2222)
	trace(`${ result }\n`);
	result = test.add64_t(1111_1111_1111_1111n, 2222_2222_2222_2222n)
	trace(`${ result }\n`);
```
> Notice that the 64-bit version requires BigInt values.


## Strings

In JavaScript, strings are primitive values and their contents are read-only. That is especially true with the Moddable SDK where a lot of JavaScript strings are stored in read-only memory. If you try to modify their contents in C, your code will crash.

Internally XS stores strings as UTF-8 encoded C strings. The zero character is escaped by the two bytes sequence `0xC0 0x80`. If your C code returns strings, it is your responsibility to ensure their proper encoding.

### Static Strings

C functions can return a static string:

```
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

The signature of the function must use `const char*`  in the FFI description:

```
	"nameDay": {
		"arguments": [ "uint8_t" ],
		"returns": "const char*"
	}
```

In that case, the glue code will allocate nothing and JavaScript will use the static string itself.

```
	const date = new Date();
	result = test.nameDay(date.getDay());
	trace(`${ result }\n`);
```

### New Strings

C functions can return a new string, created with `malloc`, `calloc` or `strdup`:

```
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

The signature of the function must use `char*`  in the FFI description:

```
	"catenate": {
		"arguments": [ "char*", "char*" ],
		"returns": "char*"
	}
```

In that case, the glue code will allocate memory in the XS heap, then copy the new string into the allocated memory, then delete the new string with `free`.

If the new string cannot be created, the C function must return `NULL` and the glue code will abort with `"not enough memory"`.

```
	result = test.catenate("a", "bc");
	trace(`${ result }\n`);
```

## Buffers

In JavaScript, buffers are instances of `ArrayBuffer` or `SharedArrayBuffer`. Their contents are usually accessed thru views, i.e. instances of `DataView` or `TypedArray`. Several views can share the same buffer with distinct offset and length.

In C, buffers are accessed thru pointers. 

### Blind Pointers

Arguments can be pointers to all integer and floating point types, or `void*`. The glue code will convert ArrayBuffer instances into pointers to their contents. You can pass other arguments for view offset and length.

#### Reading Bytes

Here is a function that sum bytes:

```
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

Here is its signature:

```
	"sumBytes": {
		"arguments": [ "uint8_t*", "uint32_t", "uint32_t" ],
		"returns": "uint64_t"
	}
```

Here is its usage:

```
	result = new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
	result = test.sumBytes(result.buffer, result.byteOffset, result.byteLength);
	trace(`${ result }\n`);
```

#### Writing Bytes

A C function cannot return a new pointer since the buffer size would be unknown. Instead pass a buffer that the C function will fill.

Here is a function that fill bytes with random integers:

```
void fillRandom(uint8_t* buffer, uint32_t offset, uint32_t length)
{
	uint32_t i = 0;
	while (i < length) {
		buffer[offset + i] = (uint8_t)floor(((double)rand() / (double)RAND_MAX) * 255);
		i++;
	}
}
```

Here is its signature:

```
	"fillRandom": {
		"arguments": [ "uint8_t*", "uint32_t", "uint32_t" ],
		"returns": "void"
	}
```

Here is its usage:

```
	result = new Uint8Array(new ArrayBuffer(10), 3, 5);
	test.fillRandom(result.buffer, result.byteOffset, result.byteLength);
	trace(`${ result }\n`);
```

## Arguments

All arguments are required. The glue code will throw if arguments are ommitted.

If you want to provide default arguments, you can easily patch functions in JavaScript:

```
import FFI from "mc/ffi";
const test = new FFI;
const add = test.add;
test.add = function(a, b = 1) {
	return add(a, b);
}
```

The same technique can be used to check arguments ranges and types for instance, or to convert results.

## Programming Patterns

Let us have a sensor that returns samples with two `uint32_t` and one `double`. Here is the C function that fill a buffer with the corresponding structure 

```
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

Here is its signature:

```
	"sampleABCSensor": {
		"arguments": [ "void*" ],
		"returns": "void"
	}
```

Here is its usage:

```
	const view = new DataView(new ArrayBuffer(16));
	test.sampleABCSensor(view.buffer);
	trace(`${ view.getInt32(0, 1, true) }, ${ view.getInt32(4, 1, true) }, ${ view.getFloat64(8, 1, true) }\n`);
```

You can use such a low level function to provide a friendly programming pattern, conformant to the [419 Sensor Class Pattern](https://419.ecma-international.org/#-13-sensor-class-pattern)

```
class ABCSensor {
	constructor() {
		this.view = new DataView(new ArrayBuffer(16));
		this.result = { a:0, b:0, c:0 };
	}
	sample() {
		const { view, result } = this;
		test.sampleABCSensor(view.buffer);
		result.a = view.getInt32(0, 1, true);
		result.b = view.getInt32(4, 1, true);
		result.c = view.getFloat64(8, 1, true);
		return result;
	}
}
```

> The constructor allocates the view and the result, so the sample function does not allocate anything.

Now you can use the class the standard way:

```
const abcSensor = new ABCSensor();
result = abcSensor.sample();
trace(`${ result.a }, ${ result.b }, ${ result.c }\n`);
```


 


