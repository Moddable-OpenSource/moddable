# XS Compartment

Revised: July 12, 2022


XS implements most of the [TC39 Compartment Proposal](https://github.com/tc39/proposal-compartments). Beware that the proposal is at en early stage and that the programming interface described in this document will likely evolve.

## About

In XS, the real host is the application that creates an XS machine to evaluate scripts and import modules.

Compartments are lightweight virtual hosts that evaluate scripts and import modules separately from the real host and from other compartments. 

Compartments have their own `globalThis` object, their own global lexical scope and their own module map. 

> The module map binds module specifiers to modules. Inside compartments like inside the real host, the same module specifier always imports the same module.

By default:

- the `globalThis` object contains only the built-ins,
- the global lexical scope is empty,
- the module map is empty.

Compartments run in the same XS machine:

- Except for `Compartment`, `Function` and `eval`, built-ins are shared. 
- Other objects can be shared thru the `globals` and `globalLexicals` options. 
- Modules can be shared thru the `modules`, `loadHook` and `loadNowHook` options.

For the sake of security, it is the responsibility of a real host that creates compartments to ensure that shared features are immutable.

### Attenuation

A parent compartment can create child compartments.

	const parent = new Compartment();
	parent.evaluate(`
		const child = new Compartment();
	`);

A compartment can only provide to its child compartments the features provided by its parent compartment (and new features based on the features provided by its parent compartment).

### Static Module Record

A static module record is an object that encapsulates informations necessary to build a module.

In XS, a static module record contains the module bindings in a compressed form and the byte codes of the function to initialize the module bindings.

Static module records are immutable and reusable. In Moddable runtime, static module records can be stored in ROM.

See the [`StaticModuleRecord`](#StaticModuleRecord) constructor for the various ways to create static module records.

### Module Descriptor

Comparments can load and initialize module namespaces from module descriptors. Like property descriptors, module descriptors are ordinary objects with various forms. 

#### descriptor with a `namespace` property

To reference an already available module namespace.

	import * as foo from "foo"
	const c = new Compartment({
		modules: {
			foo: { namespace: foo }
		}
	});

- The value of the `namespace` property must be a module namespace.

#### descriptor with `record` and `importMeta` properties

To build a module namespace from a static module record.

	const c = new Compartment({
		modules: {
			foo: { record: new StaticModuleRecord("export default foo") }
		}
	});

- The value of the `record` property must be a static module record.
- If the `importMeta` property is present, its value must be an object. The default `importMeta` object is an empty object.

Compartments copy `importMeta` object properties into the module `import.meta` object like `Object.assign`. 

As a shortcut, instead of a `record` property, XS also supports the properties of the options of the `StaticModuleRecord` constructor. The shortcut also allows XS to skip the creation of the static module record at runtime.

#### descriptor with `specifier` and `compartment` properties

To reference a module namespace to be loaded and initialized by another `compartment`. 

```
	const c1 = new Compartment({
		modules: {
			foo: { record: new StaticModuleRecord("export default foo") }
		}
	});
	const c2 = new Compartment({
		modules: {
			foo: { specifier:"foo", compartment: c1 }
		}
	});
```	

- The value of the `specifier` property is coerced into a string.
- If the `compartment` property is present, its value must be a compartment.

If absent, the `compartment` property defaults to the (real or virtual) host of the compartment being constructed in the `modules` option or being hooked in the `loadHook` and `loadNowHook` options.

## Compartment Constructor

### Compartment(options)

Returns a compartment object. 

If present, the `options` argument must be an object with optional properties: `globals`, `globalLexicals`, `modules`, `loadHook`, `loadNowHook`, `resolveHook`.

#### options.globals

The `globals` option adds properties to the compartment `globalThis` object. 

If defined, the value of the `globals` option must be an object.

The `Compartment` constructor copies the `globals` object properties into the compartment `globalThis` object like `Object.assign`.

A compartment does not keep a reference to the `globals` object.

#### options.globalLexicals

The `globalLexicals` option initializes the compartment global lexical scope.

If defined, the value of the `globalLexicals` option must be an object. 

Each own enumerable named property of the `globalLexicals ` object become a variable or a constant:

- property names are variable or constant names,
- property values are variable or constant values. 

If the property is writable, a variable (`let`) is created, else a constant (`const`) is created.

A compartment does not keep a reference to the `globalLexicals` object.

#### options.modules

The `modules` option initializes the compartment module map.

If defined, the value of the `modules` option must be an object. 

Each own enumerable named property of the `modules` object creates an entry in the compartment module map:

- property names are module specifiers,
- property values are module descriptors.

Modules are neither loaded nor initialized by the `Compartment` constructor.

A compartment does not keep a reference to the `modules` object.

#### options.loadHook(specifier)

The `loadHook` option is an asynchronous function that takes a module specifier and returns a promise to a module descriptor.

The `loadHook` function is only called directly or indirectly by `Compartment.prototype.import` if the module map of the compartment has no entry for a module specifier.

The `loadHook` function creates an entry in the the module map of the compartemnt.

The `loadHook` function is useful if the module descriptor is unavailable when constructing the compartment, or to create a module descriptor dynamically. 

A compartment keeps a reference to the `loadHook` function.

#### options.loadNowHook(specifier)

The `loadNowHook` option is a function that takes a module specifier and returns a module descriptor.

The `loadNowHook` function is only called directly or indirectly by `Compartment.prototype.importNow` if the module map of the compartment has no entry for a module specifier.

The `loadNowHook` function creates an entry in the the module map of the compartemnt.

The `loadNowHook ` function is useful if the module descriptor is unavailable when constructing the compartment, or to create a module descriptor dynamically. 

A compartment keeps a reference to the `loadNowHook ` function.

#### options.resolveHook(importSpecifier, referrerSpecifier)

The `resolveHook` option is a function that takes an import specifier and a referrer specifier and returns a module specifier.

The `resolveHook` function is called indirectly by `Compartment.prototype.import` and `Compartment.prototype.importNow` when an imported module uses `import` declarations or calls.

Typically the `resolveHook` function combine a relative path and an absolute path or URI into an absolute path or URI. But the `resolveHook` function can build arbitrary specifiers.

The default `resolveHook` function calls the `resolveHook` function of the parent compartment.

A compartment keeps a reference to the `resolveHook` function.

## Properties of the Compartment Prototype

### get globalThis

Returns the `globalThis` object of the compartment.

Except for `Compartment`, `Function` and `eval`, built-ins are shared.

### evaluate(script)

Evaluates the script in the compartment and returns its completion value.

Scripts are evaluated in strict mode, with the global lexical scope of the compartment, and with `this` being the `globalThis` object of the compartment.

### import(specifier)

Asynchronously loads and initializes a module into the compartment and returns a promise to its namespace.

The specifier is used to get a module:

- from the compartment module map,
- else with the compartment `loadHook`.

If necessary, the compartment loads and initializes the module. All `import` declarations or calls are firstly resolved by the compartment `resolveHook`, then follow the same process. Eventually the promise is fulfilled with the module namespace.

### importNow(specifier)

Synchronously loads and initializes a module into the compartment and returns its namespace.

The specifier is used to get a module:

- from the compartment module map,
- else with the compartment `loadNowHook`.

If necessary, the compartment loads and initializes the module. All `import` declarations are firstly resolved by the compartment `resolveHook`, then follow the same process. Eventually the module namespace is returned.

- The motivation behind `importNow` is to avoid unnecessary delays and to allow applications to get rid of the promise machinery. Such applications support neither `async` nor `await` nor the `import` call. The implementation of `importNow` must not depend on the availability of promises.

- Applications that support promises, `async`, `await` and the `import` call can still use `importNow`. That is expected to be rare but, then, `importNow` throws when initializing a module with top level `await`.

### [@@toStringTag]

The initial value of this property is the `"Compartment"` string.

## <a name="StaticModuleRecord"></a>StaticModuleRecord Constructor

### StaticModuleRecord(options)

Returns a static module record.

#### options with a `source` property

To construct a static module record from a string parsed and compiled as a module. 

- The `source` property is coerced into a string. 

XS does not keep a reference to the `source` string.

	const smr = new StaticModuleRecord({ source: `
		import x from "mod";
		export let y = x;
	` });
	
As a shortcut, if the `options` argument is a string, the behavior is the same.
 
	const smr = new StaticModuleRecord(`
		import x from "mod";
		export let y = x;
	`);

#### options with `initialize`, `bindings`, `needsImport` and `needsImportMeta` properties

To construct a static module record from its bindings and a function to initialize them.

	const smr = new StaticModuleRecord({ 
		bindings:[
			{ import: "x", from: "mod" },
			{ export: "y" },
		], 
		initialize($) {
			$.y = $.x;
		} 
	});

- The `initialize` property must be a function. 
- If defined, the `bindings` property must be an array. The default is an empty array.
- If defined, the `needsImport` property is coerced into a boolean. The default is `false`.
- If defined, the `needsImportMeta` property is coerced into a boolean. The default is `false`.

The `bindings` array allow the same expressiveness as import and export declarations without requiring a JavaScript parser. The constructor checks the declarations and can throw a `SyntaxError`. XS compresses the declarations and does not keep a reference to the `bindings` array.

Once the module is loaded and linked, the compartment calls the `initialize` function with three arguments

- `$`: the [module environment record](https://tc39.es/ecma262/#sec-module-environment-records),
- `Import`: a function equivalent to the `import` call in a module body. The argument is `undefined` if `needsImport` was false.
- `ImportMeta`: an object equivalent to the `import.meta` object in a module body. The argument is `undefined` if `needsImportMeta` was false.

The module environment record is sealed: 

- no properties can be created or deleted,
- export properties are writable,
- import properties are read-only,
- there are no reexports properties.

Like a module body, the `initialize` function can be asynchronous.

	const smr = new StaticModuleRecord({ 
		bindings:[
			{ import: "x", from: "mod" },
			{ export: "y" },
			{ export: "z", from: "mod" },
		], 
		async initialize($, Import, ImportMeta) {
			try {
				$.z = 0;
			}
			catch {
				// not extensible
			}
			try {
				delete $.y;
			}
			catch {
				// not allowed
			}
			try {
				$.x = 0;
			}
			catch {
				// constant
			}
		} 
	});
	
#### options with `archive` and `path` properties

To construct a static module record from a **mod**. In Moddable runtime, mods are separate archives of modules and resources. 

- The `archive` property must be an archive. 
- The `path` property is coerced into a string. 



## Properties of the StaticModuleRecord Prototype

### get bindings()

Returns the static module record bindings.

XS stores bindings into a private compressed form. The `bindings` getter decompresses and publishes the bindings into an array of JSON-like objects.

There are many forms of module bindings. See the
[imports](https://tc39.es/ecma262/#table-import-forms-mapping-to-importentry-records) and [exports](https://tc39.es/ecma262/#table-export-forms-mapping-to-exportentry-records) tables.

Most bindings can be represented as JSON-like objects with `export`, `import`, `as`, `from` properties. Except `*` bindings, which requires special forms because module namespace identifiers can be arbitrary.  

For instance:

	const smr = new StaticModuleRecord(`
		export var v0;	
		export default 0
		export { v0 as w0 };	
	
		import v1 from "mod";
		import * as ns1 from "mod";	
		import { x1 } from "mod";	
		import { v1 as w1 } from "mod";	
	
		export { x2 } from "mod";
		export { v2 as w2 } from "mod";
		export * from "mod";
		export * as ns2 from "mod";
	`);
	
	print(JSON.stringify(smr.bindings, null, "\t"));

Prints to the console:

	[
		{
			"export": "v0"
		},
		{
			"export": "default"
		},
		{
			"export": "v0",
			"as": "w0"
		},
		{
			"import": "default",
			"as": "v1",
			"from": "mod"
		},
		{
			"importAllFrom": "mod",
			"as": "ns1"
		},
		{
			"import": "x1",
			"from": "mod"
		},
		{
			"import": "v1",
			"as": "w1",
			"from": "mod"
		},
		{
			"export": "x2",
			"from": "mod"
		},
		{
			"export": "v2",
			"as": "w2",
			"from": "mod"
		},
		{
			"exportAllFrom": "mod"
		},
		{
			"exportAllFrom": "mod",
			"as": "ns2"
		}
	]


### get needsImport()

Returns true if the static module record uses the `import` call.

### get needsImportMeta()

Returns true if the static module record uses the `import.meta` object.

### StaticModuleRecord.prototype.[@@toStringTag]

The initial value of this property is the `"StaticModuleRecord"` string.
