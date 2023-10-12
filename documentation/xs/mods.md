# Mods - User Installed Extensions
Copyright 2020-2023 Moddable Tech, Inc.<BR>
Revised: October 11, 2023

Mods are scripts that users can install on their IoT products to add new features and change existing behaviors. A mod is one or more JavaScript modules together with assets like images, audio, and certificates. A mod augments the software of a product without changing the factory installed firmware. To minimize safety, security, and privacy risks, products may sandbox mods using Secure ECMAScript (aka Hardened JavaScript). Mods are a new core feature of the Moddable SDK supported by the XS JavaScript engine.

Mods are simple to create but the technology behind them is complex. This article introduces the fundamentals of mods, how to create mods, and how to implement support for running mods in projects. Mods run on microcontrollers with constrained resources which in turn puts constraints on mods. This article describes some of these limitations to help authors of mods and mod hosts make the best tradeoffs for their projects.

## Key Characteristics of Mods

Mods are a tool to extend the capabilities of IoT products. It is rare today for an IoT product to support extensions, and when they do it is through native code. Using JavaScript as the foundation for mods not only makes it feasible to support extensibility in many more products, it empowers many more users to do so.

- **Mods are portable** - The JavaScript language was designed from the start to be independent of both the host operating system and the host hardware. The Moddable SDK follows that by providing APIs that are consistent across devices.  This portability is critical so that developers can use their knowledge and experience across devices from many different manufacturers, rather than needing to learn new development tools, a new language, and new APIs for each IoT product.
- **Mods are standard** - IoT products are built using standards -- from standard electrical connectors to standard Wi-Fi -- to ensure they are reliable, safe, and can interoperate with products from different manufacturers. Mods follow this proven pattern by using modern industry-standard JavaScript (ECMAScript 2020) as their programming language. Moddable is helping to standardize APIs for IoT products through the Ecma TC53 committee, ECMAScript Modules for Embedded Systems.
- **Mods are secure** - IoT products control physical devices in the real world. It is essential that they are safe and secure when running mods. The Moddable SDK achieves this using Compartments, a sandboxing method from Secure ECMAScript. Compartments allow a mod host to control when a mod runs and the capabilities a mod has access to. This allows a mod host to set a security policy, for example, that allows a mod to control a light bulb at only certain times of day and to only communicate with certain trusted cloud services. Secure ECMAScript guarantees the mod cannot break out of the security rules.
- **Mods are lightweight** - Because memory and storage are often limited on IoT products, mods must be able to do useful work with a minimum of resources. Because mods are installed as precompiled byte code, they are ready to run immediately and their byte-code executes in place from flash storage without having to be loaded into memory. Many useful mods require just a few KB of code to do useful work.
- **Mods add value** - Users of IoT products inevitably demand more features than the product manufacturer is willing or able to implement. Mods solve this problem by allowing users and third party software developers to extend the built-in software of the product to add new features, to support additional cloud connectivity, to add new device integrations, to provide a custom user interface, and more.
- **Mods simplify products** - IoT products continue to support more features to meet the needs of various customer segments. This proliferation of features makes products more difficult to use, more difficult to implement, and much more difficult to test and validate before shipping. Mods simplify products by allowing a manufacturer to build-in only the core features and allowing users to install mods for the features they use.

## Anatomy of a Mod
A mod is made up of three parts: the JavaScript source code, any data assets it needs, and a manifest to describe how to build it. Each of these three parts works in about the same way as they do when building full projects in the Moddable SDK.

### JavaScript Source Code
A mod can contain several JavaScript source code files. Each source code file is a standard JavaScript module. The modules may be imported by the mod host and by the mod itself.

Mods are not automatically run when the system starts. There may not even be a `main.js` file. Instead, the mod host decides which modules to run and when. For example, a mod host that uses mods to render a screen saver might load and run the mod's `screensaver.js` module only when the system is idle.

A mod may use any features of the JavaScript language that are available. Because of storage constraints, the mod host may choose to omit certain JavaScript language features that it determines are not essential for its operation. If a mod attempts to use a language feature that is unavailable, XS throws a "dead strip" error. Mod hosts should be able to support full JavaScript on many common low-cost microcontrollers (such as the ESP32). The limitations are necessary on microcontrollers with constrained flash storage (such as the Silicon Labs Gecko parts) or limited flash address space (such as the ESP8266 which typically has a 4 MB flash chip but only a 1 MB address space to access it).

### Data and Assets
Mods can contain data in addition to code. A mod that displays a user interface can contain the images, fonts, and sounds it needs. A mod that connects to a network service can include TLS certificates as data. Mods can also contain text or binary data, such as calibration data, that it needs.

The mod's data is accessed using the `Resource` constructor, which allows the data to be used directly from flash storage without having to be loaded into memory.

### Manifest
A mod's manifest is a JSON file that describes the files used in the mod. The manifest is a subset of the full [manifest format](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/tools/manifest.md) used by the Moddable SDK, with only the `include`, `module`, `data`, `resource`, and `config` sections. The `build` and `platform` sections are also supported but rarely used.

A mod's manifest usually includes the manifest at [`$MODDABLE/examples/manifest_mod.json`](../../examples/manifest_mod.json) which contains settings required for building and deploying the mod, such as the connection speed to use to transfer the mod to the device.

### Example Mod
This section shows modules, data, and manifest of a simple mod.

There is just one module, which imports data from a resource and traces its content to the console together with the version number of the mod.

```js
import Resource from "Resource";
import config from "mod/config";

let message = new Resource("message.txt");
trace(String.fromArrayBuffer(message), "\n");
trace(`Software version ${config.version}\n`);
```

The mod contains a single data file, a text file with a short message.

```
Hello, mod.
```

The manifest tells the build system to include these two files.

```json
{
	"include": "$(MODDABLE)/examples/manifest_mod.json",
	"config": {
		"version": 1.1
	},
	"modules": {
		"*": [
			"./mod"
		]
	},
	"data": {
		"*": [
			"./message"
		]
	},
}
```

> **Note**: If there are no `"config"` values, either in the manifest or on the command line, then the "mod/config" module is not created.

## Building, Running, and Debugging a Mod
A mod is built and run using the `mcrun` command line tool from the Moddable SDK. The `mcrun` tool is very similar to `mcconfig` which builds full hosts that include native code.

To build and run the example mod above, set the current working directory to the mod and then execute `mcrun`:

```
mcrun -d -m
```

The output from a successful build looks as follows:

```
> mcrun -d -m
# copy message.txt
# xsc mod.xsb
# xsc check.xsb
# xsc mod/config.xsb
# xsl mc.xsa
```

When building for the desktop, the mod is automatically opened in the simulator. However, the mod cannot run by itself because it requires a mod host. The mod host is launched first using `mcconfig` and then the mod is launched using `mcrun`. A simple mod host is introduced below.

You can use `mcrun` to build and install the mod on a microcontroller by specifying the target platform with the `-p` option in the same way as `mcconfig`.

```
mcrun -d -m -p esp32/moddable_two
```

For `mcrun` to install the mod, a debug build of a mod host must first be installed on the device. The mod host must be a debug build because `mcrun` installs the mod using the xsbug debugger protocol. Mods can also run on a release build of a mod host but must be installed in some other way ([this discussion](https://github.com/Moddable-OpenSource/moddable/discussions/1105) shows one way to do this on the ESP32 silicon family).

Once the mod is installed, `mcrun` automatically restarts the microcontroller and connects to xsbug to debug the mod.

## Hosting a Mod
Adding support for mods to a project is straightforward. It requires some additional configuration and adding code to invoke the mod. Beyond that, it may be necessary to adjust some of the default settings. This section describes the fundamental steps to hosting a mod and advanced configuration options.

### A Simple Mod Host
This section shows the essential steps for creating a mod host.

#### Manifest
The commonly used manifests in the Moddable SDK, such as [`manifest_base.json`](https://github.com/Moddable-OpenSource/moddable/blob/public/examples/manifest_base.json), do not enable support for mods because supporting mods requires some additional code. Only projects that actually use mods should include that code. To enable mods, add `XS_MODS` to the `defines` section of the project's manifest.

```
"defines": {
	"XS_MODS": 1
},
```

#### Checking For Installed Mods
A mod host may want to check if a mod is installed before attempting to load it. To do that use the `Modules` module, a utility module for working with mods. First, import the `Modules` module:

```js
import Modules from "modules";
```

Add the manifest for the `Modules` module to your project manifest:

```
"include": [
	$(MODULES)/base/modules/manifest.json
]
```

Then use the static `has` method to check if a given module is available. The following code determines if the module "mod" from the example above is installed:

```js
if (Modules.has("mod"))
	trace("mod installed\n");
```

The `has` function does not load or execute the mod. It only checks for its presence.

#### Running a Mod
The `Modules` module can load and run a module within a mod.

```js
if (Modules.has("mod"))
	Modules.importNow("mod");
```

The `importNow` function loads the module and executes the module's body, similar to calling dynamic `import`, though `importNow` is synchronous.

The `importNow` function returns the module's default export. Consider the following mod that exports several functions through its default export.

```js
export default {
	onLaunch() {
	},
	onLightOn() {
	},
	onLightOff() {
	}
};
```
The mod host loads the mod at start-up and calls `onLaunch` for the mod to initialize itself.

```js
const mod = Modules.importNow("mod");
mod.onLaunch();
```

The mod host invokes the other functions at the appropriate time, `onLightOn` when the light is turned on, `onLightOff` and when it is turned off.

### Checking Compatibility at Runtime
Each mod can have a module named `check` which contains a function to verify that the mod is compatible with the current host. By default the `check` module is created automatically by `mcrun` to verify that any graphics in the mod are compatible with the current host. It is a good practice to invoke the `check` module, when present, before any other. The `check` module exports a function that throws an exception if it finds an incompatibility.

```js
if (Modules.has("check")) {
	try {
		const checkFunction = Modules.importNow("check");
		checkFunction();
	}
	catch (e) {
		trace(`Incompatible mod: ${e}\n`);
	}
}
```

If the pixel format is set to `"x"`, the `check` module is not generated by `mcrun`. To set the pixel format, use `-f x` on the command line when invoking `mcrun` or set `"format"` in the `"config"` section of the mod's manifest.  

### Keys
The dynamic nature of the JavaScript language means that the JavaScript engine needs to keep track of all properties names used by the virtual machine. XS does this using a key array. The [`preload`](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/xs/preload.md) feature used for building hosts for microcontrollers stores all the host's property names into a key array in flash storage.

At launch, XS creates a second array to store keys created at runtime. This array has an initial allocation size and then grows in increments as needed. 

Mods, however, may create many more symbols because they can contain code with many property names that do not appear in the host. To accommodate this, a  project can adjust the initial length of the runtime key array in the `"creation"` section of the manifest. 

```
"creation": {
	"keys": {
		"initial": 128,
		"incremental": 32
	}
}
```

Each entry in the key array is a pointer, which is 4 bytes on a 32-bit microcontroller. The 128 key table above uses 512 bytes of memory.

The instruments pane in xsbug displays the number of keys allocated in the runtime key array in the "Keys Used" section. Monitoring this value can help select an optimal initial length for the runtime key array.

### JavaScript Language Features
The default behavior of the Moddable SDK build tools is to remove any JavaScript language features that are not used by the host modules. This process reduces the size of the engine, saving flash storage space and speeding installation time.

The XS linker is able to safely strip unused language features by analyzing the byte code of the project's modules. The linker cannot, however, analyze the byte code of mods, which are only installed after the host is running.

A mod host can use the manifest to control which language features are available instead of allowing the XS linker to perform automatic stripping. The simplest approach is to keep all language features. For projects with no flash storage pressure, this is a viable option. Add a `strip` object with an empty strip array to the project manifest:

```
"strip": []
```

A mod host may reduce its flash storage size by explicitly stripping selected features. This is done by naming the constructors and functions to strip.  The following strip array removes features seldom used on constrained microcontrollers, such as `eval`, maps and sets, atomics, shared array buffers, regular expressions, and proxies.

```
"strip": [
	"Atomics",
	"eval",
	"Function",
	"Generator",
	"Map",
	"Proxy",
	"Reflect",
	"RegExp",
	"Set",
	"SharedArrayBuffer",
	"WeakMap",
	"WeakSet"
]
```

For additional details on stripping language features, see the [Strip](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/tools/manifest.md#strip) section of the manifest documentation.

When a mod uses a JavaScript language feature that has been stripped, a "dead strip" exception is generated and the virtual machine terminates execution.

### Securing a Mod
Mods are powerful because they have access to the same modules and global variables as the host. This means that a mod is likely able to perform all of the same operations as the host. For a simple host that only prepares an environment for the mod to run in, this is not a problem.  However, some hosts want to limit what a mod can do to ensure the integrity of operation of the host. These IoT product hosts want to ensure that the mod can only perform those operations the host chooses to allow.

Restricting a script from access to certain capabilities is common. Every web browser ensures the integrity of the user's computer by restricting scripts in many ways. For example, scripts cannot normally access the user's file system.

Using Secure ECMAScript, a mod host can restrict the capabilities available to a mod, just as a browser does for the scripts it hosts. This restriction is accomplished using Secure ECMAScript (SES), a proposed enhancement to the JavaScript language that is already implemented in XS. SES restricts the modules of a mod by loading them into a `Compartment`, a lightweight sandbox inside the current JavaScript virtual machine.

The following sections show simple examples of how SES gives mod hosts the ability to restrict the capabilities a mod may access and how SES eliminates certain attacks.

#### Restricting Module Access
The following code creates a compartment and loads the mod's "mod' module into the compartment.

```js
let c = new Compartment(
	{},
	{
		"mod": "mod"
	}
});
let exports = c.importNow("mod");
```

The first argument to the `Compartment` constructor relates to global variables inside the compartment and is described in the following section. The second argument is the compartment's module map. It performs two functions. First, it determines which modules are available to be imported by scripts running inside the compartment. Second, it allows remapping module specifiers to change how a module is accessed inside the compartment.

The module map above allows only the module named "mod" to be imported inside the compartment. There is no remapping of module specifiers. The property name on the left ("mod") is the module specifier inside the compartment and the value on the right (also "mod") is the module specified in the host.

The call to the `importNow` method loads the module "mod" inside of compartment `c`. Because of the module map, the mod cannot import any of the modules from the host. If it tries to do so, the import fails as if the module is not present.

The compartment map below is changed to use remapping.

```js
let c = new Compartment(
	{},
	{
		"modExample": "mod"
	}
});
let exports = c.importNow("modExample");
```

Remapping is useful for giving the mod access to a limited version of an existing module. For example, a host may wish to give a mod access to the HTTP client module but wants to restrict the domains to which the HTTP client can connect. The host contains the unrestricted HTTP module ("http") and a restricted version ("httpRestricted"). The host would use the unrestricted version for its requests. It would provide the mod the restricted version, remapping its module specifier to "http" inside the compartment so that the mod can use the HTTP client module by importing "http".

#### Separate Globals
Projects often store important data and objects in global variables for convenient access. These globals may provide access to capabilities or private information that the mod host wishes to keep from the mod. Without SES, the global variables are available to mods. When SES creates a new compartment, the compartment receives its own set of global variables that are separate from the globals of the mod host. The compartment's global variables contain only the globals defined by the JavaScript language.

In the following code, the global variable "secret" is unavailable to the mod.

```js
globalThis.secret = new Secret;
let c = new Compartment(
	{},
	{
		"modExample": "mod"
	}
});
let exports = c.importNow("modExample");
```

The first argument to the compartment constructor is a map of additional globals to add to the compartment when it is created. In the following code, the `secret` global is made available to the mod as a global variable with the name `sharedSecret`:

```js
globalThis.secret = new Secret;
let c = new Compartment(
	{
		sharedSecret: globalThis.secret
	},
	{
		"modExample": "mod"
	}
});
let exports = c.importNow("modExample");
```

The compartment instance `c` has a `globalThis` property which the mod host can use to access the globals of the compartment. The previous example may be rewritten using the compartment's `globalThis` property.

```js
globalThis.secret = new Secret;
let c = new Compartment(
	{
		"modExample": "mod"
	}
});
c.globalThis.sharedSecret = globalThis.secret;
let exports = c.importNow("modExample");
```

The mod host can access and update the globals of the compartment at any time. This is a powerful capability that should be used with care to avoid creating an unpredictable execution environment for the mod.

#### Mod Cannot Intercept Mod Host Calls
Those with JavaScript experience might expect that a mod could interfere with the operation of the mod host through clever patching of the built-in primordial objects like `Object`, `Function`, and `Array`. For example, the following intercepts every call to push on `Array` instances.

```js
const originalPush = Array.prototype.push;
Array.prototype.push = function(...elements) {
	// code here now has access to both the array and arguments to push
	// allowing it to inspect and modify the parameters
	if (elements[0] > 0)
		elements[0] *= -1;
	return originalPush.call(this, ...elements);
}
```

This kind of global patch of built-in objects is called a [monkey patch](https://en.wikipedia.org/wiki/Monkey_patch). It is a technique that allows one script to attack another. In SES, this kind of attack is not possible through the primordial objects because they are frozen, making it impossible to change existing properties. If a script running in a compartment attempts to replace the `push` function, an exception is thrown.

## Behind the Scenes
Mods are easy to use, which makes it easy to overlook the many complex details involved in making them work. This section describes implementation details that may be important when working with mods.

### Building Mods
`mcrun` converts the mod's JavaScript source code and data into a single binary file, an XS Archive (.xsa extension). The archive format is designed so JavaScript byte code executes directly from flash storage without having to be copied into memory.

The JavaScript modules are precompiled to XS byte code, so no source code is deployed to the target device. This allows the device to begin executing the mod without needing to parse the JavaScript source code first.

The image, font, and audio resources in the manifest are transformed to a format compatible with the target device following the manifest [resource rules](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/tools/manifest.md#resources).

While the source code to the mod is platform independent, the binary format of the mod is not. The pixel format and rotation of the images is optimized for a specific host, the XS byte code generated is for a specific version of the XS engine, etc. The archive is not a general purpose distribution format for mods.

The `mcrun` tool builds the mod without knowledge of the symbol table of the mod host. XS byte code references symbols by a 16-bit ID. When the mod is run by a mod host, XS automatically updates the symbol ID values in the mod's byte code so they match the host IDs. This process is very fast. The symbol IDs of the mod are only updated when the host changes, not on every run.

### Interaction with Preload
When a mod host loads a module, the mod is searched for the module before the host. This allows a mod to override modules of the host.

However, modules in the host that are preloaded have already completed their imports at build time. Therefore, the module imported by a preloaded module cannot be overridden by the modules of a mod.

This does not generally cause any issues with the use of mods as extensions to a mod host. It does preclude the use of mods to deliver patches to modules in the host.

### Where Mods are Stored
Mods are stored in flash memory, but the location and size of the mod storage area depends on the host.

Attempts to install a mod larger than the available space fail, so it is not possible to accidentally write outside the space reserved.

#### ESP32
The ESP-IDF on the ESP32 supports a partition map, and the Moddable SDK uses it to reserve a partition for mods. In the default [`partitions.csv`](https://github.com/Moddable-OpenSource/moddable/blob/6729c9482b9186f3654d8b158f095451edb74e62/build/devices/esp32/xsProj/partitions.csv#L25) the mod partition is defined here:

```
xs,       0x40, 1,       0x3A0000, 0x040000,
```

This reserves 256 KB of space for storing the mod. Moddable SDK projects for the ESP32 can use their own manifest to increase or decrease mod storage, or remove it if they do not support mods.

#### ESP8266 and nRF52
Storing mods on ESP8266 and nRF52 is more complex than ESP32 for two reasons. First, there are no formally defined partitions so the Moddable SDK must manage flash layout entirely. Second, the mod archive must be memory mapped to allow in-place execution, but the ESP8266 can only memory map the first 1 MB of flash and the nRF52 only has 1 MB of flash. This requires the mod archive be stored in the same 1 MB of space as the mod host.

The Moddable SDK solves this problem by storing the mod starting at the first flash sector following the mod host image. This gives the largest possible space for mods on each host. However, it also means that address of the mod and the space available for the mod depends on the host.

### XS Archive Format

The XS archive file format uses the Box (aka Atom) structuring mechanism of the [ISO Base Media File Format](https://www.loc.gov/preservation/digital/formats/fdd/fdd000079.shtml) to structure data. The atom structure of the XS archive file is shown in the following table:

```
XS_A - signature of the XS archive file
    VERS - XS version number used in byte code
    SIGN - the MD5 checksum of this mod's modules and data (before mapping)
    NAME - the dotted name of the mod (null terminated string)
    SYMB - the XS symbol table for the mods
    IDEN - host identifiers in the order of the symbol table (array of XS ID values)
    MAPS - symbol table indexes in the order of their occurrence in the CODE atoms
    MODS - the modules of this mod
        PATH - the path of this module (null terminated string)
        CODE - the byte code of of this module
        (Additional PATH / CODE pairs follow - one per module]
    RSRC - the data of this mod
        PATH - the path of this module (null terminated string)
        DATA - the data of this resource
        (Additional PATH / DATA pairs follow - one per piece of data]
```

The order of the atoms that precedes the `MODS` atom must be as shown because the microcontroller implementation expects this layout.

When loading the archive, XS iterates on the symbol table to build a mapped identifiers array. If the mapped identifiers array matches the `IDEN` atom, there is nothing else to do. Otherwise, the mapped identifiers table is written into the `IDEN` atom and XS traverses the `MAPS` and `CODE` atoms to map identifiers in the byte code, updating the CODE atoms accordingly.

#### About the Box / Atom structuring mechanism

The atom structuring mechanism is a lightweight way of structuring binary data.

Each atom begins with an 8 byte header consisting of two 32-bit big-endian values. The first value is an unsigned integer that indicates the size of the atom, including the header, in bytes.  The second value is a [four-character code](https://en.wikipedia.org/wiki/FourCC), typically consisting of human-readable ASCII values, that indentifies the content of the atom. For example, for the sole atom at the root of an XS archive file, the first value is the length of the file in bytes and the second value is `XS_A` indicating an atom containing an XS archive.

Each atom may contain atoms and/or other data. The content of an atom is defined by its four-character code. For example, the `RSRC` atom above is defined to contain zero or more pairs of `PATH` and `DATA` atoms, whereas the `NAME` atom is defined to contain a null terminated string.

File readers that do not recognize the four-character code of the atom can skip over the atom using the first value in the atom header. This approach allows new atoms to be introduced without breaking existing readers.
