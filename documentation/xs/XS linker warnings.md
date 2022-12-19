# XS Linker Warnings
Copyright 2019 Moddable Tech, Inc.<BR>
Revised: November 9, 2019

## Preload

The XS linker can preload modules by executing their body at link time. The resulting closures and objects will be in ROM at run time. 

XS supports an aliasing mechanism to allow such closures and objects to be modified at run time. Modified closures and objects are aliased in RAM. 

But the aliasing mechanism has a cost, even if closures and objects are never modified: XS uses one pointer for each aliasable closure and object.

So it is recommended to freeze objects and to use `const` instead of `let` in the body of modules. For details see [Using XS Preload to Optimize Applications](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/xs/preload.md)

But it is common to forget a few closures and objects so the XS linker now warns you about closures and objects that are aliasable.

> The XS linker forces a garbage collection after preloading modules. So closures and objects that are not referenced by modules namespaces are discarded.

## Compartments

XS supports compartments, as defined by Secure ECMAScript. See the [Draft Spec for Standalone SES](https://ses-secure-ecmascript.readthedocs.io/en/latest/draft-standalone-spec.html) for details.

Preloaded modules are shared by all compartments, their bodies are considered as vetted customization code. So the XS linker warnings are especially significant when security matters.

> When the XS linker reports warnings about a module, it means that the module exports mutable closures or objects.

## Objects

Here is a simple `"test"` module which exports one object:

	export const o = { p: 0 };
	
If the `"test"` module is preloaded, the XS linker reports:

	### warning: "test": o: not frozen
	
The fix is:

	export const o = Object.freeze({ p: 0 });

Objects are checked recursively.

	export const o = Object.freeze({ p: { q: 0 } });
	
The XS linker reports:

	### warning: "test": o.p: not frozen

Using the non standard deep freeze feature of XS, the fix is:

	export const o = Object.freeze({ p: { q: 0 } }, true);

## Closures

Here is a simple `"test"` module which exports one function that returns a variable in the module scope:

	let v = 1;
	export function f(a) { 
		return v; 
	}

If the `"test"` module is preloaded, the XS linker reports:

	### warning: "test": f() v: no const

The fix is:

	const v = 1;
	export function f() { 
		return v; 
	}

Closures and objects are checked together. For instance:

	const o = { p: 0 }; 
	export function f() { 
		return o; 
	}
	
The XS linker reports:

	### warning: "test": f() o: not frozen

The fix is:

	const o = Object.freeze({ p: 0 }); 
	export function f() { 
		return o; 
	}
	
## Globals

Preloaded modules can modify the global scope. The resulting object will be in ROM at runtime and will be used to initialize the global scope of the Moddable app (a.k.a. the SES start-compartment). The XS linker also reports warnings about globals.

Here is a simple `"test"` module which adds one object to the global scope:

	globalThis.g = { p: 0 };
	
If the `"test"` module is preloaded, the XS linker reports:

	### warning: globalThis.g: not frozen

The fix is:

	globalThis.g = Object.freeze({ p: 0 });

## Exceptions

Preloaded objects are either aliasable or frozen. Preloaded aliasable objects are mutable. Preloaded frozen objects are immutable, their properties and their private properties are read-only.

Most instances of built-ins classes can be preloaded but cannot be aliased. Once preloaded, such instances are immutable and several built-ins methods throw type errors.

| Instances of | Preload | Alias | Throw Type Error |
|--------------|---------|-------|------------------|
| Array | Yes | Yes | - |
| ArrayBuffer\* | Yes | No | - |
| AsyncGenerator\* | No | No | 
| Boolean | Yes | No | - |
| DataView | Yes | No | set |
| Date | Yes | No | setDate, setFullYear, setHours, setMilliseconds, setMinutes, setMonth, setSeconds, setTime, setYear, setUTCDate, setUTCFullYear, setUTCHours, setUTCMilliseconds, setUTCMinutes, setUTCMonth, setUTCSeconds |
| Error | Yes | No | - |
| FinalizationGroup | Yes | No | cleanupSome, register, unregister |
| Function\* | Yes | No | - |
| Generator\* | No | No | 
| Map | Yes | No | clear, delete, set |
| Number | Yes | No | - |
| Promise | Yes | No | - |
| Proxy | Yes | No | revoke |
| RegExp | Yes | No | 
| Set | Yes | No | add, clear, delete |
| String | Yes | No | - |
| Symbol | Yes | No | - |
| TypedArray\* | Yes | No | copyWithin, fill, reverse, set, sort |
| WeakMap | Yes | No | delete, set |
| WeakRef | Yes | No | - |
| WeakSet | Yes | No | add, delete |

* Instances of ArrayBuffer are mostly accessed thru instances of DataView and TypedArray. Currently the endianness at build time has to be the same as the endianness at run time.
* Since classes are instances of Function, static properties and static private properties of preloaded classes are read-only.
* The XS linker reports errors when instances of AsyncGenerator or Generator are preloaded.
* For preloaded instances of TypedArray, integer-indexed assignment also throws a type error.
