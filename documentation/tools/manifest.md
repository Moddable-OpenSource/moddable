# Manifest

Copyright 2016 Moddable Tech, Inc.

Revised: August 18, 2016

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## Overview

A manifest is a JSON file that describes the modules and resources necessary to build
a Moddable app.

#### `$MODDABLE/examples/piu/balls/manifest.json`

Let us look at the manifest of the "balls" app: . It is quite short:

	{
		"include": [
			"../all.json",
		]
		"modules": {
			"*": "./main",
		},
		"resources":{
			"*": "./balls",
		},
	}

The `include` array lists the manifests to include. Most apps have a lot in common. Instead of repeating properties, Piu examples include `all.json` which describes the modules necessary to all of them.

The `modules` and `resources` objects contain the module and resource that are specific to the "balls" app. 

#### `$MODDABLE/examples/piu/all.json`

Now let us look at the included manifest. It is quite long so let us split it into its properties...

The `build` object contains the definition of environment variables that will be used to build paths by the rest of the manifest. Notice that the manifest can access shell environment variables like `MODDABLE`.

	{
		"build": {
			"BUILD": "$(MODDABLE)/build",
			"MODULES": "$(MODDABLE)/modules",
			"COMMODETTO": "$(MODULES)/commodetto",
			"PIU": "$(MODULES)/piu",
		},

The `creation` object defines the creation parameters of the XS machine that will run the app. See **XS in C** for details.
		
		"creation": {
			"chunk": {
				"initial":1536,
				"incremental":1024,
			},
			"heap": {
				"initial":512,
				"incremental":64,
			},
			"stack":256,
			"keys": {
				"available":32,
				"name":53,
				"symbol":3,
			},
			"main":"main",
		},
		
The `modules` object describes the necessary modules on all platforms.

		"modules": {
			"~": [
			],
			"commodetto/Bitmap": "$(COMMODETTO)/commodettoBitmap",
			"commodetto/Poco": "$(COMMODETTO)/commodettoPoco",
			"commodetto/*": "$(COMMODETTO)/commodettoPocoBlit",
			"commodetto/ParseBMF": "$(COMMODETTO)/commodettoParseBMF",
			"commodetto/ParseBMP": "$(COMMODETTO)/commodettoParseBMP",
			"piu/*": [
				"$(PIU)/All/piu*",
				"$(PIU)/MC/piu*",
			],
			"*": [
				"$(MODULES)/files/resource/*",
				"$(MODULES)/base/instrumentation/*",
			],
		},

The `preload` array lists the modules preloaded in the read-only XS virtual machine that will be cloned to run the app.
		
		"preload": [
			"commodetto/*",
			"piu/*",
			"Resource",
			"instrumentation",
		],
		
The `platform` object has one property by platform. Each platform can have a `modules` object, a `preload` array, a `resources` object, a `defines` object and a `recipes` object that will be used only when such platform is the goal of the build.
		
		"platforms":{
			"esp": {
				"modules": {
					"*": [
						"$(MODULES)/drivers/ssd1306/*",
						"$(MODULES)/drivers/ssd1351/*",
						"$(MODULES)/drivers/ls013b4dn04/*",
						"$(MODULES)/drivers/lpm013m126a/*",
						"$(MODULES)/base/time/*",
						"$(MODULES)/base/timer/*",
						"$(MODULES)/pins/spi/*",
						"$(MODULES)/pins/pins/*",
						"$(MODULES)/network/wifi/*",
						"$(MODULES)/network/net/*",
						"$(MODULES)/network/sntp/*",
						"$(MODULES)/network/socket/*",
						"$(MODULES)/network/socket/lwip/*",
					],
					"setup": "$(BUILD)/devices/esp/setup/setup-piu",
				},
				"preload": [
					"ssd1306",
					"ssd1351",
					"ls013b4dn04",
					"lpm013m126a",
					"time",
					"timer",
					"pins",
					"wifi",
					"net",
					"sntp",
					"socket",
					"setup",
				],
				"defines": {
					"ls013b4dn04": {
						"cs": {
							"pin": 4
						},
						"spi": {
							"port": "#HSPI"
						}
					},
					"lpm013m126a": {
						"cs": {
							"pin": 4
						},
						"spi": {
							"port": "#HSPI"
						}
					}
				},
				"recipes": {
					"strings-in-flash": [
						"commodetto*",
						"piu*",
						"Resource*",
						"mod*",
					],
				}
			},

## Reference

**mcconfig** processes a manifest in three passes.

### Combine

The first pass combines the properties of the included manifests (in depth first order) with the properties of the processed manifest. For each manifest, the common properties are combined with the properties of the platform that is the goal of the build.

When combining properties with the same name, the combined value is the concatenation of the two values.

In the `modules` and `resources` objects, paths can be relative to the manifest that defines them.

### Match

The second pass matches files with the properties of the combined `modules` and `resources` objects:

- The `~` property contains files to exclude from the build.
- The name of each property is the target file, the value of the property is the source file or an array of source files.
- Targets and sources can use the `*` wildcard to represent all files that match.

There are no extensions:

- In the `modules` object, **mcconfig** matches `.c`, `.cpp`, `.h`, `.js` and `.m` files. 
- In the `resources` object, **mcconfig** matches `.act`, `.bmp`, `.cct`, `.dat`, `.der`, `.fnt`, `.jpg`, `.json`, `.nfnt`, `.pk8`, `.png`, `.rle`, `.ski` and `.ttf`  files. 

### Generate

The third pass generates make variables and rules for matched files:

- The .js files are compiled with **xsc** and linked with **xsl**.
- The modules listed in the `preload` properties are preloaded by **xsl** in the read-only XS virtual machine that will be cloned to run the app.
- The .png files are converted by **png2bmp**. Use the `*-color` and `*-alpha` pseudo target to get only color or only alpha bitmaps.
- The .h files define header dependencies and include directories.
- The .c files are compiled and linked with $(CC) unless they match the value of a property of the combined `recipes` object, then they use the make recipe corresponding to the name of the property.

The generated make file contains the generated variables, then the make fragment corresponding to the goal of the build, then the generated rules.

You will find the make fragments and the make recipes in `$(MODDABLE)/tools/mcconfig`. It is the make fragments that define the compiler options, the linker options, the libraries, etc.




