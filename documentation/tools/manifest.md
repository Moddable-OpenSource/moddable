# Manifest

Copyright 2017-2019 Moddable Tech, Inc.<BR>
Revised: August 28, 2020

A manifest is a JSON file that describes the modules and resources necessary to build a Moddable app. This document explains the properties of the JSON object and how manifests are processed by the Moddable SDK build tools.


## Table of Contents

* [Example](#example)
* [Properties](#properties)
	* [`build`](#build)
	* [`include`](#include)
	* [`creation`](#creation)
	* [`strip`](#strip)
	* [`modules`](#modules)
	* [`preload`](#preload)
	* [`resources`](#resources)
	* [`data`](#data)
	* [`platforms`](#platforms)
* [How manifests are processed](#process)

<a id="example"></a>
## Example

As a simple example, consider the manifest of the [balls example app](../../examples/piu/balls). It is quite short:

```
{
	"include": [
		"$(MODDABLE)/examples/manifest_base.json",
		"$(MODDABLE)/examples/manifest_piu.json",
	],
	"modules": {
		"*": "./main",
	},
	"resources":{
		"*": [
			"./balls",
		]
	},
}
```

The `include` array lists other manifests to include. This is a convenient way to use the same properties and modules across many applications because it eliminates the need for you to specify each individual property and module in every application's manifest.  For example, `manifest_base.json` is included in all of our example applications. It is used so that resource, instrumentation, time, and timer modules are included in the build for each application.

The `modules` and `resources` objects list modules and resources that are specific to the balls app that should be included in the build. Here the `modules` object specifies that the `main.js` file located in the balls app directory should be included, and the `resources` object specifies that the `balls.png` image located in the balls app directory should be included.

<a id="properties"></a>
## Properties

<a id="build"></a>
### `build`

The `build` object defines the environment variables used to build paths in other paths in the manifest. Notice that the manifest can access shell environment variables like `MODDABLE`.

```js
"build": {
	"BUILD": "$(MODDABLE)/build",
	"MODULES": "$(MODDABLE)/modules",
	"COMMODETTO": "$(MODULES)/commodetto",
	"PIU": "$(MODULES)/piu",
},
```

#### `ESP32-specific environment variables`

The `esp32` platform object supports a number of optional environment variables applications can use to customize the Moddable SDK ESP32 build:

| Variable | Description |
| --- | :--- | 
| `SDKCONFIGPATH` | Pathname to a directory containing custom [sdkconfig defaults](https://docs.espressif.com/projects/esp-idf/en/v3.3.2/api-guides/build-system-cmake.html?highlight=sdkconfig#custom-sdkconfig-defaults) entries. 
| `PARTITIONS_FILE` | Full pathname to a [partiion table](https://docs.espressif.com/projects/esp-idf/en/v3.3.2/api-guides/partition-tables.html#) in CSV format

> Note: This document does not cover native code ESP32 and ESP-IDF build details. Refer to the [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/v3.3.2/get-started/index.html) for additional information.
 
The [modClock](https://github.com/Moddable-OpenSource/moddable/tree/public/contributed/modClock) example app leverages both environment variables:

```js
"build": {
	"SDKCONFIGPATH": "$(MODDABLE)/contributed/modClock/sdkconfig",
	"PARTITIONS_FILE": "$(MODDABLE)/contributed/modClock/sdkconfig/partitions.csv"
},
```

In this example, the modClock [partitions.csv](https://github.com/Moddable-OpenSource/moddable/blob/public/contributed/modClock/sdkconfig/partitions.csv) file completely replaces the base Moddable SDK [partiions.csv](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj/partitions.csv) file at build time to provide additional partitions for OTA updates. The `sdkconfig` directory contains sdkconfig files that override and supplement the base Moddable SDK [sdkconfig.defaults](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj/sdkconfig.defaults) entries. The following section describes how the Moddable ESP32 build processes sdkconfig files.

#### How sdkconfig files are processed

The Moddable SDK sdkconfig defaults files are located in the `$MODDABLE/build/devices/esp32/xsProj` directory. The [sdkconfig.defaults](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj/sdkconfig.defaults) file is the base configuration file used by all ESP32 builds. Release and instrumented release builds merge additional configuration options, on top of the base `sdkconfig.defaults` file, from the [sdkconfig.defaults.release](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj/sdkconfig.defaults.release) and [sdkconfig.inst](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj/sdkconfig.inst) files respectively. When merging, configuration options that exist in the base sdkconfig.defaults file are replaced and options that don't exist in the base sdkconfig.defaults file are added. The merge processing order is as follows:

1. All base `sdkconfig.defaults` options are applied to the build.
2. On release builds, the `sdkconfig.defaults.release` options are merged on top of the `sdkconfig.defaults` options.
3. On release instrumented builds, the `sdkconfig.inst` options are merged on top of the merge performed in step 2.

	When applications specify optional sdkconfig files using the `SDKCONFIGPATH` manifest environment variable, the merge processing additionally includes the following:

4. On debug builds, the application `sdkconfig.defaults` file, when provided, is merged on top of the base Moddable SDK `sdkconfig.defaults` file.
5. On release builds, the application `sdkconfig.defaults.release` options, when provided,  are merged on top of the merge performed in step 2.
6. On release instrumented builds, the `sdkconfig.inst` options, when provided, are merged on top of the merge performed in step 5.

***

<a id="include"></a>
### `include`

The `include` array lists other manifests to include. It's often convenient to include other manifests to avoid repeating common properties in every manifest. For example, the [`bmp280` example](../../examples/drivers/bmp280) includes `manifest_base.json` and the manifest for the BMP280 temperature/barometric pressure sensor:

```
"include": [
	"$(MODDABLE)/examples/manifest_base.json",
	"$(MODULES)/drivers/bmp280/manifest.json",
],
```

Each example application in the Moddable SDK includes at least one of the manifests in the [examples directory](../../examples/).

- `manifest_base.json` is included by each of our example applications. It includes resource, instrumentation, time, and timer modules. It also includes a `creation` object that works for many applications. The [`creation` object](#creation) is covered in more detail later in this document.

- `manifest_commodetto.json` is for applications that use the [Commodetto graphics library](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/commodetto.md) directly.

- `manifest_net.json` is for applications that use Wi-Fi. It includes Socket, Net, SNTP, and Wi-Fi modules. It does not include specific networking protocols like HTTP and MQTT.

- `manifest_piu.json` is for applications that use the [Piu application framework](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md). It includes all the modules needed to use Piu. The screen and touch drivers are provided elsewhere, typically by the manifest of the target device itself, to keep the Piu manifest device independent.

Several touch, display, and sensor [drivers](../../modules/drivers) and some [networking modules](../../modules/network) also have manifests to make it easier to incorporate them into your projects.

***

<a id="creation"></a>
### `creation`

The `creation` object defines the creation parameters of the XS machine that will run the application. The values used in `manifest_base.json` work for many applications, including the vast majority of example applications in the Moddable SDK.

```js		
"creation": {
	"static": 32768,
	"chunk": {
		"initial": 1536,
		"incremental": 512,
	},
	"heap": {
		"initial": 512,
		"incremental": 64,
	},
	"stack": 256,
	"keys": {
		"available": 32,
		"name": 53,
		"symbol": 3,
	},
	"main": "main",
},
```
	
***

<a id="defines"></a>
### `defines`

The `defines` object creates a set of C language #define preprocessor statements. It is designed to configure the C language implementation of hardware drivers.

The `defines` object is explained more fully in the [defines document](../tools/defines.md).

***

<a id="config"></a>
### `config`

The `config` object contains values that are accessible to the application's scripts.

It is commonly used in the manifests of target platforms to specify the screen driver, touch driver, and default rotation. These properties are used by [Commodetto](../commodetto/commodetto.md) and [Piu](../piu/piu.md) setup modules.

```
"config": {
	"screen": "ili9341",
	"touch": "ft6206",
	"rotation": 90,
},
```

It is also used in the [files manifest](../../modules/files/file/manifest.json) to specify the file system root for each platform, some networking examples to specify Wi-Fi credentials, and more.

To access the `config` object in an application's scripts, import it. For example:

```js
import config from "mc/config";

if (!config.ssid) {
	trace("No Wi-Fi SSID\n");
	return;
}
```

The `config` object in the `mc/config` module is frozen, preventing its values from being changed at runtime.

***

<a id="strip"></a>
### `strip`

Most applications do not use all built-in JavaScript language features. To save ROM, the XS linker can strip unused native code from the XS engine itself. For more information about the strip feature, see the **Strip unused features** section of the [XS Differences](../xs/XS%20Differences.md) document.

The `strip` object in a manifest is a string or array that specifies which built-in objects and functions of the JavaScript language can/should be removed by the XS linker.

- `"*"` means anything unused by the application can be stripped. This is the value used by `manifest_base.json`.

	```js
	"strip": "*",
	```
	
- If you only want certain objects or functions to be stripped, pass in an array of JavaScript class and function names. Items in the array will be stripped. Anything not included in the array will not be stripped. 

	The following strips the `RegExp` class, `eval` function, and the two `Array` reduce functions.

	```js
	"strip": [
		"RegExp”,
		"eval",		
		“Array.prototype.reduce”,
		“Array.prototype.reduceRight”,
	]
	```

- You can also specify that specific items be stripped in addition to anything unused.

	The `"*"` means to strip everything unused. Because the two `Array` reduce functions are explicitly listed, they will also be stripped, whether or not they are used.
	
	```js
	"strip": [
		“*”,
		“Array.prototype.reduce”,
		“Array.prototype.reduceRight”,
	]
	```

If an application attempts to use a class or function that has been stripped, it will throw a `dead strip!` error.

***

<a id="modules"></a>
### `modules`

The `modules` object specifies which modules are included in the build. Every module used by an application must be listed in the `modules` object.

The `*` parameter of the `modules` object is an array of paths.

```js
"modules": {
	"*": [
		"./main",
		"./assets"
	],
},
```

These modules are imported into other script modules using the file name.

```js
import ASSETS from "assets"
```

If you want to use a different name with the `import` statement, you can include additional key/value pairs to the `modules` object where the key is the new name and the value is the path to the file. For example:

```js
"modules": {
	"*": [
		"./main",
	],
	"newNameForAssets": "./assets"
},
```

These modules are imported into other script modules using the name given by the manifest.

```js
import ASSETS from "newNameForAssets"
```

***

<a id="preload"></a>
### `preload`

Preloading of modules is a unique feature of the XS JavaScript engine. Preloading executes parts of a JavaScript application during the the build process, before the application is downloaded to the target device. For more information about preloading, see the [preloading document](../xs/preload.md).

The `preload` array in a manifest lists the modules preloaded in the read-only XS virtual machine that will be cloned to run the app. It is an array of module names, not paths to modules.

```js
"preload": [
	"main",
	"assets",
],
```

***

<a id="resources"></a>
### `resources`

The `resources` object specifies which resources (image, font, and audio files) are included in the build.

#### Images

Image assets may be in the GIF, JPEG, and PNG image file formats.

GIF and JPEG files should be included in the `*` array.

```
"resources":{
	"*": [
		"$(MODDABLE)/examples/assets/images/screen2",
	],
},
```

PNG files are converted by `png2bmp`. This converts them into BMP files that Moddable apps can use directly from flash storage. `png2bmp` can convert to alpha bitmaps and color bitmaps.

- Files listed in the `*` array will be converted to 8-bit alpha and 16-bit color bitmaps.

- Files listed in the `*-color` array will only be converted to 16-bit color bitmaps.

- Files listed in the `*-alpha` array will only be converted to 8-bit alpha bitmaps.

- Files listed in the `*-mask` array will be converted to 4-bit alpha bitmaps.

#### Fonts

The Moddable SDK uses bitmap fonts. The metrics are provided by binary FNT files, the glyphs are provided by PNG files. The glyph files are converted like all other PNGs (using `png2bmp`) so you can include fonts in the `*`, `*-alpha`, `*-color`, or `*-mask` arrays.

```
"resources":{
	"*-mask": [
		"$(MODDABLE)/examples/assets/fonts/OpenSans-Semibold-28",
	],
},
```

For more information about creating fonts for Moddable applications, see the [font documentation](../commodetto/Creating%20fonts%20for%20Moddable%20applications.md).

#### Audio

The [`AudioOut` module](../pins/audioout.md) requires that audio be provided either as a MAUD audio resource or as raw audio samples. WAV files are automatically converted to the `maud` format using the `wav2maud` tool of the Moddable SDK, and compressed using IMA ADCPM.

Wave audio files should be included in the `*` array.

```
"resources":{
	"*": [
		"$(MODDABLE)/examples/assets/sounds/bflatmajor",
	],
},
```

***

<a id="data"></a>
### `data`

The `data` object specifies resources to be included in the build. Unlike resources in the `resources` object, resources specified in the `data` object are never transformed in any way by the Moddable SDK build tools. You can use the `data` object to include resources like TLS certificates and JSON files.

#### TLS Certificates

`SecureSocket` objects use TLS certificates in DER (binary) format. The certificate store is located in the `$MODDABLE/modules/crypt/data` directory, or you can include your own certificates. Any valid certificate in DER format will work.

TLS certificates should be included in the `*` array.

```
"data":{
	"*": [
		"$(MODULES)/crypt/data/ca170",
	],
},
```

***

<a id="platforms"></a>
### `platforms`

The `platforms` object allows you to specify properties that should only apply to specific platforms and subplatforms. The `platforms` object has one property for each platform or subplatform. Each property can have an `include` array, a `modules` object, a `preload` array, a `resources` object, a `defines` object, and a `recipes` object that will be used only when that platform is the goal of the build.

This is useful when the implementation of a module varies from platform to platform. For example, the `platforms` object below comes from the digital manifest. The implementation of GPIO requires the use of native code, and is different on each platform. This ensures that the correct modules are used for each target device.

```js
"platforms": {
	"esp": {
		"modules": {
			"*": "$(MODULES)/pins/digital/esp/*",
		},
	},
	"esp32": {
		"modules": {
			"*": "$(MODULES)/pins/digital/esp32/*",
		},
	},
	"gecko": {
		"modules": {
			"*": "$(MODULES)/pins/digital/gecko/*",
		},
	},
	"qca4020": {
		"modules": {
			"*": "$(MODULES)/pins/digital/qca4020/*",
		},
	},
	"...": {
		"error": "pins/digital module unsupported"
	}
}
```

The "`...`" platform identifier is a fallback for when no matching platform is found. This is useful for errors and warnings.

For example, if the `platforms` object of a manifest is as follows, building for the `esp` platform will not generate a warning/error, building for the `esp32` platform will generate a warning, and building for any other platform will generate an error.

```
"platforms":{
	"esp": {
		/* modules and resources for ESP8266 go here */
	},
	"esp32": {
		"warning": "module XYZ not fully tested on esp32",
		/* modules and resources for ESP32 go here */
	},
	"..." {
		"error": "module XYZ unsupported",
	}
}
```

#### Subplatforms

A subplatform is used to configure an application for the variations in a product family. They are useful when devices are similar and share much of the same configuration, but have slight differences.

For example, there are subplatforms for each Gecko device supported by the Moddable SDK. In the segment below, the `timer` module is specified for all `gecko` platforms. The `wakeup` pin is defined differently for the `gecko/giant` and `gecko/mighty` subplatforms.

```
"platforms":{
    "gecko": {
        "modules": {
            "*": [
                "$(MODULES)/base/timer/*",
                "$(MODULES)/base/timer/mc/*",
            ]
        },
        "preload": [
            "timer",
        ],
    },
    "gecko/giant": {
        "defines": {
            "sleep": {
                "wakeup": { "pin": 2, "port": "gpioPortF", "level": 0, "register": "GPIO_EM4WUEN_EM4WUEN_F2" },
            },
	     },
	 },
    "gecko/mighty": {
        "defines": {
            "sleep": {
                "wakeup": { "pin": 7, "port": "gpioPortF", "level": 0, "register": "GPIO_EXTILEVEL_EM4WU1" },
            },
         },
      },
}
```      
The `SUBPLATFORM` variable is automatically defined by `mcconfig`. A wildcard is a available to match on subplatforms. The `SUBPLATFORM` variable and wildcard match used together simplify inclusion of subplatform manifests:

	"platforms": {
		"esp32/*": {
			"include": "./targets/$(SUBPLATFORM)/manifest.json"
		},
	}

***

<a id="process"></a>
## How manifests are processed

`mcconfig` is a command line tool that generates a make file based on a manifest, then runs `make` to build and launch Moddable apps on microcontrollers or in the simulator. `mcconfig` processes a manifest in three passes: combine, match, and generate.

### Combine

The first pass combines the properties of the included manifests (in depth-first order) with the properties of the processed manifest. For each manifest, the common properties are combined with the properties of the platform that is the goal of the build. Subplatform properties are combined with platform properties.

In the `modules` and `resources` objects, paths can be relative to the manifest that defines them.

When combining properties with the same name, the combined value is the concatenation of the two values. For example, consider the following snippet of a manifest for an application. The `include`  object specifies that the properties from `manifest_base.json` should be included. The `modules` object specifies just one module called `main`.

```
{
	"include": [
		"$(MODDABLE)/examples/manifest_base.json",
	],
	"modules": {
		"*": "./main"
	},
	/* other manifest properties here
}
```

The `modules` object from `manifest_base.json` includes additional modules from the `$MODDABLE/modules` directory.

```
	"modules": {
		"*": [
			"$(MODULES)/files/resource/*",
			"$(MODULES)/base/instrumentation/*",
		],
	},
```

The concatenation of the two `modules` objects includes the `main` module from the application, and the resouce and instrumentation modules specified in `manifest_base.json`.

```
"modules": {
	"*": [
		"./main",
		"$(MODULES)/files/resource/*",
		"$(MODULES)/base/instrumentation/*",
	]
}
```

### Match

The second pass matches files with the properties of the combined `modules` and `resources` objects:

- The `~` property contains files to exclude from the build.
- The name of each property is the target file, the value of the property is the source file or an array of source files.
- Targets and sources can use the `*` wildcard to represent all files that match.

There are no extensions:

- In the `modules` object, `mcconfig` matches `.c`, `.cc`, `.cpp`, `.h`, `.js` and `.m` files. 
- In the `resources` object, `mcconfig` matches `.act`, `.bmp`, `.cct`, `.dat`, `.der`, `.fnt`, `.jpg`, `.json`, `.nfnt`, `.pk8`, `.png`, `.rle`, `.ski` and `.ttf`  files. 

### Generate

The third pass generates make variables and rules for matched files:

- The .js files are compiled with `xsc` and linked with `xsl`.
- The modules listed in the `preload` properties are preloaded by `xsl` in the read-only XS virtual machine that will be cloned to run the app.
- The .png files are converted by `png2bmp`. Use the `*-color` and `*-alpha` pseudo target to get only color or only alpha bitmaps.
- The .h files define header dependencies and include directories.
- The .c files are compiled and linked with $(CC) unless they match the value of a property of the combined `recipes` object, then they use the make recipe corresponding to the name of the property.

The generated make file contains the generated variables, then the make fragment corresponding to the goal of the build, then the generated rules.

You will find the default make fragments and the make recipes in `$(MODDABLE)/tools/mcconfig`. It is the make fragments that define the compiler options, the linker options, the libraries, etc.

The make fragment can be specified in a platform's `build` section.

	"platforms": {
		"gecko/*": {
			"build": {
				"MAKE_FRAGMENT": "$(BUILD)/devices/gecko/targets/$(SUBPLATFORM)/make.$(SUBPLATFORM).mk",
			}
			"include": "./targets/$(SUBPLATFORM)/manifest.json"
		},
	}

