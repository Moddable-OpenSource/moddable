# Files
Copyright 2017-2022 Moddable Tech, Inc.<BR>
Revised: November 28, 2022

## Table of Contents

* [File Systems](#filesystems)
	* [File](#file)
	* [Directory](#directory)
	* [File Iterator](#file-iterator)
	* [File System](#file-system)
	* [Host File System Configuration](#platforms)
		* [SPIFFS](#spiffs)
		* [FAT32](#fat32)
		* [littlefs](#littlefs)
* [Zip](#zip)
* [Resource](#resource)
* [Preference](#preference)
* [Flash](#flash)

<a id="filesystems"></a>
## File Systems

The File module contains several classes used to access files and, when supported, directories.

### File Paths

The root path of the default file system varies depending on the host. To make it straightforward to write scripts that work on a variety of different devices, the Moddable SDK includes the root path of the default file system in the `config/mc` module.

```js
import config from "mc/config";

File.delete(config.file.root + "test.txt");
```

As a rule, scripts should always prefix full paths with this root.

The forward slash character (`/`) is always used as a path separator, even on hosts that natively use a different path separator.

The `System.config()` function, described below, provides the length of the longest supported path through the `maxPathLength` property. 

<a id="file"></a>
### class File

- **Source code:** [file](../../modules/files/file)
- **Relevant Examples:** [files](../../examples/files/files/)

The `File` class provides access to files.

```js
import {File} from "file";
```

#### `constructor(path [, write])`

The `File` constructor opens a file for read or write. The optional write argument selects the mode. The default value for write is `false`. When opened, the file position is 0.

If the file does not exist, an exception is thrown when opening in read mode. When opening in write mode, a new file is created if it does not already exist.

```js
let file = new File(config.file.root + "preferences.json");
```

***

#### `read(type [, count])`

The `read` function reads from the current position. The data is read into a `String` or `ArrayBuffer` based on the value of the `type` argument. The `count` argument is the number of bytes to read. The default value of `count` is the number of bytes between the current `position` and the file `length`.

```js
let file = new File(config.file.root + "preferences.json");
let preferences = JSON.parse(file.read(String));
file.close();
```
***

#### `write(value [, ...values])`

The `write` function writes one or more values to the file starting at the current `position`. The values may be either a `String` or `ArrayBuffer`.

```js
File.delete(config.file.root + "preferences.json");
let file = new File(config.file.root  + "preferences.json", true);
file.write(JSON.stringify(preferences));
file.close();
```

***

#### `length` property

The `length` property is a number indicating the number of bytes in the file. It is read-only.

***

#### `position` property

The `position` property is a number indicating the byte offset into the file, for the next read or write operation.

***

#### `static delete(path)`

The static `delete` function removes the file at the specified path.

```js
File.delete(config.file.root + "test.txt");
```

***

#### `static exists(path)`

The static `exists` function returns a boolean indicating whether a file exists at the specified path.

```js
let exists = File.exists(config.file.root + "test.txt");
```

***

#### `static rename(from, to)`

The static `rename` function renames the file specified by the `from` argument to the name specified by the `to` argument.

```js
File.rename(config.file.root + "test.txt", "betterName.txt");
```

The `to` argument may be either a file name, as in the example above, or a full file path, as in the example below. The full file path form is useful when the host file system supports using `rename` to move a file between directories.

```js
File.rename(config.file.root + "test.txt", config.file.root + "better/name.txt");
```

***

#### Example: Get File Size

This example opens a file in read-only mode to retrieve the file's length. If the file does not exist, it is not created and an exception is thrown.

```js
let file = new File(config.file.root + "test.txt");
trace(`File length ${file.length}\n`);
file.close();
```

***

#### Example: Read File as String

This example retrieves the entire content of a file into a `String`. If there is insufficient memory available to store the string or the file does not exist, an exception is thrown.

```js
let file = new File(config.file.root + "test.txt");
trace(file.read(String));
file.close();
```

***

#### Example: Read File into ArrayBuffers

This example reads a file into one or more `ArrayBuffer` objects. The final `ArrayBuffer` is smaller than 1024 when the file size is not an integer multiple of 1024.

```js
let file = new File(config.file.root + "test.txt");
while (file.position < file.length) {
	let buffer = file.read(ArrayBuffer, 1024);
}
file.close();
```

***

#### Example: Write String to File

This example deletes a file, opens it for write (which creates a new empty file), and then writes two `String` values to the file. The script then moves the read/write position to the start of the file, and reads the entire file contents into a single `String`, which is traced to the console.

```js
File.delete(config.file.root + "test.txt");

let file = new File(config.file.root + "test.txt", true);
file.write("This is a test.\n");
file.write("This is the end of the test.\n");

file.position = 0;
let content = file.read(String);
trace(content);

file.close();
```

***

<a id="directory"></a>
### class Directory

- **Source code:** [file](../../modules/files/file)

The `Directory` class creates and deletes directories. To list the files and directories in a directory, use the `Iterator` class.

```js
import {Directory} from "file";
```

> **Note**: Because the SPIFFS file system is a flat file system, directories cannot be created or deleted when on it.

#### `static create(path)`

The `create` function creates a directory at the specified path. All parent directories in `path` must already exist: `create` does not automatically create parent directories.

```js
Directory.create(config.file.root + "tmp");
```

***

#### `static delete(path)`

The `delete` function deletes the directory at the specified path. On most file systems, the directory must be empty to be deleted and `delete` throws an exception when it is not.

```js
Directory.delete(config.file.root + "tmp");
```

***

<a id="file-iterator"></a>
### class File Iterator

- **Source code:** [file](../../modules/files/file)
- **Relevant Examples:** [files](../../examples/files/files/)

The File `Iterator` class enumerates the files and subdirectories in a directory. 

```js
import {Iterator} from "file";
```

> **Note**: Because the SPIFFS file system is a flat file system,  no subdirectories are returned on devices that use it.

#### `constructor(path)`

The constructor takes as its sole argument the path of the directory to iterate over.

```js
let iterator = new Iterator(config.file.root);
```

The iterator instance is a JavaScript [iterable object](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/for...of) so it may be used in `for...of` loops. An example is provided below.

***

#### `next()`

The `next` function is called repeatedly, each time retrieving information about one file. When all files have been returned, the `next` function returns `undefined`. For each file and subdirectory, next returns an object. The object always contains a `name` property with the file name. If the object contains a `length` property, it references a file and the `length` property is the size of the file in bytes. If the `length` property is absent, it references a directory.

```js
let item = iterator.next();
```

***

#### Example: List Contents of a Directory

This example lists all the files and subdirectories in a directory.

```js
let iterator = new Iterator(config.file.root);
let item;
while (item = iterator.next()) {
	if (undefined === item.length)
		trace(`Directory: ${item.name}\n`);
	else
		trace(`File: ${item.name}, ${item.length} bytes\n`);
}
```

The iterator's `next` function returns an object.  If the object has a `length` property, it is a file; if there is no `length` property, it is a directory.

This is a variation of the same example using a `for...of` loop.

```js
for (const item of (new Iterator(config.file.root))) {
	if (undefined === item.length)
		trace(`Directory: ${item.name}\n`);
	else
		trace(`File: ${item.name}, ${item.length} bytes\n`);
}
```

***

<a id="file-system"></a>
### class File System

- **Source code:** [file](../../modules/files/file)

The File `System` class provides information about the file system.

```js
import {System} from "file";
```

#### `static config()`

The `config` function returns a dictionary with information about the file system. At this time, the dictionary has a single property, `maxPathLength`, which indicates the length of the longest file path in bytes.

```js
let maxPathLength = System.config().maxPathLength;
```

***

#### `static info()`

The `info` function returns a dictionary with information about the free and used space in the file system, if available. The `used` property of the dictionary gives the number of bytes in use and the `total` property indicates the maximum capacity of the file system in bytes.

```js
let info = System.info();
let percentFree = 1 - (info.used / info.total);
```

The properties available on the object returned by `info` vary based on the capabilities of the host platform. Consequently, the `total` and `used` properties may not be available and other properties may be present.

***

<a id="platforms"></a>
### Host File System Configuration

This section describes how the file system is implemented on some embedded hosts. This information is helpful for situations where the default file system configuration does not meet the needs of a particular project.

<a id="spiffs"></a>
#### SPIFFS -- ESP8266 & ESP32

On ESP8266 and (by default) ESP32, the File module is implemented using the [SPIFFS](https://github.com/pellepl/spiffs) file system.

SPIFFS is a flat file system, meaning that there are no directories and all files are at the root.

The SPIFFS file system requires some additional memory. Including SPIFFS in the build increase RAM use by about 500 bytes on ESP8266. Using the SPIFFS file system requires about another 3 KB of RAM. To minimize the memory impact, the `File` class only instantiates the SPIFFS file system when necessary -- when a file is open and when a file is deleted. The SPIFFS file system is automatically closed when not in use.

If the SPIFFS file system has not been initialized, it is formatted when first used. Initialization takes up to one minute.

On ESP32, the SPIFFS partition size is specified in a partitions file with a partition of type `data` and subtype `spiffs`. The [default partitions.csv](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj-esp32/partitions.csv) allocates a 64 KB partition for this purpose. A custom partition file can be specified by setting the `PARTITIONS_FILE` variable in the `build` section of the project manifest.

```JSON
"build": {
	"PARTITIONS_FILE": "./customPartitions.csv"
}
```

On ESP32, the SPIFFS file system is mounted at a specified path and all files/directories created should be accessed within that root path. The default root path is `/mod`, but this can be changed with the `root` define in the manifest:

```JSON
"defines": {
	"file":{
		"root": "#/myroot"
	}
}
```

<a id="fat32"></a>
#### FAT32 -- ESP32

The `File` class implements an optional FAT32 file system for the ESP32. Unlike SPIFFS, FAT32 file systems are not flat: they have directory structures and long filenames (up to 255 characters).

If the FAT32 file system has not been initialized then it is formatted when first used. As with SPIFFS, the `File` class only instantiates the FAT32 file system when necessary and it is automatically closed when not in use.

To enable the FAT32 file system, set the `fat32` [manifest define](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/tools/defines.md) to `1`:

```JSON
"defines": {
	"file":{
		"fat32": 1
	}
}
```

The storage partition used by the default Moddable SDK build for ESP32 does not reserve a partition for FAT32. Therefore, it is necessary to use a different partition file in projects that use FAT32. To do that, set the  `PARTITIONS_FILE` variable in the `build` section of the project manifest:

```JSON
"build": {
	"PARTITIONS_FILE": "./customPartitions.csv"
}
```

The FAT32 partition has the type `data` and subtype `fat`.  The FAT32 implementation requires a minimum partition size of about 576 KB. The format of the partition is defined by the ESP-IDF. The following example shows a partitions file with a FAT32 partition of the minimum size:

```CSV
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x006000,
phy_init, data, phy,     0xf000,  0x001000,
factory,  app,  factory, 0x10000, 0x300000,
xs,       0x40, 1,       0x310000, 0x040000,
settings, data, 1,       0x350000, 0x010000,
storage,  data, fat,     0x360000, 0x090000,
```
The default name for the FAT32 partition is `storage`. To use a different name, set the `partition` define in the manifest:

```JSON
"defines": {
	"file":{
		"partition": "#userdata"
	}
}
```

By default, the FAT32 file system is mounted at `/mod`. To change the default root, set the `root` define in the manifest:

```JSON
"defines": {
	"file":{
		"root": "#/myroot"
	}
}
```

<a id="littlefs"></a>
#### littlefs
The [littlefs](https://github.com/littlefs-project/littlefs) file system is "a little fail-safe filesystem designed for microcontrollers." It provides a high reliability, hierarchical file system in a small code footprint (about 60 KB) using minimal memory (well under 1 KB) with a high degree of configurability. littlefs also supports long file names (up to 255 characters) and formats a new partition very quickly.

The Moddable SDK supports littlefs using the APIs described above. To use littlefs, include its manifest. 

```json
"includes": {
	"$MODDABLE/modules/files/file/manifest_littlefs.json"
}
```

> **Note**: A project may use the littlefs manifest or the default file manifest (`$MODDABLE/modules/files/file/manifest.json`). Both cannot currently be included in the same project.

On ESP32, littlefs uses the "storage" partition to hold the file system. On ESP8266, the file system is stored in the upper 3 MB of flash (the same area used by SPIFFS). On other devices, littlefs uses a 64 KB static memory buffer to hold the file system. This RAM disk mode allows littlefs to be used with the simulator.

The littlefs implementation is thread safe on devices running FreeRTOS (ESP32) allowing littlefs to be used with Workers. Thread safety is irrelevant on ESP8266 as it runs as a single process. The thread safety support may be extended for other runtime environments.

The littlefs implementation can be configured to trade-off performance and memory use. The default configuration in the Moddable SDK uses the least memory possible. For projects that make lightweight use of the file system, this offers adequate performance. To improve performance, the configuration may be changed in the project's manifest. The `read_size`, `prog_size`, `lookahead_size`, and `block_cycles` values are described in [`lfs.h`](https://github.com/littlefs-project/littlefs/blob/40dba4a556e0d81dfbe64301a6aa4e18ceca896c/lfs.h#L194-L230). Experimentation has shown that increasing the four `*_size` settings from 16 bytes to 512 gives a significant performance boost at the expense of 2 KB of RAM.

```json
	"defines": {
		"file": {
			"lfs": {
				"read_size": 16,
				"prog_size": 16,
				"cache_size": 16,
				"lookahead_size": 16,
				"block_cycles": 500
			}
		}
	},
```

When not in use (when all files and file iterators are closed), the littlefs implementation unmounts the file system. This releases all memory. This is the same behavior implemented by SPIFFS and FAT32.

<a id="zip"></a>
## class ZIP

- **Source code:** [zip](../../modules/files/zip)
- **Relevant Examples:** [zip](../../examples/files/zip/)

The `ZIP` class implements read-only file system access to the contents of a ZIP file stored in memory. Typically these are stored in flash memory. A ZIP file is a convenient way to embed a read-only file system into a project.

The `ZIP` implementation provides access to the files contained in the ZIP file.   By default, the files in most ZIP files are compressed. The `ZIP` class does not decompress the data when reading. ZIP does not require files to be compressed, so one option is to build a ZIP file with uncompressed content. On devices with enough memory, ZIP files with compressed content may be used by decompressing the data after reading using the zlib `inflate` module.

One way to create a ZIP file with uncompressed content is the [`zip`](https://linux.die.net/man/1/zip) command line tool. It creates uncompressed ZIP files when a compression level of zero is specified. The following command line creates a ZIP file named `test.zip` with the uncompressed contents of the directory `test`.

	zip -0r test.zip test

To compress the content, use a different compression level. The highest compression level is `9`:

	zip -9r test.zip test

**Note**: The `zip` command line tool creates a directory named "test" at the root of the ZIP file. For example, the file at "test/example.txt" is accessed in the ZIP file as "test/example.txt" not "example.txt".

### `constructor(buffer)`

The `ZIP` constructor instantiates a `ZIP` object to access the contents of the buffer as a read-only file system. The buffer may be either an `ArrayBuffer` or a Host Buffer.

The constructor validates that the buffer contains a ZIP archive, throwing an exception if it does not.

A ZIP archive is stored in memory. If it is ROM, it will be accessed using a Host Buffer, a variant of an `ArrayBuffer`. The host platform software provides the Host Buffer instance through a platform specific mechanism. This example uses the `Resource` constructor to create the Host Buffer.

```js
let buffer = new Resource("test.zip");
let archive = new ZIP(buffer);
```

***

### `file(path)`

The `file` function instantiates an object to access the content of the specified path within the ZIP archive. The returned instance implements the same API as the `File` class.

```js
let file = archive.file("small.txt");
```

***

### `iterate(path)`

The `iterate` function instantiates an object to access the content of the specified directory path within the ZIP archive. The returned instance implements the same API as the Iterator class. Directory paths end with a slash ("`/`") character and, with the exception of the root path, do not begin with a slash.

```js
let root = archive.iterate("/");
```

***

### `map(path)`

The `map` function returns a Host Buffer that references the bytes of the file at the specified path.

***

### `method`

The read-only `method` property returns an integer indicating how the file is compressed in the ZIP file. The values are taken from the ZIP specification. For example, the value 8 indicates `deflate` compression was used.

***

### `crc`

The read-only `crc` property returns an integer containing the CRC value stored for the file in the ZIP file. This property is useful for caching.

***

### Example: Read File from ZIP Archive

The `ZIP` instance's `file` function provides an instance used to access a file. Though instantiated differently, the ZIP file instance shares the same API with the `File` class.

```js
let file = archive.file("small.txt");
trace(`File size: ${file.length} bytes\n`);
let string = file.read(String);
trace(string);
file.close();
```

***

### Example: List Contents of a ZIP Archive's Directory

The following example iterates the files and directories at the root of the archive. Often the root contains only a single directory.

```js
let root = archive.iterate("/");
let item;
while (item = root.next()) {
    if (undefined == item.length)
        trace(`Directory: ${item.name}\n`);
    else
        trace(`File: ${item.name}, ${item.length} bytes\n`);
}
```

The ZIP iterator expects directory paths to end with a slash ("`/`"). To iterate the contents of a directory named "test" at the root, use the following code:

```js
let iterator = archive.iterate("test/");
```

***

<a id="resource"></a>
## class Resource

- **Source code:** [resource](../../modules/files/resource)
- **Relevant Examples:** [zip](../../examples/files/zip/), many [Commodetto examples](../../examples/commodetto) including [sprite](../../examples/commodetto/sprite) and [text](../../examples/commodetto/text)

The `Resource` class provides access to assets from an application's resource map.

```js
import Resource from "Resource";
```

### `constructor(path)`

The `Resource` constructor takes a single argument, the resource path, and returns a Host Buffer containing the resource data.

```js
let resource = new Resource("logo.bmp");
trace(`resource size is ${resource.byteLength}\n`);
```

***

### `static exists(path)`

The static `exists` function returns a boolean indicating whether a resource exists at the specified path.

```js
let path = "test.zip";
if (Resource.exists(path))
	trace(`File ${path} exists\n`);
```

***

### `slice(begin[[, end], copy])`

The `slice` function returns a portion of the resource in an `ArrayBuffer`. The default value of `end` is the resource size.

```js
let resource = new Resource("table.dat");
let buffer1 = resource.slice(5);		// Get a buffer starting from offset 5
let buffer2 = resource.slice(0, 10);	// Get a buffer of the first 10 bytes
```

The optional `copy` argument defaults to `true`. If it is set to `false`, the return value is a read-only `HostBuffer` that references the original resource data. This option is useful for creating a reference to a portion of the resource data without copying it to RAM.

***

<a id="preference"></a>
## class Preference

- **Source code:** [preference](../../modules/files/preference/)
- **Relevant Examples:** [preference](../../examples/files/preference/), [preferences](../../examples/piu/preferences/)

The `Preference` class provides storage of persistent preference storage. Preferences are appropriate for storing small amounts of data that needs to persist between runs of an application.

```js
import Preference from "preference";
```
	
Preferences are grouped by domain. A domain contains one or more keys. Each domain/key pair holds a single value, which is either a `Boolean`, integer (e.g. `Number` with no fractional part), `String` or `ArrayBuffer`.

```js
const domain = "wifi";
let ssid = Preference.get(domain, "ssid");
let password = Preference.get(domain, "psk");
```

Limits on the length of key/domain names and preference values vary by target platform.

 - On ESP8266, key/domain names are limited to 32 characters and values are limited to 63 bytes.
 - On ESP32, the `Preference` class is backed by the ESP-IDF's [NVS Library](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html) which limits key/domain names to 15 characters and values to 4000 bytes.

On embedded devices the storage space for preferences is limited. The amount depends on the device, but it can be as little as 4 KB. Consequently, applications should take care to keep their  preferences as small as practical.

> **Note**: On embedded devices, preferences are stored in SPI flash which has a limited number of erase cycles. Applications should minimize the number of write operations (set and delete). In practice, this isn't a significant concern. However, an application that updates preferences once per minute, for example, could eventually exceed the available erase cycles for the preference storage area in SPI flash.

### `static set(domain, key, value)`

The static `set` function sets a preference value.

```js
Preference.set("wifi", "ssid", "linksys");
Preference.set("wifi", "password", "admin");
Preference.set("wifi", "channel", 6);
```

***

### `static get(domain, key)`

The static `get` function reads a preference value. If the preference does not exist, `get` returns `undefined`.

```js
let value = Preference.get("settings", "timezone");
if (value !== undefined)
	trace(`timezone ${value}\n`);
```

***

### `static delete(domain, key)`

The static `delete` function removes a preference. If the preference does not exist, no error is thrown.

```js
Preference.delete("wifi", "password");
```

***

### `static keys(domain)`

Returns an array of all keys under the given domain.
 
```js
let wifiKeys = Preference.keys("wifi");
for (let key of wifiKeys)
	trace(`${key}: ${Preference.get("wifi", key)}\n`);
```
 
***

<a id="flash"></a>
## class Flash

This class is not yet documented.
