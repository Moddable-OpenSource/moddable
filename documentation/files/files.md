# Files
Copyright 2017 Moddable Tech, Inc.

Revised: November 7, 2017

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.


## class File

The `File` class provides access to files in the [SPIFFS](https://github.com/pellepl/spiffs) file system.

	import {File} from "file";

The SPIFFS file system requires some additional memory. Including SPIFFS in the build increase RAM use by about 500 bytes. Using the SPIFFS file system requires about another 3 KB of RAM. To minimize the memory impact, the File class only instantiates the SPIFFS file system when necessary - when a file is open and when a file is deleted. The SPIFFS file system is automatically closed when no longer in use.

If the SPIFFS file system has not been initialized, it is formatted with SPIFFS_format when first used.

### Get file size

This example opens a file in read-only mode to retrieve the file's length. If the file does not exist, it is not created and an exception is thrown.

	let file = new File("test.txt");
	trace(`File length ${file.length}\n`);
	file.close();

### Read file as String

This example retrieves the entire content of a file into a `String`. If there is insufficient memory available to store the string or the file does not exist, an exception is thrown.

	let file = new File("test.txt");
	trace(file.read(String));
	file.close();

### Read file into ArrayBuffers

This example reads a file in 1024 byte ArrayBuffers. The final ArrayBuffer is smaller than 1024 when the file size is not an integer multiple of 1024.

	let file = new File("test.txt");
	while (file.position < file.length) {
		let buffer = file.read(ArrayBuffer, 1024);
	}
	file.close();

### Write string to file

This example deletes a file, opens it for write (which creates a new empty file), and then writes two `String` values to the file. The script then moves the read/write position to the start of the file, and reads the entire file contents into a single `String`, which is traced to the console.

	File.delete("test.txt");
	
	let file = new File("test.txt", true);
	file.write("This is a test.\n");
	file.write("This is the end of the test.\n");
	
	file.position = 0;
	let content = file.read(String);
	trace(content);
	
	file.close();

### new File(path [, write])

The `File` constructor opens a file for read or write. The optional write argument selects the mode. The default value for write is `false`. When opened, the file position is 0.

If the file does not exist, an exception is thrown when opening in read mode. When opening in write mode, a new file is created if it does not already exist.

### read(type [, count])

The `read` function reads from the current position. The data is read into a `String` or `ArrayBuffer` based on the value of the `type` argument. The `count` argument is the number of bytes to read. The default value of `count` is the number of bytes between the current `position` and the file `length`.

	let file = new File("preferences.json");
	let preferences = JSON.parse(file.read(String));
	file.close();

### write(value [, ...values])

The `write` function writes one or more values to the file starting at the current `position`. The values may be either a `String` or `ArrayBuffer`.

	File.delete("preferences.json");
	let file = new File("preferences.json", true);
	file.write(JSON.stringify(preferences));
	file.close();

### length property

The `length` property is a number indicating the number of bytes in the file. It is read-only.

### position property

The `position` property is a number indicating the byte offset into the file, for the next read or write operation.

### delete(path)

The static `delete` function removes the file at the specified path.

	File.delete("test.txt");

<!-- 11/7/2017 BSF
File.exists and File.rename need to be documented.
-->

## class File Iterator

The Iterator class enumerates the files and subdirectories in a directory. 

	import {Iterator} from "file";

> **Note**: The SPIFFS file system used on ESP8266 is a flat file system, so there are no subdirectories.

### List contents of a directory

This example lists all the files and subdirectories in a directory.

	let root = new Iterator("/");
	let item;
	while (item = root.next()) {
		if (undefined == item.length)
			trace(`Directory: ${item.name}\n`);
		else
			trace(`File: ${item.name}, ${item.length} bytes\n`);
	}

The iterator's `next` function returns an object.  If the object has a `length` property, it is a file; if the `length` property is undefined, it is a directory.

### new Iterator(path)

The constructor takes as its sole argument the path of the directory to iterate over.

> **Note**: For the SPIFFS flat file system, always pass "/" for the `path` argument to the constructor. This ensures compatibility with file systems that implement directories.

### next()

The `next` function is called repeatedly, each time retrieving information about one file. When all files have been returned, the `next` function returns `undefined`. For each file and subdirectory, next returns an object. The object always contains a `name` property with the file name. If the object contains a `length` property, it references a file and the `length` property is the size of the file in bytes. If the `length` property is absent, it references a directory.

<!-- 11/7/2017 BSF
The System class needs to be documented.
-->

## class ZIP

The ZIP class implements read-only file system access to the contents of a ZIP file stored in memory. Typically these are stored in flash memory.

The ZIP implementation requires all files in the ZIP file to be uncompressed. The default in ZIP files is to compress files, so special steps are necessary to build a compatible ZIP file.

The [`zip`](https://linux.die.net/man/1/zip) command line tool creates uncompressed ZIP files when a compression level of zero is specified. The following command line creates a ZIP file named `test.zip` with the uncompressed contents of the directory `test`.

	zip -0r test.zip test

### Instantiate ZIP archive

A ZIP archive is stored in memory. If it is ROM, it will be accessed using a Host Buffer, a variant of an `ArrayBuffer`. The host platform software provides the Host Buffer instance through a platform specific mechanism. This example assumes the ESP8266 object provides that.

	import {ZIP} from "zip";

	let archive = new ZIP(ESP8266.getZIP());

<!-- 11/7/2017 BSF
Perhaps a snippet from our sample app provides an easier example:
	let buffer = new Resource("test.zip");
	let archive = new ZIP(buffer);
-->

### Reading a file from ZIP archive

The ZIP instance's file function provides an instance used to access a file. Though instantiated differently, the ZIP file instance shares the same API with the `File` class.

	let file = archive.file("small.txt");
	trace(`File size: ${file.length} bytes\n`);
	let string = file.read(String);
	trace(string);
	file.close();

### List contents of a ZIP archive's directory

The following example iterates the files and directories at the root of the archive. Often the root contains only a single directory.

	let root = archive.iterate("/");
	let item;
	while (item = root.next()) {
	    if (undefined == item.length)
	        trace(`Directory: ${item.name}\n`);
	    else
	        trace(`File: ${item.name}, ${item.length} bytes\n`);
	}

The ZIP iterator expects directory paths to end with a slash ("`/`"). To iterate the contents of a directory named "test" at the root, use the following code:

	let iterator = archive.iterate("test/");

### new ZIP(buffer)

The ZIP constructor instantiates an ZIP object to access the contents of the buffer as a read-only file system. The buffer may be either an `ArrayBuffer` or a Host Buffer.

The constructor validates that the buffer contains a ZIP archive, throwing an exception if it does not.

### file(path)

The `file` function instantiates an object to access the content of the specified path within the ZIP archive. The returned instance implements the same API as the `File` class.

### iterate(path)

The `iterate` function instantiates an object to access the content of the specified directory path within the ZIP archive. The returned instance implements the same API as the Iterator class. Directory paths end with a slash ("`/`") character and, with the exception of the root path, do not begin with a slash.

### map(path)

The `map` function returns a Host Buffer that references the bytes of the file at the specified path.

<!-- 11/7/2017 BSF
The Flash, Preference and Resource classes live under files in the modules directory. Those classes should probably be documented here.
-->
