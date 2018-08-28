# Embedded JavaScript Development with XS6
November 8, 2016 2:20 PM

## Introduction
Using JavaScript to program embedded devices brings the convenience and expressiveness of JavaScript with the classic challenges of embedded development: limited memory and limited performance.

Thanks to XS6 the JavaScript language on embedded is nearly identical to the language used in Node.js and a web pages. The hardware that runs JavaScript code, however, is completely different. Instead of a multi-core, multi-GHz device with gigabytes of memory, there is micro-controller running at around 100 MHz with tens of KB free memory. Those differences are measured in orders of magnitude, not percentages.

To write effective JavaScript code for embedded means taking a fresh look at JavaScript programming and thinking like an embedded programmer, always alert to performance issues and memory use. At the same time, stay consistent with the spirit of modern JaaScript development by creating clean, clear, elegant, modular scripts. Be prepared to learn discover new techniques and to leave some tried-and-true libraries behind.

## Pre-compilation
One of the key techniques XS6 uses to operate efficiently in memory constrained environments is to compile JavaScript to byte code on a computer. The resulting byte code is downloaded to the target device together with the rest of the firmware. There are many advantages to this approach:

- Faster start-up, since no time is required to parse and compile scripts
- Lower memory use because the following are stored in Flash ROM instead of RAM:
 - JavaScript byte code
 - Static string values
 - Symbol table
 - Debugging information
- Errors detected by compiler on computer before transferring code to target device.
- Source code to scripts not stored on the target device.

Because of pre-compilation, there is no RAM cost to providing detailed error message strings and to using expressive variable names.

## ECMAScript 2016 (ES6)
XS6 implements the latest JavaScript standard. The 2015/2016 revision of the language includes many new features, quite a few of which benefit embedded development.

XS6 for ESP makes use of many new features of the language, including modules and arrow functions. Take time to become familiar with the new language features. Mozilla publishes an excellent series of articles introducing ES6 called [ES6 In Depth](https://hacks.mozilla.org/category/es6-in-depth/).

## Memory
This section describes techniques for reducing memory use in scripts.

Memory in JavaScript works different from native code. In JavaScript, there are two kinds of memory: slots and chunks. Slots are 16-byte memory blocks that store small values including integers, floating point numbers, and booleans, as well as references (pointers) to larger variable sized values that include strings and arrays, which are stored in chunks. The means that each property of an object requires at least 16 bytes.

### Measure
The best way to measure memory use is to enable memory reporting in XS6. Because XS6 maintains its own memory heaps, using the operating system memory measurements is often not very useful.

To enable memory reporting in XS6, modify xs6Platform.h so that mxReport is defined by changing

	//#define mxReport 1

to

	#define mxReport 1

Then do a **clean** rebuild of XS6. Thereafter, whenever XS6 runs the garbage collector on the slot and/or chunk heaps, the total heap sizes and free space are output to the console or, when active, xsbug.

### new
Each time your script invokes `new` it is allocating memory. There's nothing wrong with creating new instances. They are needed. But, the more new objects created, the less free memory is available and/or the more often the garbage collector needs to work. Once you become conscious of memory use in JavaScript, it is often possible to design APIs that minimize memory allocations or re-use existing objects.

### Array
The JavaScript language specification more-or-less requires Arrays to be implemented as linked lists. That's because in JavaScript arrays are sparse. To verify this, the language conformance suites do things like this:

	let a = new Array();
	a[2147418112] = 1;

XS6 optimizes arrays that are continuous (not sparse) using an array rather than a linked list. Use `fill` to allocate a continuous array.

	let a = (new Array(128)).fill(0, 0, 128);

Scripts often fill in the values of an array by pushing elements on the end:

	let a = new Array();
	for (i = 0; i < 64; i++)
		a.push(i);

Rewriting that using a preallocated array is more efficient, both in time and memory:

	let a = (new Array(64)).fill(0, 0, 64);
	for (i = 0; i < 64; i++)
		a[i] = i;

If the array is small (under 10 elements), there's likely no need to bother with preallocation.

### Symbols
Any symbol created during the executing of a script is stored in RAM. Symbols defined at compile time of a script are stored in ROM. Symbols are never deallocated, so once allocated they consume memory until the virtual machine exits.

It is easy to create a symbol at runtime:

	headers["content-length"] = 12;

If "content-length" wasn't already in the symbol table, it is added.

There is nothing inherently wrong in creating symbols at runtime. Because there is a memory cost, care is needed. There is a limit to the number of new symbols that may be created. The number of runtime symbols that may be created is fixed when XS6 is compiled. By default it is just 32, and can be increased by rebuilding XS6.

### JSON
Once place where new symbols are likely to appear is in parsing JSON. Consider the following:

	let json = "{temperature: 93, zipcode: "94025", weatherserverversion: 6.2}";
	let obj = JSON.parse(json);
	trace(`Temperature ${obj.temperature} at ZIP code ${json.zipcode}\n`);

The symbols `temperature` and `zipcode` are defined at compile time because they are used by the script in the `trace` statement. The symbol `weatherserverversion` is not in the symbol table built at compile time (since it only appears in a string literal), so it will be create at runtime.

A workaround for this problem is declare any symbols expected in JSON that aren't accessed by script code. For example:

	function unused() {
		let weatherserverversion;
	}

The function `unused` does not need to be called, just compiled, to add `weatherserverversion` to the built-in symbol table.

### Use scopes
Using the "let" statement to declare local variables minimizes their scope, so that they are not active for the entire execution of the function. Once a local variable goes out of scope, any object it references is eligible for garbage collection (if it is not referenced elsewhere).

	function example() {
		if (x == 12) {
			let data = file.read(ArrayBuffer, 256);
			// ...
			// data cannot be garbage collected here
		}
		// data can be garbage collected here
		// ...
	}

### delete unused properties
Each property of an object uses a minimum of 16 bytes. Sometimes a property is only used during part of the object's life-cycle. Use the `delete` statement to remove a property when it is no longer needed:

	delete this.propertyNoLongerNeeded;

Use xsbug, the XS6 debugger, to view the properties of objects at different times during executing to see if there are any properties which could be removed.

## Speed
When running on a relatively slow embedded device target, performance is often a concern. 

Don't optimize too early. Often straightforward scripts perform well enough. Learn where the performance are by running and measuring the code, not by guesswork. Only optimize code that is performance critical - that will focus your efforts. Once optimized, code tends to be less readable and less maintainable.

### Measure
The best way to know if an optimization is worthwhile is to measure the result. The `Date.now` function is the most efficient way to do that, as unlike creating a new Date instance, it allocates no new memory:

	let start = Date.now();

	// run some performance critical code

	let duration = Date.now() - start;
	trace(`Operation took ${duration} ms.\n`);

### Integers
The JavaScript language contains only floating point numbers. There is no integer type. When the XS6 implementation internally detects a Number value is an integer, it stores it as an integer and operates on it as an integer where possible. This can help with performance. Where possible, use integer values.

To convert an floating point value to an integer, you can use the `Math` object:

	let one = Math.truncate(1.6);
	let two = Math.round(1.6)

A faster, but less readable, way to truncate a floating point value to an integer is to use the logical OR operator:

	let one = 1.6 | 0;

Similarly, using the shift operators to multiple and divide by powers of two is faster than multiply and divide operators and guarantees an integer result:

	let two = 16 >> 3;
	let four = 1 << 2;

These integer math tricks are useful, but should be used sparingly, only in code where performance truly matters and where their use will be well tested.

### Array iteration
When an Array is stored as a linked list, iterating over the array becomes slow because the number of list elements to traverse increases with each element:

	for (let i = 0; i < a.length; i++)
		a[i] = i * i;

The `for... of` statement provides a way iterate over an array that avoids redundant traversal of the linked-list. This approach may perform better for longer arrays:

	for (let [i, value] of a)
		a[i] = i * i;

	for (let value of a)
		trace(`value ${value}`);

Whether the `for... of` statement or a simple for loop performs better depends on several factors. Use a simple loop unless the array is long, and then measure the performance difference to find the best approach.

### trace
The `trace` statement outputs messages to the console, which is useful for debugging. The trace statement takes time, particularly when running with xsbug (over serial or network connections). When assessing the performance of your code, disable the trace statements in performance critical regions. 

### Cache property look-ups
Because of the dynamic nature of JavaScript, the compiler cannot do as much optimization for you as a C compiler. 

For example, the following code looks up the `length` property from array on each loop iteration:

	for (let i = 0; i < array.length; i++)
		;
	
The following code caches `array.length` in the local variable `length`, which does not require a look-up to access. This code will be faster for values of `length` greater than 2 or 3.

	for (let i = 0, length = array.length; i < length; i++)
		;

Don't worry about it if `length` is small or code isn't performance critical.

Similarly, code like this is common:

	this.library.doThis();
	this.library.doThat();
	this.library.begin();
	this.library.end();

Where performance matters, this is better written as follows to save redundant property look-ups and reduce byte code size.

	let library = this.library;
	library.doThis();
	library.doThat();
	library.begin();
	library.end();

### Native code
JavaScript is a powerful tool, but it will never outperform good native code. Sometimes the best solution is to write a few functions in C. The "XS in C" API allows any function of a JavaScript object to be implemented in C.

The real challenge is determining when to use a native function. As a rule, start out in script. If the script doesn't perform adequately, optimize the script. If that doesn't achieve the needed performance, switch to C.

<!--

### Cloned machine

Use Map to keep list of items with arbitrary keys (HTTP headers)

Use TypedArray (Uint8Array is more compact and potentially faster)

-->
