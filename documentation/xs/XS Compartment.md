# XS Compartment

Revised: July 11, 2022


XS implements most of the [TC39 Compartment Proposal](https://github.com/tc39/proposal-compartments). Beware that the proposal is at en early stage and that the programming interface described in this document will probably change.

## About

In XS, the real host is the application that creates an XS machine to evaluate scripts and import modules.

Compartments are lightweight virtual hosts that evaluate scripts and import modules separately from the real host and from other compartments. 

Compartments have their own `globalThis` object, their own global lexical scope and their own module map. 

> The module map binds module identifiers to module namespaces. Inside compartments like inside the real host, the same module identifier always imports the same module namespace.

By default:

- the `globalThis` object contains only the built-ins,
- the global lexical scope is empty,
- the module map is empty.

Compartments run in the same XS machine:

- Except for `Compartment`, `Function` and `eval`, built-ins are shared. 
- Other objects can be shared thru the `globals` and `globalLexicals` options. 
- Module namespaces can be shared thru the `modules`, `loadHook` and `loadNowHook` options.

For the sake of security, it is the responsibility of a real host that creates compartments to ensure that shared features are immutable.

### Attenuation

A parent compartment can create child compartments.

	const parent = new Compartment();
	parent.evaluate(`
		const child = new Compartment();
	`);

A compartment can only provide to its child compartments the features provided by its parent compartment (and new features based on the features provided by its parent compartment).

### Module Descriptor

Comparments can load and initialize module namespaces from module descriptors. Like property descriptors, module descriptors are ordinary objects with various forms. 

XS supports the following descriptors:

#### descriptor with a `namespace` property

To reference an already loaded and initialized module namespace.

- The value of the `namespace` property must be a module namespace.

#### descriptor with `record` and `importMeta` properties

To build a module namespace from a module record.

- The value of the `record` property must be a module record.
- If the `importMeta` property is present, its value must be an object. The default `importMeta` object is an empty object.

Compartments copy `importMeta` object properties into the module `import.meta` object like `Object.assign`. 

Instead of a  `record` property, XS also supports the options of the `StaticModuleRecord` constructor: a `source` property, `bindings` and `initialize` properties, `archive` and `path` properties.

#### descriptor with `specifier` and `compartment` properties

To reference a module namespace to be loaded and initialized by another `compartment`. 

- The value of the `specifier` property is coerced into a string.
- If the `compartment` property is present, its value of must be a compartment.

If absent, the `compartment` property defaults to the (real or virtual) host of the compartment being constructed in the `modules` option or being hooked in the `loadHook` and `loadNowHook` options.

### Module Record

A module record encapsulates all the informations necessary to build a module namespace.

## Compartment Constructor

### Compartment(options)

Returns a compartment object. 

If present, the `options` argument must be an object with optional properties: `globals`, `globalLexicals`, `modules`, `loadHook`, `loadNowHook`, `resolveHook`.

### options.globals

The `globals` option adds properties to the compartment `globalThis` object. 

If defined, the value of the `globals` option must be an object.

The `Compartment` constructor copies the `globals` object properties into the compartment `globalThis` object like `Object.assign`.

A compartment does not keep a reference to the `globals` object.

### options.globalLexicals

The `globalLexicals` option initializes the compartment global lexical scope.

If defined, the value of the `globalLexicals` option must be an object:

- property names become variable or constant names,
- property values become variable or constant values. 

If the property is writable, a variable (`let`) is created, else a constant (`const`) is created.

A compartment does not keep a reference to the `globalLexicals` object.

### options.modules

The `modules` option initializes the compartment module map.

If defined, the value of the `modules` option must be an object:

- property names are module specifiers,
- property values are module descriptors.

Each property of the `modules` object creates an entry in the compartment module map.

Modules are neither loaded nor initialized by the `Compartment` constructor.

A compartment does not keep a reference to the `modules` object.

### options.loadHook(specifier)

The `loadHook` option is an asynchronous function that takes a module specifier and returns a promise to a module descriptor.

The `loadHook` function is only called directly or indirectly by `Compartment.prototype.import` if the module map of the compartment has no entry for the specifier.

The `loadHook` function creates an entry in the the module map of the compartemnt.

The `loadHook` function is useful if the module descriptor is unavailable when constructing the compartment, or to create a module descriptor dynamically. 

A compartment keeps a reference to the `loadHook` function.

### options.loadNowHook(specifier)

The `loadNowHook` option is a function that takes a module specifier and returns a module descriptor.

The `loadNowHook` function is only called directly or indirectly by `Compartment.prototype.importNow` if the module map of the compartment has no entry for the specifier.

The `loadNowHook` function creates an entry in the the module map of the compartemnt.

The `loadNowHook ` function is useful if the module descriptor is unavailable when constructing the compartment, or to create a module descriptor dynamically. 

A compartment keeps a reference to the `loadNowHook ` function.

### options.resolveHook(importSpecifier, referrerSpecifier)

The `resolveHook` option is a function that takes an import specifier and a referrer specifier and returns a module specifier.

The `resolveHook` function is called indirectly by `Compartment.prototype.import` and `Compartment.prototype.importNow` when an imported module uses `import` declarations or calls.

Typically the `resolveHook` function combine a relative path and an absolute path or URI into an absolute path or URI. But the `resolveHook` function can build arbitrary specifiers.

The default `resolveHook` function calls the `resolveHook` function of the parent compartment.

A compartment keeps a reference to the `resolveHook` function.

## Properties of the Compartment Prototype

### get Compartment.prototype.globalThis

Returns the `globalThis` object of the compartment.

Except for `Compartment`, `Function` and `eval`, built-ins are shared.

### Compartment.prototype.evaluate(script)

Evaluates the script in the compartment and returns its completion value.

Scripts are evaluated in strict mode, with the global lexical scope of the compartment, and with `this` being the `globalThis` object of the compartment.

### Compartment.prototype.import(specifier)

Asynchronously loads and initializes a module into the compartment and returns a promise to its namespace.

The specifier is used to find a module:

- in the compartment module map,
- else by the compartment `loadHook`.

If necessary, the compartment loads and initialize the module. Relative `import` declarations or calls are firstly resolved by the compartment `resolveHook`. Then all `import` declarations or calls follow the same process. Eventually the promise is fulfilled with the module namespace.

### Compartment.prototype.importNow(specifier)

Synchronously loads and initializes a module into the compartment and returns its namespace.

The specifier is used to find a module:

- in  the compartment module map,
- else by the compartment `loadNowHook`.

If necessary, the compartment loads and initialize the module. Relative `import` declarations are firstly resolved by the compartment `resolveHook`. Then all `import` declaration follow the same process and the module namespace is returned.

- The motivation behind `importNow` is to avoid unnecessary delays and to allow applications to get rid of the promise machinery. Such applications support neither `async` nor `await` nor the `import` call. The implementation of `importNow` must not depend on the availability of promises.

- Applications that support promises, `async`, `await` and the `import` call can still use `importNow`. That is expected to be rare but, then, `importNow` throws when initializing a module with top level `await`.
