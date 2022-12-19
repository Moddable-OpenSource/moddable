# XS Marshalling
Copyright 2021 Moddable Tech, Inc.<BR>
Revised: March 15, 2021

## Introduction

To exchange data between machines that can run in separate threads, XS always supported marshalling thru the **XS in C** programming interface. 

Even if you do not write C code, this document is useful if you write JavaScript code using the standard `Worker` programming interface provided by the [worker](../base/worker.md) module of the Moddable SDK.

## What Is Marshalling?

Similar to what `JSON.stringify` and `JSON.parse` do with a string, *marshalling* creates a memory block from a JavaScript value and *demarshalling* creates a JavaScript value from a memory block. The format of the memory block is binary. 

XS machines can be created from scratch or cloned from a read-only machine. Typically on micro-controllers, multiple small machines can be created in RAM based based on one large read-only machine in ROM.

> For those familiar with Secure ECMAScript (SES), a cloned machine is in a state analogous to the post-lockdown state in SES. In this state, all built-ins are deeply frozen. An XS created from scratch is fully mutable and therefore analogous to the pre-lockdown state in SES.

When two machines are created from scratch or cloned from different read-only machines, they are **alien** and the alien programming interface must be used:

	void* xsMarshallAlien(xsSlot slot);
	xsSlot xsDemarshallAlien(void* data);

> In **mcsim**, the simulator and the app are alien machines. They communicate using the standard `Worker` programming interface, which is implemented here with `xsMarshallAlien` and `xsDemarshallAlien`.

When two machines are cloned from the same read-ony machinee, marshalling can take advantage of what is shared by the two machines and the **full** programming interface can be used:

	void* xsMarshall(xsSlot slot);
	xsSlot xsDemarshall(void* data);

> The **worker** module clones all machines from the same read-only machine. They communicate using the standard `Worker` programming interface, which is implemented here with `xsMarshall` and `xsDemarshall`.

## Alien Marshalling

What can be exchanged between alien machines?

Firstly, everything that can be exchanged thru JSON: 

- `false`
- `null` 
- `true`
- numbers
- strings
- instances of
	- `Object`
	- `Array`

> Usually, the marshalled memory block is smaller that the equivalent JSON string and `xsMarshallAlien`/`xsDemarshallAlien` are faster than `JSON.stringify`/ `JSON.parse`.

XS can also marshall other values not possible using JSON:

- `undefined` 
- bigints
- symbols 
- instances of 
	- `Boolean`, `Error`, `EvalError`, `RangeError`, `ReferenceError`, `SyntaxError`, `TypeError`, `URIError`, `AggregateError`
	- `Number`, `Date`
	- `String`, `RegExp`
	- `BigInt64Array`, `BigUint64Array`, `Float32Array`, `Float64Array`, `Int8Array`, `Int16Array`, `Int32Array`, `Uint8Array`, `Uint8ClampedArray`, `Uint16Array`, `Uint32Array`
	- `Map`, `Set`
	- `ArrayBuffer`, `SharedArrayBuffer`, `DataView`
	- `Proxy`

Furthermore, XS can marshall cyclic references:

##### main.js
	const a = { b: {} }
	a.b.a = a;
	worker.postMessage(a);

##### worker.js
	self.onmessage = function(a) {
		trace(`${ a.b.a === a }\n`); // true
	}

### Symbols Caveat

Symbols are consistent inside a marshalled memory block: 

##### main.js
	const symbol = Symbol();
	const object = { [symbol]: null }
	worker.postMessage({ symbol, object });

##### worker.js
	self.onmessage = function(m) {
		trace(`${m.object[m.symbol]}\n`); // null
	}

However, successive marshallings of the same symbol result in successive demarshalling of different symbols:

##### main.js
	let step = 0;
	const symbol = Symbol();
	worker.postMessage({ step, symbol });
	step++;
	worker.postMessage({ step, symbol });

##### worker.js
	let symbol;
	self.onmessage = function(m) {
		if (m.step == 0)
			symbol = m.symbol;
		else
			trace(`${symbol === m.symbol}`) // false
	}

### No Way

Marshalling is for data. Objects related to code, or to the execution of code, cannot be marshalled: accessors, arguments, classes, functions, generators, modules and promises.

Also, objects related to garbage collection cannot be marshalled: finalization registries, weak references, weak maps and weak sets. And, of course, objects implemented outside XS cannot be marshalled: host objects and host functions.

In these cases, XS tries to report a meaningful error:

##### main.js
	try {
		worker.postMessage([ { p: { get x() {} } } ]);
	}
	catch {
	}

breaks into **xsbug** with 

	main.js (2) # Break: (host): marshall [0].p.x: accessor!

> Historically, XS reported marshalling errors as `marshall: no way!`

## Full Marshalling

What can be exchanged between machines cloned from the same read-only machine?

Firstly, everything that can be marshalled between alien machines. Then XS takes advantage of what is shared in the read-only machine to complete the marshalled instances.

In the following examples, *preload.js* is a module that is preloaded by the XS linker. Its body is executed at link time to define classes and objects in the read-only machine.

### Prototypes

Like what happens with `JSON.stringify` and `JSON.parse`, custom prototypes are lost when instances are marshalled:

##### main.js
	class C {}
	const o = new C();
	trace(`${o.constructor.name}\n`); // C
	worker.postMessage(o);
	
##### worker.js
	self.onmessage = function(m) {
		trace(`${m.constructor.name}\n`); // Object
	}

But if custom prototypes are in the read-only machine, they are kept:

##### preload.js
	export class C {}

##### main.js
	import { C } from "preload";
	const o = new C();
	trace(`${o.constructor.name}\n`); // C
	worker.postMessage(o);
	
##### worker.js
	self.onmessage = function(m) {
		trace(`${m.constructor.name}\n`); // C
	}

### Private Fields

Similarly, private fields are ignored when marshalled.
	
##### main.js
	class C {
		#x;
		constructor(x) {
			this.#x = x;
		}
		toString() {
			return this.#x;
		}
	}
	const o = new C("oops");
	worker.postMessage(o);
	
##### worker.js
	self.onmessage = function(m) {
		trace(`${m.toString()}\n`); // [object Object]
	}

Except when the class is in the read-only machine:

##### preload.js
	export class C {
		#x;
		constructor(x) {
			this.#x = x;
		}
		toString() {
			return this.#x;
		}
	}
	
##### main.js
	import { C } from "preload";
	const o = new C("wow");
	worker.postMessage(o);
	
##### worker.js
	self.onmessage = function(m) {
		trace(`${m.toString()}\n`); // wow
	}

### References

References to instances in the read-only machine are preserved:

##### preload.js
	export const o = Object.freeze({});

##### main.js
	import { o } from "preload";
	worker.postMessage({ o });
	
##### worker.js
	import { o } from "preload";
	self.onmessage = function(m) {
		trace(`${m.o === o}\n`); // true
	}

That is especially useful for exchanging references to objects with methods, like the handler of a proxy. Here is an example inspired by the [MDN Proxy documentation](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Proxy):

##### preload.js
	export const handler = Object.freeze({
	  get: function (target, key, receiver) {
	    if (key === "message2") {
	      return "world";
	    }
	    return Reflect.get(...arguments);
	  }
	});
	
##### main.js
	const target = {
	  message1: "hello",
	  message2: "everyone"
	};
	import { handler } from "preload";
	worker.postMessage(new Proxy(target, handler));
	
##### worker.js
	self.onmessage = function(m) {
		trace(`${m.message1}\n`); // hello
		trace(`${m.message2}\n`); // world
	}


### Exchange Data, Share Code

Marshalling is a way to safely exchange **data** between machines. Prototypes, classes, and proxy handlers in the read-only machine are ways to safely share **code** between cloned machines.
