# JavaScript language considerations on embedded devices using the XS engine
Copyright 2018 Moddable Tech, Inc.<BR>
Revised: October 10, 2018

## Ultra-light JavaScript
XS is the JavaScript engine at the core of Moddable applications and tools. XS has existed since the beginning of this century. You can get the latest version on [GitHub](https://github.com/Moddable-OpenSource/moddable).

Other JavaScript engines are primarily used for client or server side web development. Their main focus is speed, their main cost is the significant resources they consume to get that speed. 

The XS engine targets embedded platforms built around microcontrollers like the [ESP8266](https://www.espressif.com/en/products/hardware/esp8266ex/overview) or [ESP32](https://www.espressif.com/en/products/hardware/esp32/overview). XS also runs on the usual desktop and mobile platforms.

The challenges of embedded development are well known: limited memory and limited performance. Compared to hardware that usually runs JavaScript for the web on the client or the server sides, the differences are measured in orders of magnitude, not percentages. Moreover battery life is often critical.

Despite such constraints, and unlike other scripting libraries available on microcontrollers, XS always strives to fully implement the quite extensive [ECMAScript Language Specification](https://tc39.github.io/ecma262/). 

These constraints have consequences. This document highlights key differences between XS, with its focus on embedded devices, and  JavaScript engines focused on web development.

## Spare runtime resources
Embedded platforms do not exist alone. You need a computer to develop applications for them. Therefore, you should prepare in advance of execution, at *compile* time, everything you can in order to spare *run*-time resources. 

On the web, JavaScript engines currently execute scripts and modules from their source code. For microcontrollers, the XS compiler transforms modules source code into byte code on your computer, so the XS engine on the microcontroller only has to execute byte code.

At Moddable, we generalized this approach beyond scripts to all kinds of assets. Fonts, movies, pictures, sounds, and textures are always encoded on your computer into the most practical format for the specific hardware target.

> It is a common to mischaracterize web applications as static or dynamic based on whether its assets are adapted for a specific target device. Using content negotiation, for example, web applications can be as live as you want while still running optimized modules and assets. In this case, the *compile*-time is on the server side, the *run*-time is on the client side, with caches in between.

Despite being based on JavaScript, embedded development with XS is more similar to mobile development than web development. Applications built with XS are described by a manifest and built with a tool chain that includes the XS compiler and linker, asset encoders, C compiler and linker, a debugger, and ROM flasher.

## Prepare the environment
On embedded platforms, the amount of RAM is extremely small, often under 64 KB. The amount of ROM is larger, 512 KB to 4 MB, roughly the same amount of data downloaded for a typical modern web page. An application's native code and data are flashed into ROM. 

JavaScript modules compiled to byte code and encoded assets are stored in flash ROM as part of the native data. This allows the byte code and assets to be used in place, rather than being copied to RAM first. For example, the XS engine executes byte code directly from ROM. Still, a significant amount of RAM is needed for the JavaScript environment your application runs in. For example:

- JavaScript Built-ins including `Object`, `Function`, `Boolean`, `Symbol`, `Error`, `Number`, `Math`, `Date`, `String`, `RegExp`, `Array`, `TypedArray`, `Map`, `Set`, `WeakMap`, `WeakSet`, `ArrayBuffer`, `SharedArrayBuffer`, `DataView`, `Atomics`, `JSON`, `Generator`, `AsyncGenerator`, `AsyncFunction`, `Promise`, `Reflect`, `Proxy`, etc. 
- Modules your application uses to do something useful, like a user interface framework, a secure network framework, etc. 

Constructing built-ins and loading modules creates many classes, functions, and prototype objects. These objects often require more RAM than is present on a modest microcontroller.

To solve this problem, the XS linker allows you to prepare a JavaScript environment on your computer. The XS linker constructs all built-ins and preloads most modules, then saves the result as native data, which is flashed into ROM.

The benefits are significant:

- Since almost nothing is ever copied from ROM to RAM, your application runs with a small amount of RAM.
- Since everything is ready in ROM, your application boots instantaneously. 

> The XS linker cannot preload a module with a body that calls a native function that is only available on the microcontroller. Typically there will be only one module like that to start your application.

## Freeze most objects
But what happens when applications want to modify objects that the XS linker prepared in ROM?

The XS engine maintains a a table of aliases which is initially empty. All aliasable objects in ROM have an index in that table. When an application modifies an aliasable object, the aliasable object is cloned into RAM and inserted into the table to override the aliasable object.

Such a mechanism has a cost in memory footprint and performance, but is essential for conformance with the JavaScript language specification. However JavaScript has a feature to specify that an object cannot be modified: `Object.freeze`. When objects are frozen, the XS linker does not index them as aliasable. 

Modules can use `Object.freeze` in their body to tell the XS linker which objects do not need to be indexed as aliasable. Calling that for each object is tedious enough, so the XS linker can automatically freeze all class, function and prototype objects, as well as other built-in objects like `Atomics`, `JSON`, `Math` and `Reflect`. 

In ECMAScript parlance, that is related to a [frozen realm](https://github.com/tc39/proposal-frozen-realms).

> Freezing most objects is healthy, especially for dynamic applications, since you can be sure that nothing can modify the JavaScript environment.

## Strip unused features
As mentioned above, JavaScript defines many of built-ins, which are all implemented in the XS engine with native code. It is often the case that your application does not use all built-in language features.

Based on the byte code of your modules, the XS linker can strip unused native code from the XS engine itself. So your application will run its own version of the XS engine, tailored to reduce its ROM footprint.

That is automatic for applications that are self-contained and updated as a whole, which is still common on embedded platforms for the sake of consistency, safety, and security. 

For applications that expect to be customized or updated with separate modules, you can manually specify the built-in features to strip from the XS engine and to document the profiled JavaScript environment. For instance you can strip `eval`, `Function`, etc to get rid of the XS parser and byte coder.

> Although there are no profiles in JavaScript, your application can define its own.

## Use native code

That may seem contradictory when talking about a JavaScript engine! But the simplicity of [XS in C](./XS%20in%20C.md), the C programming interface of XS, has always been essential to develop efficient applications.

Web development often claims to be "pure" Javascript while it is in fact relying on the huge amount of native code required to implement web browsers. The reality is of course that web development is restricted to JavaScript on the client side.

At Moddable, we use native code only when necessary, for instance to build drivers, or when the memory footprint or performance gains are obvious, for instance in our graphics library and user interface framework.

Indeed a reasonable solution to sparing resources is to sometimes use native code instead of JavaScript. Remember that your application is in charge of everything. 

> Since the tool chain always requires compiling and linking native code, there is no overhead in your development cycle.

## Conformance
When its environment is not frozen and when its engine is not stripped, XS passes **99.5%** of the tests that are part of the [Official ECMAScript Conformance Test Suite](https://github.com/tc39/test262). The [XS Conformance](./XS%20Conformance.md) document describes the results and test procedures.

Here are a few incompatibilities that you should be aware of:

- **Function**: XS does not store the source code of functions so `Function.prototype.toString` always fails.

- **RegExp**: By default the Unicode property escapes are not built-in because of the size of the tables they require.

- **String**: Strings are UTF-8 encoded, their length is the number of code points instead of the number of code units they contain and they are indexed by code points instead of code units.

- **Tagged Template**: XS supports tagged templates but does not currently implement the tagged templates cache.

No XS hosts are web browsers, so the Annex B of the ECMAScript Language Specification is not supported. However XS implements `Date.prototype.getYear`, `Date.prototype.setYear`, `Object.prototype.__proto__`, `String.prototype.substr`, `escape`, and `unescape`.

The ECMAScript Internationalization API Specification is separate from the ECMAScript Language Specification and is not supported by XS.
