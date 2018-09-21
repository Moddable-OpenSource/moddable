# Manifest

Copyright 2017-2018 Moddable Tech, Inc.

Revised: September 21, 2018

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## Overview

A manifest is a JSON file that describes the modules and resources necessary to build a Moddable app.

### `$MODDABLE/examples/piu/balls/manifest.json`

Let us look at the manifest of the [balls example app](../../examples/piu/balls). It is quite short:

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
			"./main",
			"./balls",
		]
	},
}
```

The `include` array lists other manifests to include. Most apps have many common properties, so instead of repeating common properties in every manifest, our examples include some subset of the manifests in the [examples folder](../../examples/).

- `manifest_base.json` is included in all of our example applications. It includes resource, instrumentation, time, and timer modules and a `creation` object that works for many applications. The `creation` object is covered in more detail later in this document.

- `manifest_commodetto.json` is for applications that use the [Commodetto graphics library](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/commodetto.md).

- `manifest_net.json` is for applications that use Wi-Fi. It includes Socket, Net, SNTP, and Wi-Fi modules. It does not include specific networking protocols like HTTP and MQTT.

- `manifest_piu.json` is for applications that use the [Piu application framework](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md). It includes all the modules needed to use Piu. The screen and touch drivers are provided elsewhere, typically by the manifest of the target device itself, to allow the Piu manifest to be device independent.

The `modules` and `resources` objects contain the module and resources that are specific to the balls app. 

### `$(MODDABLE)/examples/manifest_base.json`

Now let us look at one of the included manifests: `manifest_base.json`. It is quite long so let us split it into its properties...

#### `build`

The `build` object contains the definition of environment variables that will be used to build paths by the rest of the manifest. Notice that the manifest can access shell environment variables like `MODDABLE`.

	"build": {
		"BUILD": "$(MODDABLE)/build",
		"MODULES": "$(MODDABLE)/modules",
		"COMMODETTO": "$(MODULES)/commodetto",
		"PIU": "$(MODULES)/piu",
	},

#### `creation`

The `creation` object defines the creation parameters of the XS machine that will run the app. See the [XS in C documentation](../xs/XS%20in%20C.md) for details.
		
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
		
#### `modules`

The `modules` object describes the necessary modules on all platforms.

	"modules": {
		"*": [
			"$(MODULES)/files/resource/*",
			"$(MODULES)/base/instrumentation/*",
		],
	},

#### `preload`

The `preload` array lists the modules preloaded in the read-only XS virtual machine that will be cloned to run the app.
		
	"preload": [
		"Resource",
		"instrumentation",
	],

#### `strip`

The `strip` object specifies which built-in objects and functions of the JavaScript language may/may not be removed by the XS linker.

	"strip": "*",
		
`"*"` means anything unused by the application can be stripped.

If you only want to allow certain objects or functions to be stripped, pass in an array. Items in the array can be stripped. Anything not included in the array will not be stripped. For example, the following allows `RegExp` to be stripped:

		"strip": [
			"fx_RegExp_prototype_compile",
			"fx_RegExp_prototype_exec",
			"fx_RegExp_prototype_match",
			"fx_RegExp_prototype_replace",
			"fx_RegExp_prototype_search",
			"fx_RegExp_prototype_split",
			"fx_RegExp_prototype_test",
			"fx_RegExp_prototype_toString",
			"fx_RegExp_prototype_get_flags",
			"fx_RegExp_prototype_get_global",
			"fx_RegExp_prototype_get_ignoreCase",
			"fx_RegExp_prototype_get_multiline",
			"fx_RegExp_prototype_get_source",
			"fx_RegExp_prototype_get_sticky",
			"fx_RegExp_prototype_get_unicode",
			"fx_RegExp",
		]

#### `platform `

The `platform` object has one property by platform or sub-platform. Objects in a matching platform/sub-platform are merged with platform objects. Each platform can have an `include` array, a `modules` object, a `preload` array, a `resources` object, a `defines` object and a `recipes` object that will be used only when such platform is the goal of the build. The `esp`, `esp32`, and `gecko` platforms below include an external manifest, whereas the `mac` platform is directly inline.
		
	"platforms": {
		"esp": {
			"include": "$(BUILD)/devices/esp/manifest.json"
		},
		"esp32": {
			"include": "$(BUILD)/devices/esp32/manifest.json"
		},
		"gecko": {
			"include": "$(BUILD)/devices/gecko/manifest.json"
		},
		"mac": {
			"modules": {
				"*": [
					"$(BUILD)/simulator/screen",
					"$(MODULES)/base/time/*",
					"$(MODULES)/base/time/mac/*",
					"$(MODULES)/base/timer/*",
					"$(MODULES)/base/timer/mac/*",
				],
			},
		},

The "`...`" platform identifier is a fallback, if no matching platform is found. This is useful for errors and warnings.

	"platforms":{
		"esp": {
			/* modules and resources for ESP8266 go here */
		},
		"esp32": {
			"warning": "module XYZ not fully tested on esp32",
			/* modules and resources for ESP32 go here */
		},
		"..." {
			"error": "module XYZ supported",
		}
	}

### Sub-platforms

The Gecko family of devices are similar and share much of the same configuration. A sub-platform is used to configure an application for the variations in a product family.

In the segment below, the `timer` module is specified for all `gecko` platforms. The `wakeup` pin is defined differently for the `gecko/giant` and `gecko/mighty` platforms.

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
          
The `SUBPLATFORM` variable is automatically defined by `mcconfig`. A wildcard is a available to match on sub-platforms. The `SUBPLATFORM` variable and wildcard match used together simplify inclusion of sub-platform manifests:

	"platforms": {
		"esp32/*": {
			"include": "./targets/$(SUBPLATFORM)/manifest.json"
		},
	}

## Reference

**mcconfig** processes a manifest in three passes.

### Combine

The first pass combines the properties of the included manifests (in depth first order) with the properties of the processed manifest. For each manifest, the common properties are combined with the properties of the platform that is the goal of the build. Sub-platform properties are combined with platform properties.

When combining properties with the same name, the combined value is the concatenation of the two values.

In the `modules` and `resources` objects, paths can be relative to the manifest that defines them.

### Match

The second pass matches files with the properties of the combined `modules` and `resources` objects:

- The `~` property contains files to exclude from the build.
- The name of each property is the target file, the value of the property is the source file or an array of source files.
- Targets and sources can use the `*` wildcard to represent all files that match.

There are no extensions:

- In the `modules` object, **mcconfig** matches `.c`, `.cc`, `.cpp`, `.h`, `.js` and `.m` files. 
- In the `resources` object, **mcconfig** matches `.act`, `.bmp`, `.cct`, `.dat`, `.der`, `.fnt`, `.jpg`, `.json`, `.nfnt`, `.pk8`, `.png`, `.rle`, `.ski` and `.ttf`  files. 

### Generate

The third pass generates make variables and rules for matched files:

- The .js files are compiled with **xsc** and linked with **xsl**.
- The modules listed in the `preload` properties are preloaded by **xsl** in the read-only XS virtual machine that will be cloned to run the app.
- The .png files are converted by **png2bmp**. Use the `*-color` and `*-alpha` pseudo target to get only color or only alpha bitmaps.
- The .h files define header dependencies and include directories.
- The .c files are compiled and linked with $(CC) unless they match the value of a property of the combined `recipes` object, then they use the make recipe corresponding to the name of the property.

The generated make file contains the generated variables, then the make fragment corresponding to the goal of the build, then the generated rules.

You will find the default make fragments and the make recipes in `$(MODDABLE)/tools/mcconfig`. It is the make fragments that define the compiler options, the linker options, the libraries, etc.

The make fragment can be specified in a platform's `"build"` section.

	"platforms": {
		"gecko/*": {
			"build": {
				"MAKE_FRAGMENT": "$(BUILD)/devices/gecko/targets/$(SUBPLATFORM)/make.$(SUBPLATFORM).mk",
			}
			"include": "./targets/$(SUBPLATFORM)/manifest.json"
		},
	}

