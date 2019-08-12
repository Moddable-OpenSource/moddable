/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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

const BinaryMessage = {
	Float32: {
		bytesLength:4,
		getter:DataView.prototype.getFloat32,
		setter:DataView.prototype.setFloat32,
	},
	Float64: {
		bytesLength:8,
		getter:DataView.prototype.getFloat64,
		setter:DataView.prototype.setFloat64,
	},
	Int8: {
		bytesLength:1,
		getter:DataView.prototype.getInt8,
		setter:DataView.prototype.setInt8,
	},
	Int16: {
		bytesLength:2,
		getter:DataView.prototype.getInt16,
		setter:DataView.prototype.setInt16,
	},
	Int32: {
		bytesLength:4,
		getter:DataView.prototype.getInt32,
		setter:DataView.prototype.setInt32,
	},
	Uint8: {
		bytesLength:1,
		getter:DataView.prototype.getUint8,
		setter:DataView.prototype.setUint8,
	},
	Uint16: {
		bytesLength:2,
		getter:DataView.prototype.getUint16,
		setter:DataView.prototype.setUint16,
	},
	Uint32: {
		bytesLength:4,
		getter:DataView.prototype.getUint32,
		setter:DataView.prototype.setUint32,
	},
	generate(format) {
		let message = {};
		format.forEach(item => {
			let value = 0;
			let range = item.range;
			if (range) 
				value = range[value];
			message[item.name] = value;
		});
		return message;
	},
	parse(format, buffer, endian) {
		let view = new DataView(buffer);
		let message = {};
		let bytesLength = 0;
		format.forEach(item => {
			let value = item.type.getter.call(view, bytesLength, endian);
			let range = item.range;
			if (range) 
				value = range[value];
			message[item.name] = value;
			bytesLength += item.type.bytesLength;
		});
		return message;
	},
	serialize(format, message, endian) {
		let bytesLength = format.reduce((sum, item) => sum + item.type.bytesLength, 0);
		let buffer = new ArrayBuffer(bytesLength);
		let view = new DataView(buffer);
		bytesLength = 0;
		format.forEach(item => {
			let value = message[item.name];
			let range = item.range;
			if (range) 
				value = range.findIndex(string => string == value);
			item.type.setter.call(view, bytesLength, value, endian);
			bytesLength += item.type.bytesLength;
		});
		return buffer;
	},
}

Object.freeze(BinaryMessage, true);

export default BinaryMessage;
