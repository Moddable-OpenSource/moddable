# Manifest
Copyright 2017-2023 Moddable Tech, Inc.<BR>
Revised: April 9, 2023

A manifest is a JSON file that describes the modules and resources necessary to build a Moddable app. This document explains the properties of the JSON object and how manifests are processed by the Moddable SDK build tools.


## Table of Contents

* [Example](#example)
* [Properties](#properties)
	* [`build`](#build)
	* [`include`](#include)
		* [Including git repositories](#include-git)
	* [`creation`](#creation)
	* [`defines`](#defines)
	* [`config`](#config)
	* [`strip`](#strip)
	* [`modules`](#modules)
	* [`preload`](#preload)
	* [`resources`](#resources)
	* [`data`](#data)
	* [`platforms`](#platforms)
		* [`subplatforms`](#subplatforms)
	* [`bundle`](#bundle)
* [How manifests are processed](#process)

<a id="example"></a>
## Example

As a simple example, consider the manifest of the [balls example app](../../examples/piu/balls). It is quite short:

```json
{
	"include": [
		"$(MODDABLE)/examples/manifest_base.json",
		"$(MODDABLE)/examples/manifest_piu.json"
	],
	"modules": {
		"*": "./main"
	},
	"resources":{
		"*": [
			"./balls"
		]
	}
}
```

The `include` array lists other manifests to include. This is a convenient way to use the same properties and modules across many applications because it eliminates the need for you to specify each individual property and module in every application's manifest.  For example, `manifest_base.json` is included in all of our example applications. It is used so that resource, instrumentation, time, and timer modules are included in the build for each application.

The `modules` and `resources` objects list modules and resources that are specific to the balls app that should be included in the build. Here the `modules` object specifies that the `main.js` file located in the balls app directory should be included, and the `resources` object specifies that the `balls.png` image located in the balls app directory should be included.

<a id="properties"></a>
## Properties

<a id="build"></a>
### `build`

The `build` object defines the environment variables used to build paths in other paths in the manifest. Notice that the manifest can access shell environment variables like `MODDABLE`.

```json
"build": {
	"BUILD": "$(MODDABLE)/build",
	"MODULES": "$(MODDABLE)/modules",
	"COMMODETTO": "$(MODULES)/commodetto",
	"PIU": "$(MODULES)/piu"
}
```

When you build an application, the default output directory name is taken from the directory name containing the manifest. You can specify a different output directory name by specifying a `NAME` environment variable in the `build` object.

```json
"build": {
	"NAME": "balls"
}
```
	
#### ESP32-specific environment variables

The `esp32` platform object supports a number of optional environment variables applications can use to customize the Moddable SDK build for ESP32 and ESP32-S2:

| Variable | Description |
| --- | :--- | 
| `SDKCONFIGPATH` | Pathname to a directory containing custom [sdkconfig defaults](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html#custom-sdkconfig-defaults) entries. 
| `PARTITIONS_FILE` | Full pathname to a [partition table](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html) in CSV format.
| `BOOTLOADERPATH` | Pathname to a directory containing a custom [ESP-IDF bootloader component](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/bootloader.html#custom-bootloader).
| `C_FLAGS_SUBPLATFORM` | C compiler flags to use when compiling Moddable SDK sources.

> Note: This document does not cover native code ESP32 and ESP-IDF build details. Refer to the [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/get-started/index.html) for additional information.
 
The [modClock](https://github.com/Moddable-OpenSource/moddable/tree/public/contributed/modClock) example app leverages the `SDKCONFIGPATH` and `PARTITIONS_FILE` environment variables:

```json
"build": {
	"SDKCONFIGPATH": "$(MODDABLE)/contributed/modClock/sdkconfig",
	"PARTITIONS_FILE": "$(MODDABLE)/contributed/modClock/sdkconfig/partitions.csv"
},
```

In this example, the modClock [partitions.csv](https://github.com/Moddable-OpenSource/moddable/blob/public/contributed/modClock/sdkconfig/partitions.csv) file completely replaces the base Moddable SDK [partitions.csv](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj-esp32/partitions.csv) file at build time to provide additional partitions for OTA updates. The `sdkconfig` directory contains sdkconfig files that override and supplement the base Moddable SDK [sdkconfig.defaults](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj-esp32/sdkconfig.defaults) entries. The following section describes how the Moddable ESP32 build processes sdkconfig files.

The `C_FLAGS_SUBPLATFORM` environment variable is for use in manifests of subplatforms (and should not be used elsewhere). It allows compiler specific settings unique to a subplatform. For example, a subplatform using first generation ESP32 silicon with external PSRAM would enable the following settings to cause the compiler to generate code to work around the silicon bugs:

```json
"build": {
	"C_FLAGS_SUBPLATFORM": "-mfix-esp32-psram-cache-issue -mfix-esp32-psram-cache-strategy=memw"
},
```

#### How partitions.csv is processed
The [ESP-IDF partition table](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html) must contain certain partitions to support certain features:

- [Mods](../xs/mods.md) requires a partition to store the mod's archive
- Files requires a partition to store the file system
- Over-the-Air (OTA) updates require two OTA app partitions plus an OTA Data partition

The `mcconfig` tool can automatically modify the partition map to support these features. This simplifies development by eliminating the need to manually create a targeted partition table for projects. It makes optimal use of flash space, by only creating partitions for features that are used by the project.

The `mcconfig` tool creates new partitions by dividing the factory app partition based on the features used by the project. The default partition tables for Moddable SDK devices all have a single factory app partition and so support this feature of `mcconfig`.

To determine the features in-use, `mcconfig` checks the following manifest [defines](#defines). These defines are set in the manifests that require them, so it is usually unnecessary to set them in project manifests.

- Mods – if `XS_MODS` is set to a non-zero value, mods are considered to be in use

```json
"defines": {
	"XS_MODS": 1
}
```
- Files - if `file partition` is set to the name of a partition, files are considered to be in use

```json
"defines": {
	"file": {
		"partition": "#storage"
	}
}
```
- OTA – if `ota autospilt` is set, OTA is considered to be in-use.

```json
"defines": {
	"ota": {
		"autosplit": 1
	}
}
```
If the partitions.csv file for the project includes a partition for these features, `mcconfig` does not automatically create the corresponding partition. For example, if a mods partition is defined in the partitions.csv file, `mcconfig` does not create one from the factory app partition.

The partitions created have the following sizes. Options could be implemented in the future to configure these sizes.

- Mods - 256 KB
- Storage - 64 KB
- OTA - 8 KB is reserved for the OTA Data partition required by the ESP-IDF. The space in the factory app partition not used for other partitions is divided into two OTA app partitions.

#### How sdkconfig files are processed

The Moddable SDK sdkconfig defaults files are located in the `$MODDABLE/build/devices/esp32/xsProj-esp32` and `$MODDABLE/build/devices/esp32/xsProj-esp32s2` directories for ESP32 and ESP32-S2, respectively. The `sdkconfig.defaults` ([ESP32](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj-esp32/sdkconfig.defaults)/[ESP32-S2](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj-esp32s2/sdkconfig.defaults)) file is the base configuration file used by all ESP32/ESP32-S2 builds. Release and instrumented release builds merge additional configuration options, on top of the base `sdkconfig.defaults` file, from the `sdkconfig.defaults.release` ([ESP32](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj-esp32/sdkconfig.defaults.release)/[ESP32-S2](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj-esp32s2/sdkconfig.defaults.release)) and `sdkconfig.inst` ([ESP32](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj-esp32/sdkconfig.inst)/[ESP32-S2](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj-esp32s2/sdkconfig.inst)) files respectively. When merging, configuration options that exist in the base `sdkconfig.defaults` file are replaced and options that don't exist in the base `sdkconfig.defaults` file are added. The merge processing order is as follows:

1. All base `sdkconfig.defaults` options are applied to the build.
2. On release builds, the `sdkconfig.defaults.release` options are merged.
3. On release instrumented builds, the `sdkconfig.inst` options are merged.

	When applications specify optional sdkconfig files using the `SDKCONFIGPATH` manifest environment variable, the merge processing additionally includes the following:

4. The application `$(SDKCONFIGPATH)/sdkconfig.defaults` options, when provided, are merged.
5. On release builds, the application `$(SDKCONFIGPATH)/sdkconfig.defaults.release` options, when provided,  are merged.
6. On release instrumented builds, the application `$(SDKCONFIGPATH)/sdkconfig.inst` options, when provided, are merged.

***

<a id="include"></a>
### `include`

The `include` array lists other manifests to include. It's often convenient to include other manifests to avoid repeating common properties in every manifest. For example, the [`bmp280` example](../../examples/drivers/bmp280) includes `manifest_base.json` and the manifest for the BMP280 temperature/barometric pressure sensor:

```json
"include": [
	"$(MODDABLE)/examples/manifest_base.json",
	"$(MODULES)/drivers/bmp280/manifest.json"
]
```

Each example application in the Moddable SDK includes at least one of the manifests in the [examples directory](../../examples/).

- `manifest_base.json` is included by each of our example applications. It includes resource, instrumentation, time, and timer modules. It also includes a `creation` object that works for many applications. The [`creation` object](#creation) is covered in more detail later in this document.

- `manifest_commodetto.json` is for applications that use the [Commodetto graphics library](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/commodetto.md) directly.

- `manifest_net.json` is for applications that use Wi-Fi. It includes Socket, Net, SNTP, and Wi-Fi modules. It does not include specific networking protocols like HTTP and MQTT.

- `manifest_piu.json` is for applications that use the [Piu application framework](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md). It includes all the modules needed to use Piu. The screen and touch drivers are provided elsewhere, typically by the manifest of the target device itself, to keep the Piu manifest device independent.

Several touch, display, and sensor [drivers](../../modules/drivers) and some [networking modules](../../modules/network) also have manifests to make it easier to incorporate them into your projects.

<a id="include-git"></a>
#### Including Git Repositories
A manifest may directly include git repositories. The repositories are cloned as part of the build process and stored with the project's temporary build files.

> **Note**: This feature is experimental. While the intent is to keep the manifest JSON as-is for including git repositories, changes may be made based on feedback from experience using the feature.

Each git repository to fetch is specified by an object in the `include` array of a manifest:

- The object must have a `"git"` property, which is the git URL of the repo.
- The object can have an `"include"` property, which is the path of the manifest to include.

The default value of the `"include"` property is `"manifest.json"`. The `"include"` property can also be an array of paths in order to include several manifests from the same repository.

```json
{
	"build": {
		"URL":"https://github.com/moddable"
	},
	"include": [
		{
			"git":"$(URL)/test0.git"
		},
		{ 
			"git":"$(URL)/test1.git", 
			"include":"modules/test1/manifest.json"
		},
		{ 
			"git":"$(URL)/test23.git", 
			"include": [
				"test2/module.json",
				"test3/module.json"
			]
		}
	]
}
```

When processing a manifest, **mcconfig** and **mcrun** clone or pull the repositories into the `repos` directory in the project's temporary build files.

Specific branches and tags are accessed using the optional `branch` and `tag` properties:

```json
    {
		"git":"$(URL)/test0.git",
		"branch":"feature-test"
	},
    {
		"git":"$(URL)/test1.git",
		"tag":"3.5.0"
	},
    {
		"git":"$(URL)/test2.git",
		"tag":"3.5.0",
		"branch":"feature-test"
	},
```

The hostname, pathname. branch, and tag are included in the path where the cloned repositories are stored to avoid conflicts.

> **Note**: Cloned repositories are deleted when the project is cleaned (`mcconfig -d -m -t clean`). Therefore, the cloned repositories should not be edited.

***

<a id="creation"></a>
### `creation`

The `creation` object defines the creation parameters of the XS machine that will run the application. The values used in `manifest_base.json` work for many applications, including the vast majority of example applications in the Moddable SDK.

```json
"creation": {
	"static": 32768,
	"chunk": {
		"initial": 1536,
		"incremental": 512
	},
	"heap": {
		"initial": 512,
		"incremental": 64
	},
	"stack": 256,
	"keys": {
		"initial": 32,
		"incremental": 0,
		"name": 53,
		"symbol": 3
	},
	"main": "main"
},
```

These values correspond to machine allocation values [described](../xs/XS%20in%20C.md#machine-allocation) in the XS in C documentation (the sole exception is the `main` property, which is the module specifier of the module to load following the [set-up phase](../base/setup.md)). Take care when changing these values as configuring them improperly can result in an unstable or unusable system. Bigger values are not always better, especially on devices with limited resources.

#### `static` memory allocations
The `static` property is the most important for microcontrollers. It is the total number of bytes that can be used by the JavaScript language runtime, including the stack, objects, byte-code, strings, etc. It is allocated as a single block of memory to minimize bookkeeping overhead and to allow the runtime to dynamically manage areas for fixed size slots and variable sized chunks. The `static` property also imposes a strict limit on the memory allocated by the language runtime to guarantee that scripts cannot exceed their memory budget (if they could, a script could take memory required by the host OS leading to failures and instabilities).

The `static` property is ignored by the simulator. The simulator falls back to on-demand memory allocation. Since computers have nearly infinite memory compared to microcontrollers, this isn't a problem.

There are some situations where the `static` allocation technique of dynamically managing a single block of memory isn't the optimal choice. The first is on microcontrollers where RAM is divided across two or more discontiguous address ranges (the ESP32 is a common example). In this case, using a single block of memory for the virtual machine prevents it from using some of the available RAM. The second is when the application has been carefully tuned to work with specific sized slot and chunk pools. Done correctly, such tuning can improve performance by reducing the frequency of garbage collections, particularly at launch. For these situations, do the following:

- Set the `static` property to `0` to disable the static allocator. (It is not enough to delete the property as the `static` property may be defined in an included manifest)
- Set the `chunk` `initial` property to the size of the the chunk heap in bytes
- Set the `heap` `initial` property to the number of slots (on a 32-bit MCU, each slot is 16 bytes)
- Set the `chunk` `incremental` and `heap` `incremental` properties to `0`. (this prevents the slot and chunk heaps from growing beyond their initial allocations)

Using this approach, the memory allocator on the microcontroller allocates the following:

- one memory block for the stack (the stack must be contiguous)
- one memory block for chunks (this eliminates memory lost to chunk fragmentation and allows allocating the largest possible blocks from JavaScript)
- one or more memory blocks for the slot heap

> **Note**: The microcontroller runtime could be enhanced to allocate the chunk heap across multiple memory blocks; to-date this has not been necessary. The chunk heap is allocated before the stack and slot heap, allowing it to allocate the largest possible contiguous free block.

#### `keys`
the VM allocates space for `keys.initial` runtime keys when the VM is initialized. For embedded projects, this number should be small as most keys are allocated when building, not at runtime. To prevent more keys than this being allocated at runtime, set `keys.incremental` to `0`. To allow additional keys to be allocated, provide a non-zero value for `keys.incremental`.

The `keys` property previously contained an `available` property for the total number of keys that could be allocated at runtime. If `keys.initial` is not provided, the value of `keys.available` is used with `keys.incremental` of `0`.

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

```json
"config": {
	"screen": "ili9341",
	"touch": "ft6206",
	"rotation": 90
}
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

The content of the `config` object may be overridden by adding key value pairs to the command line provided to `mcconfig`. See the Arguments section of the [Tools document](./tools.md#arguments) for details.

***

<a id="strip"></a>
### `strip`

Most applications do not use all built-in JavaScript language features. To save ROM, the XS linker can strip unused native code from the XS engine itself. For more information about the strip feature, see the **Strip unused features** section of the [XS Differences](../xs/XS%20Differences.md) document.

The `strip` object in a manifest is a string or array that specifies which built-in objects and functions of the JavaScript language can/should be removed by the XS linker.

- `"*"` means anything unused by the application can be stripped. This is the value used by `manifest_base.json`.

	```json
	"strip": "*",
	```
	
- If you only want certain objects or functions to be stripped, pass in an array of JavaScript class and function names. Items in the array will be stripped. Anything not included in the array will not be stripped. 

	The following strips the `RegExp` class, `eval` function, and the two `Array` reduce functions.

	```json
	"strip": [
		"RegExp",
		"eval",
		"Array.prototype.reduce",
		"Array.prototype.reduceRight"
	]
	```

- You can also specify that specific items be stripped in addition to anything unused.

	The `"*"` means to strip everything unused. Because the two `Array` reduce functions are explicitly listed, they will also be stripped, whether or not they are used.
	
	```json
	"strip": [
		"*",
		"Array.prototype.reduce",
		"Array.prototype.reduceRight"
	]
	```

If an application attempts to use a class or function that has been stripped, it will throw a `dead strip!` error.

***

<a id="modules"></a>
### `modules`

The `modules` object specifies which modules are included in the build. Every module used by an application must be listed in the `modules` object.

The `*` parameter of the `modules` object is an array of paths.

```json
"modules": {
	"*": [
		"./main",
		"./assets"
	]
},
```

These modules are imported into other script modules using the file name.

```js
import ASSETS from "assets"
```

If you want to use a different name with the `import` statement, you can include additional key/value pairs to the `modules` object where the key is the new name and the value is the path to the file. For example:

```json
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

```json
"preload": [
	"main",
	"assets"
]
```

***

<a id="resources"></a>
### `resources`

The `resources` object specifies which resources (image, font, and audio files) are included in the build.

#### Images

Image assets may be in the GIF, JPEG, and PNG image file formats.

GIF and JPEG files should be included in the `*` array.

```json
"resources":{
	"*": [
		"$(MODDABLE)/examples/assets/images/screen2"
	]
},
```

PNG files are converted by `png2bmp`. This converts them into BMP files that Moddable apps can use directly from flash storage. `png2bmp` can convert to alpha bitmaps and color bitmaps.

- Files listed in the `*` array will be converted to 8-bit alpha and 16-bit color bitmaps.

- Files listed in the `*-color` array will only be converted to 16-bit color bitmaps.

- Files listed in the `*-alpha` array will only be converted to 8-bit alpha bitmaps.

- Files listed in the `*-mask` array will be converted to 4-bit alpha bitmaps.

#### Fonts

The Moddable SDK uses bitmap fonts. The metrics are provided by binary FNT files, the glyphs are provided by PNG files. The glyph files are converted like all other PNGs (using `png2bmp`) so you can include fonts in the `*`, `*-alpha`, `*-color`, or `*-mask` arrays.

```json
"resources":{
	"*-mask": [
		"$(MODDABLE)/examples/assets/fonts/OpenSans-Semibold-28"
	]
},
```

For more information about creating fonts for Moddable applications, see the [font documentation](../commodetto/Creating%20fonts%20for%20Moddable%20applications.md).

#### Audio

The [`AudioOut` module](../pins/audioout.md) requires that audio be provided either as a MAUD audio resource or as raw audio samples. WAV files are automatically converted to the `maud` format using the `wav2maud` tool of the Moddable SDK, and compressed using IMA ADCPM.

Wave audio files should be included in the `*` array.

```json
"resources":{
	"*": [
		"$(MODDABLE)/examples/assets/sounds/bflatmajor"
	]
}
```

***

<a id="data"></a>
### `data`

The `data` object specifies resources to be included in the build. Unlike resources in the `resources` object, resources specified in the `data` object are never transformed in any way by the Moddable SDK build tools. You can use the `data` object to include resources like TLS certificates and JSON files.

#### TLS Certificates

`SecureSocket` objects use TLS certificates in DER (binary) format. The certificate store is located in the `$MODDABLE/modules/crypt/data` directory, or you can include your own certificates. Any valid certificate in DER format will work.

TLS certificates should be included in the `*` array.

```json
"data":{
	"*": [
		"$(MODULES)/crypt/data/ca170"
	]
}
```

***

<a id="platforms"></a>
### `platforms`

The `platforms` object allows you to specify properties that should only apply to specific platforms and subplatforms. The `platforms` object has one property for each platform or subplatform. Each property can have an `include` array, a `modules` object, a `preload` array, a `resources` object, a `defines` object, and a `recipes` object that will be used only when that platform is the goal of the build.

This is useful when the implementation of a module varies from platform to platform. For example, the `platforms` object below comes from the digital manifest. The implementation of GPIO requires the use of native code, and is different on each platform. This ensures that the correct modules are used for each target device.

```json
"platforms": {
	"esp": {
		"modules": {
			"*": "$(MODULES)/pins/digital/esp/*"
		}
	},
	"esp32": {
		"modules": {
			"*": "$(MODULES)/pins/digital/esp32/*"
		}
	},
	"gecko": {
		"modules": {
			"*": "$(MODULES)/pins/digital/gecko/*"
		}
	},
	"qca4020": {
		"modules": {
			"*": "$(MODULES)/pins/digital/qca4020/*"
		}
	},
	"...": {
		"error": "pins/digital module unsupported"
	}
}
```

The "`...`" platform identifier is a fallback for when no matching platform is found. This is useful for errors and warnings.

For example, if the `platforms` object of a manifest is as follows, building for the `esp` platform will not generate a warning/error, building for the `esp32` platform will generate a warning, and building for any other platform will generate an error.

```json
"platforms":{
	"esp": {
		/* modules and resources for ESP8266 go here */
	},
	"esp32": {
		"warning": "module XYZ not fully tested on esp32",
		/* modules and resources for ESP32 go here */
	},
	"..." {
		"error": "module XYZ unsupported"
	}
}
```

<a id="subplatforms"></a>
#### Subplatforms

A subplatform is used to configure an application for the variations in a product family. They are useful when devices are similar and share much of the same configuration, but have slight differences.

For example, there are subplatforms for each Gecko device supported by the Moddable SDK. In the segment below, the `timer` module is specified for all `gecko` platforms. The `wakeup` pin is defined differently for the `gecko/giant` and `gecko/mighty` subplatforms.

```json
"platforms": {
    "gecko": {
        "modules": {
            "*": [
                "$(MODULES)/base/timer/*",
                "$(MODULES)/base/timer/mc/*"
            ]
        },
        "preload": [
            "timer"
        ]
    },
    "gecko/giant": {
        "defines": {
            "sleep": {
                "wakeup": { "pin": 2, "port": "gpioPortF", "level": 0, "register": "GPIO_EM4WUEN_EM4WUEN_F2" }
            }
	     }
	 },
    "gecko/mighty": {
        "defines": {
            "sleep": {
                "wakeup": { "pin": 7, "port": "gpioPortF", "level": 0, "register": "GPIO_EXTILEVEL_EM4WU1" }
            }
         }
    }
}
```

The `SUBPLATFORM` variable is automatically defined by `mcconfig`. A wildcard is a available to match on subplatforms. The `SUBPLATFORM` variable and wildcard match used together simplify inclusion of subplatform manifests:

```json
	"platforms": {
		"esp32/*": {
			"include": "./targets/$(SUBPLATFORM)/manifest.json"
		}
	}
```
***

<a id="bundle"></a>
### `bundle`

The `bundle` object is used by the [`mcbundle` command line tool](./tools.md#mcbundle) to build and package app archives for the Moddable Store. It has the following properties:

| Property | Required | Description |
| :---: | :---: | :--- |
| `id` | ✓ | The app signature. We typically use the app signature `tech.moddable.` + the name of the app, for example `tech.moddable.balls` for an app called `balls`.
| `devices` | ✓ | An array of platform identifiers or device signatures that support the app.<BR><BR>You can also use wildcards (e.g. `esp/*` or `esp32/*`).
| `custom` | | The path to the `custom` directory for the app's configurable preferences, if any.
| `icon` | | The path to the custom app icon, if any. The app icon is the image that shows up next to the app name in the Moddable Store. The Moddable Store supplies a default icon based on the Moddable logo.

```json
"bundle": {
    "id": "tech.moddable.countdown",
    "devices": [
        "esp/moddable_one",
        "com.moddable.two"
    ],
    "custom": "./store/custom",
    "icon": "./store/icon.png"
}
```

***
<a id="process"></a>
## How manifests are processed

`mcconfig` is a command line tool that generates a make file based on a manifest, then runs `make` to build and launch Moddable apps on microcontrollers or in the simulator. `mcconfig` processes a manifest in three passes: combine, match, and generate.

### Combine

The first pass combines the properties of the included manifests (in depth-first order) with the properties of the processed manifest. For each manifest, the common properties are combined with the properties of the platform that is the goal of the build. Subplatform properties are combined with platform properties.

In the `modules` and `resources` objects, paths can be relative to the manifest that defines them.

When combining properties with the same name, the combined value is the concatenation of the two values. For example, consider the following snippet of a manifest for an application. The `include`  object specifies that the properties from `manifest_base.json` should be included. The `modules` object specifies just one module called `main`.

```json
{
	"include": [
		"$(MODDABLE)/examples/manifest_base.json"
	],
	"modules": {
		"*": "./main"
	},
	/* other manifest properties here */
}
```

The `modules` object from `manifest_base.json` includes additional modules from the `$MODDABLE/modules` directory.

```json
	"modules": {
		"*": [
			"$(MODULES)/files/resource/*",
			"$(MODULES)/base/instrumentation/*"
		]
	},
```

The concatenation of the two `modules` objects includes the `main` module from the application, and the resource and instrumentation modules specified in `manifest_base.json`.

```json
"modules": {
	"*": [
		"./main",
		"$(MODULES)/files/resource/*",
		"$(MODULES)/base/instrumentation/*"
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

```json
"platforms": {
	"gecko/*": {
		"build": {
			"MAKE_FRAGMENT": "$(BUILD)/devices/gecko/targets/$(SUBPLATFORM)/make.$(SUBPLATFORM).mk"
		}
		"include": "./targets/$(SUBPLATFORM)/manifest.json"
	}
}
```
