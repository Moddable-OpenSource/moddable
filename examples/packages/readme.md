# Package Examples
Updated October 4, 2023

This directory contains example projects that use `package.json` to describe the build dependencies. These are intended to be used with `mcpack`, a tool in the Moddable SDK to package embedded applications. `mcpack` is front-end to the Moddable build process. It starts with `package.json`, scans the dependencies, and generates a Moddable `manifest.json` file which is built, deployed, and run using `mcconfig`.

## Simplifying Development
`mcpack` has has several features to help streamline development and provide compatibility with common JavaScript conventions. It does this by analyzing the project source code.

- **Built-in modules**. When a project imports a module built into the Moddable SDK such as "timer", "wifi", "pins/digital", or "piu/MC", `mcpack` includes the module in the project. Built-in modules may optionally prefixed with "moddable:", such as "moddable:timer", to be explicit that they are Moddable SDK built-in modules.
- **Well-known globals**. JavaScript developers are accustomed to using certain global variables, such as `setTimeout`, `console`, and `fetch` across JavaScript environments. `mcpack` detects use of these globals and automatically includes the required modules and defines the global. The following globals are supported.
	- `clearImmediate`, `setImmediate`, `setInterval`, `setTimeout` - Timer functions.
	- `console.log()` - Output to the debug console. The other capabilities of `console` are not provided
	- `fetch` - HTTP requests.
	- `Headers` - Map of http headers, typically used with `fetch()`
	- `structuredClone` - Clone JavaScript objects.
	- `TextDecoder` - Convert binary buffers to JavaScript strings.
	- `TextEncoder` - Convert JavaScript strings to UTF-8 formatted buffers.
	- `URL` - URL parsing
	- `Worker`, `SharedWorker` - Use multiple independent JavaScript virtual machines (Web Workers).
- **Top-level await (TLA)**. The Moddable SDK supports TLA but requires it to be explicitly enabled in the project. This allows projects that do not use the feature to be smaller. `mcpack` detects when TLA is used and automatically sets the `MODDEF_MAIN_ASYNC` build flag.

The `package.json` support does not replace the Moddable SDK `manifest.json`, though for many projects `package.json` may be all that's needed. `mcpack` generates a `manifest.json` from the `package.json`, and manifests can still be used together with `package.json`.

## Running Examples
To run the package examples on the simulator, do the following:

```
cd $MODDABLE/examples/package/hello
npm install
mcpack mcconfig -d -m -p sim
```

The `npm install` is only necessary the first time you build, to retrieve any external dependencies. The `mcpack` tool invocation is followed by the usual `mcconfig`. To run the same project on an ESP32 Node-MCU board:

```
mcpack mcconfig -d -m -p esp32/nodemcu
```

To run a package that requires the network, such as `fetch`, specify the Wi-Fi credentials to mcconfig as usual:

```
mcpack mcconfig -d -m -p esp/moddable_one ssid="My Wi-Fi" password=secret
```

## Examples
The following section provides notes on each example. The notes describe some of the features of `package.json` and `mcpack` that each example demonstrates.

### balls
This is the package.json version of the classic Moddable SDK Piu [balls](https://github.com/Moddable-OpenSource/moddable/blob/public/examples/piu/balls/main.js) example. In addition to the `package.json` there is a `manifest.json` which is used to include the graphical assets and disable touch input handling. The manifest.json is included automatically based on its name and location in the same directory as the `package.json`.

Notice that main.js does:

```js
import {} from "piu/MC";
```

This import is automatically detected and the `manifest_piu.json` is included in the generated manifest:

```
> mcpack mcconfig -d -m
# mcpack include: $(MODDABLE)/examples/manifest_piu.json
```

### eventemitter
This example demonstrates use of the `@moddable/eventemitter3` package, a Moddable SDK port of the popular [`eventemitter3`](https://www.npmjs.com/package/eventemitter3) npm package.

### fetch
This example makes several HTTP requests using the `fetch()` API. In main.js, the application accesses `fetch`, `URL`, `Headers`, and `console.log` globals so these are automatically included by `mcpack`.

```
> mcpack mcconfig -d -m
# mcpack include: $(MODDABLE)/modules/data/url/manifest.json
# mcpack define: URL, URLSearchParams
# mcpack include: $(MODDABLE)/modules/data/headers/manifest.json
# mcpack define: Headers
# mcpack include: $(MODDABLE)/examples/io/tcp/fetch/manifest_fetch.json
# mcpack define: fetch
# mcpack define: console
# xsc globals.xsb
```

`mcpack` generates a `globals.js` file to initialize the global variables.

### hello
This example shows different ways that modules can be included in a project. This example uses external modules from npm, so it is necessary to do `npm install` in the directory to download those before running `mcpack`.

The `package.json` imports three packages from npm. Two of those are examples to show package import and mapping. The third is the popular mustache library, to demonstrate that modules implemented using standard JavaScript and supported dependencies can run on embedded devices, thanks to the ES2023 support of the XS JavaScript engine in the Moddable SDK.

The `imports` section of `package.json` shows two important features:

```json
"imports": {
	"#test": {
	  "moddable": "./test-xs.js",
	  "default": "./test-node.js"
	}
}
```

First, the `"#test"` import is a private relative specifier, so the module will only be available within this package. Second, the package.json can provide different implementations of a module depending on the target runtime. Here the key "moddable" means that test-xs.js is used when building for the Moddable SDK with `mcpack` and otherwise test-node.js is used. This is a powerful tool for organizing your project so that it can run on embedded targets as well as others.

### mod-balls
This is an example of using `mcpack` to build and run a [mod](https://www.moddable.com/documentation/xs/mods) using `mcrun`. Before running this example, it is necessary to run the `run-mod` example to install the host for the mod.

mod-balls is the classic [balls](https://github.com/Moddable-OpenSource/moddable/blob/public/examples/piu/balls/main.js) example as a mod using `package.json`. To run it on the simulator:

```
mcpack mcrun -d -m
```

Rather than use an external `manifest.json`, like balls above, to include the graphical assets, it embeds a manifest inside `package.json`. The choice of whether to use an embedded or external manifest is a matter of preferred style; there is no functional difference.

```json
	"moddable": {
		"manifest": {
			"resources": {
				"*": "./balls"
			}
		}
	}
```

When `mcpack` is used to build a mod, it does not perform any automatic imports or initialization of globals as it assumes the host will take care of that. Dependencies on the built-in Moddable SDK modules can still be included by adding them to the manifest.

### run-mod
This is a host for running [mods](https://www.moddable.com/documentation/xs/mods) that use the [Piu](https://www.moddable.com/documentation/piu/piu) user interface framework.

By itself run-mod doesn't do much. It relies on a mod to draw something. Once run-mod is installed, run the mod-balls example.

### timers
This example is a web-compatible variation of the [timers](https://github.com/Moddable-OpenSource/moddable/blob/public/examples/base/timers/main.js) example. Instead of using the `timer` module directly, it uses `setTimeout` (and friends) through globals. `mcpack` detects the use of these well-known globals and automatically adds implementations of these functions to globals.js to make them available to the script. The implementation of those globals uses the "timer" module.

### top-level-await
This example shows two different way that top-level await can be used, even in a projects `main.js`. The main module imports the "wow" module, which uses await at the top-level to initialize its return value. This promise is resolved before execution of main continues. Once main runs, it waits 500 ms for `setTimeout` and then outputs "done waiting."

### ts-hello
This example shows how TypeScript sources files can be used in a `package.json` using `mcpack` The `package.json` contains a "build" script to convert the TypeScript sources to JavaScript:

```
  "scripts": {
    "build": "tsc"
   }
```

To invoke this script as part of the `mcpack` build, use the following command line:

```
npm run build && mcpack mcconfig -d -m
```

There are many different ways that developers use to process the TypeScript sources. This is just one workflow.

The directory includes a `tsconfig.json` to configure the TypeScript compiler. This is important as it sets the target output of the TypeScript compiler to ES2022. Targeting earlier versions of JavaScript causes TypeScript to generate JavaScript that is less efficient and, in some cases, incompatible with embedded execution.

