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


class GATTFile {
	constructor(dictionary) {
		this.tool = dictionary.tool;
		this.role = dictionary.role;
		this.file = dictionary.file;
		this.services = dictionary.services;
	}
	generate() {}
};

class ESP32GATTFile extends GATTFile {
	generate() {
		if ("server" == this.role || "client" == this.role) {
			let tool = this.tool;
			let path = tool.moddablePath + tool.slash + "build" + tool.slash + "devices" + tool.slash + "esp32" + tool.slash + "xsProj" + tool.slash + "sdkconfig";
			let sdkconfig = tool.readFileString(path);
			if (-1 == sdkconfig.indexOf("CONFIG_BT_ENABLED=y"))
				throw new Error("Set CONFIG_BT_ENABLED=y in sdkconfig to enable BLE in ESP32 build!");
			if ("client" == this.role) {
				if (-1 == sdkconfig.indexOf("CONFIG_GATTC_ENABLE=y"))
					throw new Error("Set CONFIG_GATTC_ENABLE=y in sdkconfig to enable BLE client in ESP32 build!");
			}
			else {
				if (-1 == sdkconfig.indexOf("CONFIG_GATTS_ENABLE=y"))
					throw new Error("Set CONFIG_GATTS_ENABLE=y in sdkconfig to enable BLE server in ESP32 build!");
			}
		}
		let file = this.file;
		let services = this.services;
		file.line('/* WARNING: This file is automatically generated. Do not edit. */');
		file.line("");
		file.line("typedef struct {");
		file.line("\tuint8_t service_index;");
		file.line("\tuint8_t att_index;");
		file.line("\tconst char *name;");
		file.line("\tconst char *type;");
		file.line("} char_name_table;");
		file.line("");
		if (0 == services.length) {
			file.line("#define service_count 0");
			file.line("#define char_name_count 0");
			file.line("#define max_attribute_count 0");
			file.line("");
			file.line("static const uint8_t attribute_counts[0] = {};");
			file.line("static const esp_gatts_attr_db_t gatt_db[0][0] = {};");
			file.line("static const char_name_table char_names[0] = {};");
			return;
		}
		var attributeIndex = 0;
		var char_names = [];
		file.line("#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))");
		file.line("");
		file.line("static const uint16_t primary_service_uuid = 0x2800;");
		file.line("static const uint16_t character_declaration_uuid = 0x2803;");
		file.line("static const uint16_t character_client_config_uuid = 0x2902;");
		file.line("");
		file.line("static const uint8_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;");
		file.line("static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;");
		file.line("static const uint8_t char_prop_read_indicate = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_INDICATE;");
		file.line("static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;");
		file.line("static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;");
		file.line("static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;");
		file.line("");
		
		var maxAttributeCount = 0;
		var attributeCounts = new Array(services.length);
		var characteristicIndex = 0;
		services.forEach((service, index) => {
			let attributeCount = 1;
			let characteristics = service.characteristics;
			attributeCount += (Object.keys(characteristics).length * 2);
			for (let key in characteristics) {
				let characteristic = characteristics[key];
				let properties = characteristic.properties;
				if (properties.includes("notify") || properties.includes("indicate")) {
					++attributeCount;
					characteristic._notify = true;
				}
			}
			if (attributeCount > maxAttributeCount)
				maxAttributeCount = attributeCount;
			attributeCounts[index] = attributeCount;
			if (4 == service.uuid.length)
				file.line(`static const uint16_t service_uuid${index} = 0x${service.uuid};`);
			else {
				file.write(`static const uint8_t service_uuid${index}[16] = { `);
				file.write(buffer2hexlist(uuid128toBuffer(service.uuid)));
				file.write(" };");
				file.line("");
			}
			for (let key in characteristics) {
				let characteristic = characteristics[key];
				let uuid = characteristic.uuid;
				if (4 == uuid.length)
					file.line(`static const uint16_t char_uuid${characteristicIndex} = 0x${characteristic.uuid};`);
				else if (36 == uuid.length) {
					file.write(`static const uint8_t char_uuid${characteristicIndex}[16] = { `);
					file.write(buffer2hexlist(uuid128toBuffer(uuid)));
					file.write(" };");
					file.line("");
				}
				else
					throw new Error("unsupported UUID length");
				if ("value" in characteristic) {
					let buffer = typedValueToBuffer(characteristic.type, characteristic.value);
					characteristic._length = buffer.byteLength;
					file.write(`static const uint8_t char_value${characteristicIndex}[${buffer.byteLength}] = { `);
					file.write(buffer2hexlist(buffer));
					file.write(" };");
					file.line("");
				}
				if (characteristic._notify) {
					file.line(`static const uint8_t char_ccc${characteristicIndex}[2] = { 0x00, 0x00 };`);
				}
				++characteristicIndex;
			}
			file.line("");
		});
		characteristicIndex = 0;
		
		file.line(`#define service_count ${services.length}`);
		file.line(`#define max_attribute_count ${maxAttributeCount}`);
		file.write(`static const uint8_t attribute_counts[${services.length}] = { `);
		file.write(buffer2hexlist(attributeCounts));
		file.write(" };");
		file.line("");

		file.line(`static const esp_gatts_attr_db_t gatt_db[${services.length}][${maxAttributeCount}] = {`);
		services.forEach((service, index) => {
			attributeIndex = 0;
			file.line("\t{");
			
			// primary service attribute
			file.line(`\t\t// Service ${service.uuid}`);
			file.line("\t\t[", attributeIndex, "] = {");
			file.line("\t\t\t{ESP_GATT_AUTO_RSP},");
			file.line(`\t\t\t{ESP_UUID_LEN_16, (uint8_t*)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(service_uuid${index}), (uint8_t*)&service_uuid${index}}`);
			file.line("\t\t},");
			++attributeIndex;
			
			let characteristics = service.characteristics;
			for (let key in characteristics) {
				let characteristic = characteristics[key];
				
				// characteristic declaration
				let properties = this.parseProperties(characteristic.properties.split(","));
				let permissions = this.parsePermissions(characteristic.permissions.split(","));
				file.line("\t\t[", attributeIndex, "] = {");
				file.line("\t\t\t{ESP_GATT_AUTO_RSP},");
				file.line(`\t\t\t{ESP_UUID_LEN_16, (uint8_t*)&character_declaration_uuid, ${permissions}, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*)&${properties}}`);
				file.line("\t\t},");
				++attributeIndex;

				// characteristic value
				let esp_uuid_len = (4 == characteristic.uuid.length ? "ESP_UUID_LEN_16" : "ESP_UUID_LEN_128");
				file.line("\t\t[", attributeIndex, "] = {");
				if ("value" in characteristic)
					file.line("\t\t\t{ESP_GATT_AUTO_RSP},");
				else {
					let char_name = { service_index:index, att_index:attributeIndex, name:key };
					char_name.type = characteristic.type ? characteristic.type: "";
					char_names.push(char_name);
					file.line("\t\t\t{ESP_GATT_RSP_BY_APP},");
				}
				file.write(`\t\t\t{${esp_uuid_len}, (uint8_t*)&char_uuid${characteristicIndex}, ${permissions}, ${characteristic.maxBytes}, `);
				if ("value" in characteristic)
					file.write(`${characteristic._length}, (uint8_t*)&char_value${characteristicIndex}}`);
				else
					file.write("0, NULL}");
				file.line("");
				file.line("\t\t},");
				++attributeIndex;
				
				// characteristic configuration descriptor
				if (characteristic._notify) {
					file.line("\t\t[", attributeIndex, "] = {");
					file.line("\t\t\t{ESP_GATT_AUTO_RSP},");
					file.line(`\t\t\t{ESP_UUID_LEN_16, (uint8_t*)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(char_ccc${characteristicIndex}), (uint8_t*)char_ccc${characteristicIndex}}`);
					file.line("\t\t},");
					++attributeIndex;
				}
				++characteristicIndex;
			}
			file.line("\t},");
		});
		file.line("};");
		file.line("");
		
		file.line(`#define char_name_count ${char_names.length}`);
		file.line(`static const char_name_table char_names[${char_names.length}] = {`);
		char_names.forEach(entry => {
			file.line(`\t{${entry.service_index}, ${entry.att_index}, "${entry.name}", "${entry.type}"},`);
		});
		file.line("};");
		file.line("");
	}
	parseProperties(properties) {
		const ESP_GATT_CHAR_PROP_BIT_READ = (1 << 1);
		const ESP_GATT_CHAR_PROP_BIT_WRITE = (1 << 3);
		const ESP_GATT_CHAR_PROP_BIT_NOTIFY = (1 << 4);
		const ESP_GATT_CHAR_PROP_BIT_INDICATE = (1 << 5);
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
				case "indicate":
					props |= ESP_GATT_CHAR_PROP_BIT_INDICATE;
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
		else if (props == (ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_INDICATE))
			props = "char_prop_read_indicate";
		else
			throw new Error("unsupported property combination");
		return props;
	}
	parsePermissions(permissions) {
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
};

class GeckoGATTFile extends GATTFile {
	generate() {
		let file = this.file;
		let services = this.services;
		file.line('/* WARNING: This file is automatically generated. Do not edit. */');
		file.line('');
		file.line('#include "bg_gattdb_def.h"');
		file.line('');
		file.line('#ifdef __GNUC__');
		file.line('\t#define GATT_HEADER(F) F __attribute__ ((section (".gatt_header")))');
		file.line('\t#define GATT_DATA(F) F __attribute__ ((section (".gatt_data")))');
		file.line('#else');
		file.line('\t#ifdef __ICCARM__');
		file.line('\t\t#define GATT_HEADER(F) _Pragma("location=\\".gatt_header\\"") F');
		file.line('\t\t#define GATT_DATA(F) _Pragma("location=\\".gatt_data\\"") F');
		file.line('\t#else');
		file.line('\t\t#define GATT_HEADER(F) F');
		file.line('\t\t#define GATT_DATA(F) F');
		file.line('\t#endif');
		file.line('#endif');
		file.line('');
		file.line("typedef struct {");
		file.line("\tuint16_t handle;");
		file.line("\tconst char *name;");
		file.line("\tconst char *type;");
		file.line("} char_name_table;");
		file.line("");
		let char_names = [];
		let uuidtable_16_map = ["0x2800","0x2801","0x2803"];
		let uuidtable_128_map = [];
		let dynamic_mapping_map = [];
		let attributes_max = services.length;
		let attributes_dynamic_max = 0;
		let needs_cccd = true;
		services.forEach((service, index) => {
			uuidtable_16_map.push('0x' + service.uuid);
			let characteristics = service.characteristics;
			let length = Object.keys(characteristics).length;
			attributes_max += (length * 2);
			for (let key in characteristics) {
				let characteristic = characteristics[key];
				if (!characteristic.hasOwnProperty("value"))
					attributes_dynamic_max += 1;
				uuidtable_16_map.push('0x' + characteristic.uuid);
				let properties = characteristic.properties;
				if (properties.includes("notify") || properties.includes("indicate")) {
					++attributes_max;
					characteristic._flags = properties.includes("notify") ? 1 : 2;
					if (needs_cccd) {
						uuidtable_16_map.push('0x2902');
						needs_cccd = false;
					}
				}
			}
		});
		file.line("GATT_DATA(const uint16_t bg_gattdb_data_uuidtable_16_map[]) = {");
		file.write('\t' + uuidtable_16_map.join(','));
		file.line('');
		file.line("};")
		file.line("GATT_DATA(const uint8_t bg_gattdb_data_uuidtable_128_map[]) = {0x00};");
		file.line('');
		file.line("GATT_DATA(const uint8_t bg_gattdb_data_adv_uuid16_map[]) = {0x00};");
		file.line("GATT_DATA(const uint8_t bg_gattdb_data_adv_uuid128_map[]) = {0x00};");
		file.line('');
		let field_index = 0;
		let clientconfig_index = 0;
		let index = 0;
		let buffer;
		
		services.forEach((service) => {
			buffer = uuid16toBuffer(parseInt(service.uuid, 16));
			
			// service
			file.line(`GATT_DATA(const struct bg_gattdb_buffer_with_len bg_gattdb_data_attribute_field_${field_index}) = {`);
			file.line(`\t.len = ${buffer.byteLength},`);
			file.write('\t.data = {');
			file.write(buffer2hexlist(buffer));
			file.write('}');
			file.line('');
			file.line('};');
			++field_index;
			let characteristics = service.characteristics;
			for (let key in characteristics) {
				let characteristic = characteristics[key];
				let uuid = parseInt(characteristic.uuid, 16);
				let data = new Uint8Array(5);
				let properties = this.parseProperties(characteristic.properties.split(","));
				data[0] = properties;
				data[1] = (field_index + 2) & 0xFF;
				data[2] = ((field_index + 2) >> 8) & 0xFF;
				data[3] = uuid & 0xFF;
				data[4] = (uuid >> 8) & 0xFF;
				
				// characteristic declaration
				file.line(`GATT_DATA(const struct bg_gattdb_buffer_with_len bg_gattdb_data_attribute_field_${field_index}) = {`);
				file.line(`\t.len = ${data.buffer.byteLength},`);
				file.write('\t.data = {');
				file.write(buffer2hexlist(data.buffer));
				file.write('}');
				file.line('');
				file.line('};');
				dynamic_mapping_map.push('0x' + parseInt(field_index + 2).toString(16));
				++field_index;

				// characteristic value
				let maxBytes = characteristic.maxBytes;
				if (characteristic.hasOwnProperty("value")) {
					buffer = typedValueToBuffer(characteristic.type, characteristic.value);
					file.write(`uint8_t bg_gattdb_data_attribute_field_${field_index}_data[${maxBytes}] = {`);
					file.write(buffer2hexlist(buffer));
					file.write('};');
					file.line('');
				}
				else {
					let char_name = { handle:field_index + 1, name:key };
					char_name.type = characteristic.type ? characteristic.type: "";
					char_names.push(char_name);
				}
				file.line(`GATT_DATA(const struct bg_gattdb_attribute_chrvalue bg_gattdb_data_attribute_field_${field_index}) = {`);
				file.line(`\t.properties = 0x${properties.toString(16)},`);
				file.line(`\t.index = ${index},`);
				if (characteristic.hasOwnProperty("value")) {
					file.line(`\t.max_len = ${maxBytes},`);
					file.line(`\t.data = bg_gattdb_data_attribute_field_${field_index}_data`);
				}
				else {
					file.line(`\t.max_len = 0,`);
					file.line(`\t.data = NULL`);
				}
				file.line('};');
				field_index += characteristic._flags ? 2 : 1;
				++index;
			}
		});
		file.line("GATT_DATA(const uint16_t bg_gattdb_data_attributes_dynamic_mapping_map[]) = {");
		file.write('\t' + dynamic_mapping_map.join(','));
		file.line('');
		file.line("};")
		file.line('');

		let _uuid = 2;
		field_index = 0;
		file.line('GATT_DATA(const struct bg_gattdb_attribute bg_gattdb_data_attributes_map[]) = {');
		services.forEach((service) => {
			file.line(`\t{.uuid = 0x00, .permissions = 0x801, .caps = 0xFFFF, .datatype = 0x00, .min_key_size = 0x00, .constdata = &bg_gattdb_data_attribute_field_${field_index}},`);
			++_uuid;
			++field_index;
			let characteristics = service.characteristics;
			for (let key in characteristics) {
				let characteristic = characteristics[key];
				file.line(`\t{.uuid = 0x02, .permissions = 0x801, .caps = 0xFFFF, .datatype = 0x00, .min_key_size = 0x00, .constdata = &bg_gattdb_data_attribute_field_${field_index}},`);
				++field_index;
				++_uuid;
				let datatype = (characteristic.hasOwnProperty("value") ? 1 : 7);
				let permissions = characteristic._flags ? "0x800" : "0x" + this.parsePermissions(characteristic.permissions.split(",")).toString(16);
				file.line(`\t{.uuid = ${toPaddedHex(_uuid)}, .permissions = ${permissions}, .caps = 0xFFFF, .datatype = ${toPaddedHex(datatype)}, .min_key_size = 0x00, .dynamicdata = &bg_gattdb_data_attribute_field_${field_index}},`);
				++field_index;
				if (characteristic._flags) {
					let index = dynamic_mapping_map.findIndex(element => field_index == parseInt(element, 16));
					++_uuid;
					file.line(`\t{.uuid = ${toPaddedHex(_uuid)}, .permissions = 0x807, .caps = 0xFFFF, .datatype = 0x03, .min_key_size = 0x00, .configdata = {.flags = ${toPaddedHex(characteristic._flags)}, .index = ${toPaddedHex(index)}, .clientconfig_index = ${toPaddedHex(clientconfig_index)}}},`);
					++field_index;
					++clientconfig_index;
				}
			}
		});
		file.line('};');
		file.line('');
		file.line('GATT_HEADER(const struct bg_gattdb_def bg_gattdb_data) = {');
		file.line('\t.attributes = bg_gattdb_data_attributes_map,');
		file.line(`\t.attributes_max = ${attributes_max},`);
		file.line(`\t.uuidtable_16_size = ${uuidtable_16_map.length},`);
		file.line('\t.uuidtable_16 = bg_gattdb_data_uuidtable_16_map,');
		file.line(`\t.uuidtable_128_size = ${uuidtable_128_map.length},`);
		file.line('\t.uuidtable_128 = bg_gattdb_data_uuidtable_128_map,');
		file.line(`\t.attributes_dynamic_max = ${attributes_dynamic_max},`);
		file.line('\t.attributes_dynamic_mapping = bg_gattdb_data_attributes_dynamic_mapping_map,');
		file.line('\t.adv_uuid16 = bg_gattdb_data_adv_uuid16_map,');
		file.line('\t.adv_uuid16_num = 0,');
		file.line('\t.adv_uuid128 = bg_gattdb_data_adv_uuid128_map,');
		file.line('\t.adv_uuid128_num = 0,');
		file.line('\t.caps_mask = 0xffff,');
		file.line('\t.enabled_caps = 0xffff');
		file.line('};');
		file.line('');
		file.line(`#define char_name_count ${char_names.length}`);
		file.line(`static const char_name_table char_names[${char_names.length}] = {`);
		char_names.forEach(entry => {
			file.line(`\t{${entry.handle}, "${entry.name}", "${entry.type}"},`);
		});
		file.line("};");
		file.line("");		
	}
	parseProperties(properties) {
		const gatt_char_prop_read = 0x02;
		const gatt_char_prop_write = 0x08;
		const gatt_char_prop_notify = 0x10;
		const gatt_char_prop_indicate = 0x20;
		let props = 0;
		properties.forEach(p => {
			switch(p.trim()) {
				case "read":
					props |= gatt_char_prop_read;
					break;
				case "write":
					props |= gatt_char_prop_write;
					break;
				case "notify":
					props |= gatt_char_prop_notify;
					break;
				case "indicate":
					props |= gatt_char_prop_indicate;
					break;
				default:
					throw new Error("unknown property");
			}
		});
		return props;
	}
	parsePermissions(permissions) {
		const gatt_att_perm_readable = 0x0001;
		const gatt_att_perm_writable = 0x0002;
		const gatt_att_perm_write_no_response = 0x0004;
		const gatt_att_perm_discoverable = 0x0800;
		let perms = gatt_att_perm_discoverable;
		permissions.forEach(p => {
			switch(p.trim()) {
				case "read":
					perms |= gatt_att_perm_readable;
					break;
				case "write":
					perms |= gatt_att_perm_writable;
					break;
				default:
					throw new Error("unsupported permission");
			}
		});
		return perms;
	}
};

export default class extends TOOL {
	constructor(argv) {
		super(argv);
		this.windows = this.currentPlatform == "win";
		this.slash = this.windows ? "\\" : "/";
		this.moddablePath = this.getenv("MODDABLE");
		this.name = null;
		this.outputPath = null;
		this.files = [];
		var argc = argv.length;
		var name, path, role;
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
					if (this.outputPath.includes('tmp/esp32') || this.outputPath.includes('tmp\\esp32'))
						this.platform = "esp32";
					else if (this.outputPath.includes('tmp/gecko') || this.outputPath.includes('tmp\\gecko'))
						this.platform = "gecko";
					else
						throw new Error("unknown platform");
					break;
				case "-r":
					argi++;	
					if (argi >= argc)
						throw new Error("-r: no role!");
					this.role = argv[argi];
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
		if (!this.role)
			this.role = "none";
	}
	run() {
		var path = this.joinPath({directory: this.outputPath, name:this.name});
		var file = new FILE(path);
		var role = this.role;
		var services = [];
		this.files.forEach((path, index) => {
			services = services.concat(JSON.parse(this.readFileString(path)).service);
		});
		var gatt;
		if ("esp32" == this.platform)
			gatt = new ESP32GATTFile({ tool:this, role, file, services });
		else if ("gecko" == this.platform)
			gatt = new GeckoGATTFile({ tool:this, role, file, services });
		gatt.generate();
		file.close();
	}
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

function toPaddedHex(value) {
	return "0x" + ("00" + value.toString(16)).slice(-2);
}

function uuid128toBuffer(uuid128) {
	let uuid = uuid128.replace(/-/g,'');
	let array = new Uint8Array(16);
	for (let i = 0; i < 16; ++i)
		array[15 - i] = parseInt(uuid.slice(2 * i, 2 * i + 2), 16);
	return array.buffer;
}

function uuid16toBuffer(uuid16) {
	return (new Uint8Array([uuid16 & 0xFF, (uuid16 >> 8) & 0xFF])).buffer;
}

function typedValueToBuffer(type, value) {
	let buffer;
	switch(type) {
		case "Array":
			buffer = new Uint8Array(value).buffer;
			break;
		case "String":
			buffer = ArrayBuffer.fromString(value);
			break;
		case "Uint8":
			buffer = new Uint8Array([value & 0xFF]).buffer;
			break;
		case "Uint16":
			buffer = new Uint8Array([value & 0xFF, (value >> 8) & 0xFF]).buffer;
			break;
		case "Uint32":
			buffer = new Uint8Array([value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF]).buffer;
			break;
		default:
			buffer = value;
			break;
	}
	return buffer;
}
