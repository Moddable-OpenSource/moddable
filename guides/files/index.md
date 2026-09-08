---
name: Files
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-09-07
---

The ECMA-419 embedded standard [Files](https://419.ecma-international.org/#storage-files) API works with both files and directories. The API is modeled on POSIX file semantics, so it is familiar to most developers.

File and directory operations are performed on a directory instance. The global `device.files` is the root directory instance.

## Files

- [Create, Open, and Close File](./file-create-open-close.md)
- [Read File](./file-read.md)
- [Write File](./file-write.md)
- [Delete File](./file-delete.md)
- [Get File Information](./file-info.md)

## Directories

- [Create Directory](./dir-create.md)
- [Enumerate Directory](./dir-enumerate.md)
- [Delete Directory](./dir-delete.md)
- [Open Directory](./dir-open.md)

## Paths

Paths are [subpath strings](https://419.ecma-international.org/#storage-file-subpath) that follow a few rules:

- All paths are relative to the directory instance they are used with. Absolute paths, `"./"`, and `"../"` are not allowed.
- The path separator is always `"/"`, regardless of the host file system separator.
- Case sensitivity of names depends on the underlying host file system. For portability, be consistent in your use of case to avoid surprises.

## Building

Include the Files manifest in your project's `manifest.json`. Including the manifest initializes the host provider `device.files`:

	$(MODDABLE)/modules/io/files/manifest.json

The manifest selects the implementation for your target: POSIX on macOS and Linux, LittleFS on ESP32, ESP8266, and Raspberry Pi Pico; and platform file systems on Pebble OS and Zephyr.

### ESP-IDF VFS
On ESP32, the ESP-IDF a [virtual file system](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/vfs.html) (VFS) implementation is available as an alternative to the Moddable SDK's built-in LittleFS implementation. VFS is how projects can use FAT file systems, including SD cards. Include one of the following manifests instead of the manifest above. Only one Files implementation may be included in a build.

	$(MODDABLE)/modules/io/files/esp32/manifest.json				SPIFFS on the "storage" flash partition
	$(MODDABLE)/modules/io/files/esp32/manifest_fat.json			FAT on the "storage" flash partition, with wear levelling
	$(MODDABLE)/modules/io/files/esp32/manifest_sdspi.json		FAT on an SD card connected by SPI
	$(MODDABLE)/modules/io/files/esp32/manifest_sdmmc.json		FAT on an SD card connected to the SDMMC host
	$(MODDABLE)/modules/io/files/esp32/manifest_littlefs.json	littlefs on the "storage" flash partition, using the ESP-IDF managed component

The flash partition is created automatically when the manifest is included. The SD card manifests take the card connection from the target's `sdcard` defines, alongside its `spi` and `i2c` defines. SD cards over SPI need `sdcard.cs` and use the target's `spi.miso_pin`, `spi.mosi_pin`, and `spi.sck_pin` unless `sdcard.miso`, `mosi`, and `sck` are provided. SDMMC needs `sdcard.clk`, `cmd`, and `d0` on targets with a GPIO matrix, and `d1`, `d2`, and `d3` when `sdcard.width` is `4`; `sdcard.internalPullup` enables the weak internal pull-ups. Both accept optional `sdcard.cd` and `sdcard.hz`. A card is never formatted unless `file.format` is set to `1`.

The mount point is `file.root` (default `/mod`), and `config.files` may name a directory beneath it to use as the root directory instance. Note that SPIFFS is a flat file system: `createDirectory` throws, and file names may contain `/` without any directory existing.

## Learn More

- Standard
	- [Files](https://419.ecma-international.org/#storage-files) in ECMA-419
- Implementation
	- [Files](../../modules/io/files/) in Moddable SDK
- TypeScript Declaration
	- [Files](../../typings/embedded/storage/files.d.ts) in Moddable SDK
