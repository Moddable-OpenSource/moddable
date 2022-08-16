# XS Compartment

Revised: July 18, 2022

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

### Implementation

XS implements compartments natively, without `Module` and `Evaluators` classes, and without modifying dynamic import. Maybe other proposals like the `module` construct will eventually justify such a dramatic evolution of the ECMAScript module machinery. But compartments do not.

## Built-ins

### Compartment Constructor

#### Compartment(options)

Returns a compartment, an instance of `Compartment.prototype`. 

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
- property values are [module descriptors](#ModuleDescriptor).

Modules are neither loaded nor initialized by the `Compartment` constructor.

A compartment does not keep a reference to the `modules` object.

#### options.loadHook(specifier)

The `loadHook` option is an asynchronous function that takes a module specifier and returns a promise to a [module descriptor](#ModuleDescriptor).

The `loadHook` function is only called directly or indirectly by `Compartment.prototype.import` if the module map of the compartment has no entry for a module specifier.

The `loadHook` function is useful if the module descriptor is unavailable when constructing the compartment, or to create a module descriptor dynamically. 

A compartment keeps a reference to the `loadHook` function.

#### options.loadNowHook(specifier)

The `loadNowHook` option is a function that takes a module specifier and returns a [module descriptor](#ModuleDescriptor).

The `loadNowHook` function is only called directly or indirectly by `Compartment.prototype.importNow` if the module map of the compartment has no entry for a module specifier.

The `loadNowHook ` function is useful if the module descriptor is unavailable when constructing the compartment, or to create a module descriptor dynamically. 

A compartment keeps a reference to the `loadNowHook ` function.

#### options.resolveHook(importSpecifier, referrerSpecifier)

The `resolveHook` option is a function that takes an import specifier and a referrer specifier and returns a module specifier.

The `resolveHook` function is called indirectly by `Compartment.prototype.import` and `Compartment.prototype.importNow` when an imported module uses `import` declarations or calls.

Typically the `resolveHook` function combine a relative path and an absolute path or URI into an absolute path or URI. But the `resolveHook` function can build arbitrary specifiers.

The default `resolveHook` function calls the `resolveHook` function of the parent compartment.

A compartment keeps a reference to the `resolveHook` function.

### Properties of the Compartment Prototype

#### get globalThis

Returns the `globalThis` object of the compartment.

#### evaluate(script)

Evaluates the script in the compartment and returns its completion value.

Scripts are evaluated in strict mode, with the global lexical scope of the compartment, and with `this` being the `globalThis` object of the compartment.

#### import(specifier)

Asynchronously loads and initializes a module into the compartment and returns a promise to its namespace.

The specifier is used to get a module:

- from the compartment module map,
- else with the compartment `loadHook`.

If necessary, the compartment loads and initializes the module. All `import` declarations or calls are firstly resolved by the compartment `resolveHook`, then follow the same process. Eventually the promise is fulfilled with the module namespace.

#### importNow(specifier)

Synchronously loads and initializes a module into the compartment and returns its namespace.

The specifier is used to get a module:

- from the compartment module map,
- else with the compartment `loadNowHook`.

If necessary, the compartment loads and initializes the module. All `import` declarations are firstly resolved by the compartment `resolveHook`, then follow the same process. Eventually the module namespace is returned.

- The motivation behind `importNow` is to avoid unnecessary delays and to allow applications to get rid of the promise machinery. Such applications support neither `async` nor `await` nor the `import` call. The implementation of `importNow` must not depend on the availability of promises.

- Applications that support promises, `async`, `await` and the `import` call can still use `importNow`. That is expected to be rare but, then, `importNow` throws when initializing a module with top level `await`.

#### [Symbol.toStringTag]

The initial value of this property is the `"Compartment"` string.

### <a name="ModuleSource"></a>ModuleSource Constructor

#### ModuleSource(source)

Returns a module source, an instance of `ModuleSource.prototype`.

The source argument is coerced into a string, then parsed as a module and compiled into byte codes. 

XS does not keep a reference to the `source` string.

### Properties of the ModuleSource Prototype

#### get bindings()

Returns the module source bindings.

XS stores bindings into a private compressed form. The `bindings` getter decompresses and publishes the bindings into an array of [module bindings](#ModuleBinding).

#### get needsImport()

Returns true if the module source uses the `import` call.

#### get needsImportMeta()

Returns true if the module source uses the `import.meta` object.

#### [Symbol.toStringTag]

The initial value of this property is the `"ModuleSource"` string.

## Patterns

### <a name="ModuleDescriptor"></a> Module Descriptor

Comparments can load and initialize module namespaces from module descriptors. Like property descriptors, module descriptors are ordinary objects with various forms. 

#### descriptors with `source`, `importMeta` and `specifier` properties

- If fhe value of the `source` property is a string, the parent compartment loads the module but the compartment itself initializes the module.

- Else if the value of the `source` property is a [module source](#ModuleSource), the module is loaded and initialized from the module source.

- Else the value of the `source` property must be an object. The module is loaded and initialized from the object according to the [virtual module source](#VirtualModuleSource) pattern,

If the `importMeta` property is present, its value must be an object. The default `importMeta` object is an empty object.

Compartments copy the `importMeta` object properties into the module `import.meta` object like `Object.assign`. 

If the `specifier` property is present, its value is coerced into a string and becomes the referrer specifier of the module.

#### descriptors with `namespace` and `compartment` properties

- If fhe value of the `namespace` property is a string, the descriptor shares a module to be loaded and initialized by the compartment referred by the `compartment` property. 

	- If the `compartment` property is present, its value must be a compartment.
	- If absent, the `compartment` property defaults to the compartment being constructed in the `modules` option, or being hooked in the `loadHook` and `loadNowHook` options.
	
- Else if the value of the `namespace ` property is a module namepace, the descriptor shares a module that is already available.

- Else the value of `record` property must be an object. The module is loaded and initialized from the object according to the [virtual module namespace](#VirtualModuleNamespace) pattern.
	
#### descriptor with `archive` and `path` properties

To construct a static module record from a [mod](./mods.md). In Moddable runtime, mods are separate archives of modules and resources. 

- The `archive` property must be an archive. 
- The `path` property is coerced into a string then used to find the module in the archive.

### <a name="ModuleBinding"></a> Module Binding

A module binding is an ordinary object with properties that mimick the `import` and `export` constructs. There are many forms of module bindings. See the
[imports](https://tc39.es/ecma262/#table-import-forms-mapping-to-importentry-records) and [exports](https://tc39.es/ecma262/#table-export-forms-mapping-to-exportentry-records) tables.

Most bindings can be represented as JSON-like objects with `export`, `import`, `as`, `from` properties. Except `*` bindings, which requires special forms because module namespace identifiers can be arbitrary.  

| Construct | Module Binding |
| :--- | :--- |
| export { x } | { export: "x" }
| export { x as y } | { export: "x", as: "y" }
| export { x } from "mod" | { export: "x", from: "mod" }
| export { x as y } from "mod" | { export: "x", as: "y", from: "mod" }
| export * from "mod" | { exportAllFrom: "mod" }
| export * as star from "mod" | { exportAllFrom: "mod", as: "star" }
| import x from "mod" | { import: "default", as: "x", from: "mod" }
| import { x } from "mod" | { import: "x", from: "mod" }
| import { x as y } from "mod" | { import: "x", as: "y", from: "mod" }
| import * as star from "mod" | { importAllFrom: "mod", as: "star" } 

### <a name="VirtualModuleSource"></a> Virtual Module Source

A virtual module source is an ordinary object with `execute`, `bindings`, `needsImport` and `needsImportMeta` properties

- The `execute` property must be a function. 
- If defined, the `bindings` property must be an array of [module bindings](#ModuleBinding). The default is an empty array.
- If defined, the `needsImport` property is coerced into a boolean. The default is `false`.
- If defined, the `needsImportMeta` property is coerced into a boolean. The default is `false`.

The `bindings` array allow the same expressiveness as import and export declarations without requiring a JavaScript parser. Compartments check the declarations and can throw a `SyntaxError`.

Once the module is loaded and linked, the compartment calls the `execute` function with three arguments

- `$`: the [module environment record](https://tc39.es/ecma262/#sec-module-environment-records),
- `Import`: a function equivalent to the `import` call in a module body. The argument is `undefined` if `needsImport` was false.
- `ImportMeta`: an object equivalent to the `import.meta` object in a module body. The argument is `undefined` if `needsImportMeta` was false.

The module environment record is sealed: 

- no properties can be created or deleted,
- export properties are writable,
- import properties are read-only,
- there are no reexports properties.

Like a module body, the `execute` function can be asynchronous.

### <a name="VirtualModuleNamespace"></a> Virtual Module Namespace

A virtual module namespace is an ordinary object posing as a module namespace.

When a compartment loads a virtual module namespace, each own enumerable named property of the object becomes an exported property of the module. 
