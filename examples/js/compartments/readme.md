# Compartment Examples
#### Copyright 2019 Moddable Tech, Inc.

XS implements the following constructor to create compartments:

	function Compartment(from, endowments, modules, options)

- Execution in the compartment begins with the module specified by the `from` parameter. The `from` parameter is required.
- `endowments` is an object whose properties are added to the global object of the new compartment. The `endowments` parameter is optional and defaults to `{}`. 
- `modules ` is an object whose properties define the white list of modules that the compartment can import. The `modules` parameter is optional and defaults to `Compartment.map`. 
- `options` is an object with conventional properties to customize the compartment. That parameter is optional and defaults to `{ type:"module" }`.

> The `Compartment` constructor only works in the Moddable runtime when the XS linker has prepared a frozen realm to be shared across compartments. For instance the `Compartment` constructor throws an exception in the runtime XS uses to pass test262 cases.

Instances of `Compartment` have two read-only properties: 

- `global` -- the global object of the compartment
- `export` -- the module namespace of the compartment

> Compartments could be scripts with a `type:"program"` in the `options` argument. In that case, the `export` property is replaced by a `return` property that contains the result of the program. That option cannot be tested currently since XS does not support programs in the Moddable runtime.

## Globals

The Globals examples show that compartments have separate global scopes but can share features thru endowments.

### Separate Globals
	
#### mod.js

The compartment module defines a global variable `x`, a global function `increment` that increments the global variable, and a global function `test` to trace the result.

	globalThis.x = 0;
	globalThis.increment = function() {
		return x++;
	}
	globalThis.test = function() {
		trace("mod " + increment() + "\n");
	}

#### app.js

The main module defines the same global variable and functions:

	globalThis.x = 0;
	globalThis.increment = function() {
		return x++;
	}
	globalThis.test = function() {
		trace("app " + increment() + "\n");
	}
	
Then creates a compartment with `mod.js` here above.
	
	let mod = new Compartment("mod");
	
Then executes:

	test();
	mod.global.test();
	test();
	mod.global.test();

To trace into xsbug:

	app 0
	mod 0
	app 1
	mod 1

### Shared Globals

#### mod.js

The compartment module only defines a global function `test` to trace the result.

	globalThis.test = function() {
		trace("mod " + increment() + "\n");
	}
	
`increment` is a global variable to be passed as an endowment.

#### app.js

The main module defines a global variable `x`, a global function `increment` that increments the global variable, and a global function `test` to trace the result:

	globalThis.x = 0;
	globalThis.increment = function() {
		return x++;
	}
	globalThis.test = function() {
		trace("app " + increment() + "\n");
	}
	
Then it creates a compartment with `mod.js` here above, passing the `increment` function as an endowment.

	let mod = new Compartment("mod", { increment });
	
Then executes:

	test();
	mod.global.test();
	test();
	mod.global.test();

To trace into xsbug:

	app 0
	mod 1
	app 2
	mod 3
	
## Modules	
	
Using the global scope in modules is rare. Modules usually define features in their own scope.

Let us define a module for use in the Modules examples that has as its default export a function that increments a local variable `x`.

#### increment.js

	let x = 0;
	export default function() {
		return x++;
	}
	
### Separate modules
	
By default, at run time, compartments load modules separately so compartments also have separate module scopes.

#### mod.js
	
The compartment module imports the `increment` function and exports a function to trace the result. 
	
	import increment from "increment";
	export function test() {
		trace("mod " + increment() + "\n");
	}
	
#### app.js
	
The main module also imports the `increment` function and defines a function to trace the result. 

	import increment from "increment";
	function test() {
		trace("app " + increment() + "\n");
	}

Then creates a compartments with `mod.js` here above.
	
	let mod = new Compartment("mod");
	
Then executes:

	test();
	mod.export.test();
	test();
	mod.export.test();

To trace into xsbug:

	app 0
	mod 0
	app 1
	mod 1

Since the `increment` module has been loaded separately by the app and the mod, there are separate `x` variables and `increment` functions.

### Shared Modules

To load the `increment` module only once, the manifest of the app has to instruct the XS linker to preload the `increment` module.

	"preload": [
		"increment"
	],

Preloading modules is a kind of vetted customization code. The bodies of preloaded modules are executed at build time and the created objects and closures are stored in ROM, with the built-ins. 

Like built-ins, preloaded modules are shared by compartments. But, thanks to XS aliasing mechanism, created objects do not have to be frozen and created closures do not have to be constants.
	
#### mod.js
	
The compartment module imports the `increment` function and exports a function to trace the result. 
	
	import increment from "increment";
	export function test() {
		trace("mod " + increment() + "\n");
	}
	
#### app.js
	
The main module also imports the `increment` function and defines a function to trace the result. 

	import increment from "increment";
	function test() {
		trace("app " + increment() + "\n");
	}

Then creates a compartments with `mod.js` here above.
	
	let mod = new Compartment("mod");
	
Then executes:

	test();
	mod.export.test();
	test();
	mod.export.test();

To trace into xsbug:

	app 0
	mod 1
	app 2
	mod 3

Since the `increment` module has been preloaded, the app and the mod share the `x` variable and `increment` function.

## Modules Map

Apps may restrict the modules mods can access thru the `modules` parameter of the `Compartment` constructor. The `modules` parameters is an object that maps module specifiers to module paths in the Moddable runtime. 

Apps and mods access their own modules map with `Compartment.map`, a getter which returns a new object like:  

	{
		"app": "/app.xsb",
		"increment": "/increment.xsb",
		"mod": "/mod.xsb",
	}
	
> Modules maps contain both shared and not shared modules.

### Restrictions

Apps may use `delete` on the object returned by `Compartment.map` to restrict the modules that mods may access.

#### app.js

	let map = Compartment.map;
	delete map.increment;
	let mod = new Compartment("mod", {}, map);

#### mod.js

	import increment from "increment"; // fails

The `Compartment` constructor always filters the `modules` parameter based on its own modules map. So a compartment can only give access to modules it can access itself. To enforce such constraint, each compartment has its own `Compartment` constructor. 

### Variations

The `modules` parameters may also be used to provide different implementations of a module to different compartments.

#### mod.js

	import vary from "vary";
	export function test() {
	    trace(name + " " + vary() + "\n");
	}
	
`name` is a global variable to be passed as an endowment.

#### app.js

The main module creates two compartments with different modules maps: 

	let { decrement, increment, mod } = Compartment.map;
	let mod1 = new Compartment("mod", { name:"mod1" }, { mod, vary:decrement });
	let mod2 = new Compartment("mod", { name:"mod2" }, { mod, vary:increment });
	
Then executes:	
	
	mod1.export.test();
	mod2.export.test();
	mod1.export.test();
	mod2.export.test();

To trace into xsbug:

	mod1 0
	mod2 0
	mod1 -1
	mod2 1

## Runtime Evaluators

Although it is currently rare for the Moddable runtime to have evaluators, XS implements the secure specifications. The constructors of syntactically created functions, generators, async functions and async generators always throw.

	function f() {}
	new f.constructor(""); // throws

Each compartment has its own `Function` constructor and its own `eval` function.

For instance in the *Separate Globals* example here above, the `increment` function:

	globalThis.increment = function() {
	    return x++;
	}

can be replaced by:

	globalThis.increment = new Function("return x++");
	
or by:
	
	globalThis.increment = function() {
	    return (1,eval)("x++");
	}

without changing the observed behavior.
	