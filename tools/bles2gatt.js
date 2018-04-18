/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import { FILE, TOOL } from "tool";

const ESP_GATT_CHAR_PROP_BIT_READ = (1 << 1);
const ESP_GATT_CHAR_PROP_BIT_WRITE = (1 << 3);
const ESP_GATT_CHAR_PROP_BIT_NOTIFY = (1 << 4);

export default class extends TOOL {
	constructor(argv) {
		super(argv);
		this.name = null;
		this.outputPath = null;
		this.files = [];
		var argc = argv.length;
		var name, path;
		for (var argi = 1; argi < argc; argi++) {
			var option = argv[argi];
			switch (option) {
				case "-o":
					argi++;	
					if (argi >= argc)
						throw new Error("-o: no directory!");
					name = argv[argi];
					if (this.outputDirectory)
						throw new Error("-o '" + name + "': too many directories!");
					path = this.resolveDirectoryPath(name);
					if (!path)
						throw new Error("-o '" + name + "': directory not found!");
					this.outputPath = path;
					break;
				default:
					name = argv[argi];
					path = this.resolveFilePath(name);
					if (!path)
						throw new Error("'" + name + "': file not found!");
					this.files.push(path);
					break;
			}
		}
		if (!this.name)
			this.name = "mc.bleservices.c";
		if (!this.outputPath)
			this.outputPath = this.currentDirectory;
		if (!this.platform)
			this.platform = this.currentPlatform;
	}
	run() {
		var path = this.joinPath({directory: this.outputPath, name:this.name});
		var file = new FILE(path);
		if (0 == this.files.length) {
			file.line("#define attribute_count 0");
			file.line("");
			file.line("static const esp_gatts_attr_db_t gatt_db[0] = {};");
			file.close();
			return;
		}
		var services = [];
		var attributeIndex = 0;
		var attributeCount = 0;
		var characteristicIndex = 0;
		file.line("#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))");
		file.line("");
		file.line("static const uint16_t primary_service_uuid = 0x2800;");
		file.line("static const uint16_t character_declaration_uuid = 0x2803;");
		file.line("static const uint16_t character_client_config_uuid = 0x2902;");
		file.line("");
		file.line("static const uint8_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;");
		file.line("static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;");
		file.line("static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;");
		file.line("static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;");
		file.line("static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;");
		file.line("");
		this.files.forEach((path, index) => {
			services = services.concat(JSON.parse(this.readFileString(path)));
		});
		services.forEach((service, index) => {
			++attributeCount;
			if ("characteristics" in service) {
				attributeCount += (service.characteristics.length * 2);
			}
		});
		services.forEach((service, index) => {
			file.line(`static const uint16_t service_uuid${index} = 0x${service.uuid};`);
			if ("characteristics" in service) {
				service.characteristics.forEach((characteristic, index) => {
					let uuid = characteristic.uuid;
					if (4 == uuid.length)
						file.line(`static const uint16_t char_uuid${characteristicIndex} = 0x${characteristic.uuid};`);
					else if (36 == uuid.length) {
						uuid = uuid.replace(/-/g,'');
						let uuid128 = new Uint8Array(16);
						for (let i = 0; i < 16; ++i)
							uuid128[15 - i] = parseInt(uuid.slice(2 * i, 2 * i + 2), 16);
						file.write(`static const uint8_t char_uuid${characteristicIndex}[16] = { `);
						file.write(buffer2hexlist(uuid128.buffer));
						file.write(" };");
						file.line("");
					}
					else
						throw new Error("unsupported UUID length");
					if ("value" in characteristic) {
						let buffer, value = characteristic.value;
						if (value instanceof Array)
							buffer = new Uint8Array(value).buffer;
						else if (typeof value == "string")
							buffer = ArrayBuffer.fromString(value);
						else
							throw new Error("unknown format");
						characteristic._length = buffer.byteLength;
						file.write(`static const uint8_t char_value${characteristicIndex}[${buffer.byteLength}] = { `);
						file.write(buffer2hexlist(buffer));
						file.write(" };");
						file.line("");
					}
					++characteristicIndex;
				});
				file.line("");
			}
		});
		characteristicIndex = 0;
		
		file.line(`#define attribute_count ${attributeCount}`);
		file.line("");
		file.line(`static const esp_gatts_attr_db_t gatt_db[attribute_count] = {`);
		services.forEach((service, index) => {
			// primary service attribute
			file.line(`\t// service ${service.uuid}`);
			file.line("\t[", attributeIndex, "] = {");
			file.line("\t\t{ESP_GATT_AUTO_RSP},");
			file.line(`\t\t{ESP_UUID_LEN_16, (uint8_t*)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(service_uuid${index}), (uint8_t*)&service_uuid${index}}`);
			file.line("\t},");
			++attributeIndex;
			
			if ("characteristics" in service) {
				service.characteristics.forEach((characteristic, index) => {
					// characteristic declaration
					let properties = parseProperties(characteristic.properties.split(","));
					let permissions = parsePermissions(characteristic.permissions.split(","));
					file.line("\t[", attributeIndex, "] = {");
					file.line("\t\t{ESP_GATT_AUTO_RSP},");
					file.line(`\t\t{ESP_UUID_LEN_16, (uint8_t*)&character_declaration_uuid, ${permissions}, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*)&${properties}}`);
					file.line("\t},");
					++attributeIndex;

					// characteristic value
					let esp_uuid_len = (4 == characteristic.uuid.length ? "ESP_UUID_LEN_16" : "ESP_UUID_LEN_128");
					file.line("\t[", attributeIndex, "] = {");
					file.line("\t\t{ESP_GATT_RSP_BY_APP},");
					file.write(`\t\t{${esp_uuid_len}, (uint8_t*)&char_uuid${characteristicIndex}, ${permissions}, ${characteristic.maxBytes}, `);
					if ("value" in characteristic)
						file.write(`${characteristic._length}, (uint8_t*)&char_value${characteristicIndex}}`);
					else
						file.write("0, NULL}");
					file.line("");
					file.line("\t},");
					++attributeIndex;
					++characteristicIndex;
				});
			}
		});
		file.line("};");
		file.line("");
		file.close();
	}
}

function parseProperties(properties) {
	let props = 0;
	properties.forEach(p => {
		switch(p.trim()) {
			case "read":
				props |= ESP_GATT_CHAR_PROP_BIT_READ;
				break;
			case "write":
				props |= ESP_GATT_CHAR_PROP_BIT_WRITE;
				break;
			case "notify":
				props |= ESP_GATT_CHAR_PROP_BIT_NOTIFY;
				break;
			default:
				throw new Error("unknown property");
		}
	});
	if (props == ESP_GATT_CHAR_PROP_BIT_READ)
		props = "char_prop_read";
	else if (props == ESP_GATT_CHAR_PROP_BIT_WRITE)
		props = "char_prop_write";
	else if (props == ESP_GATT_CHAR_PROP_BIT_NOTIFY)
		props = "char_prop_notify";
	else if (props == (ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE))
		props = "char_prop_read_write";
	else if (props == (ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY))
		props = "char_prop_read_notify";
	else
		throw new Error("unsupported property combination");
	return props;
}

function parsePermissions(permissions) {
	let perms = [];
	permissions.forEach(p => {
		switch(p.trim()) {
			case "read":
				perms.push("ESP_GATT_PERM_READ");
				break;
			case "write":
				perms.push("ESP_GATT_PERM_WRITE");
				break;
			default:
				throw new Error("unsupported permission");
		}
	});
	return perms.join("|");
}

function buffer2hexlist(buffer) {
	let byteArray = new Uint8Array(buffer);
	let hexParts = [];
	for (let i = 0; i < byteArray.length; i++) {
		let hex = byteArray[i].toString(16);
		let padded = ("00" + hex).slice(-2);
		hexParts.push("0x" + padded);
	}
	return hexParts.join(", ");
}
