/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

let BinaryMessage = {
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

Object.freeze(BinaryMessage);
Object.freeze(BinaryMessage.Float32);
Object.freeze(BinaryMessage.Float64);
Object.freeze(BinaryMessage.Int8);
Object.freeze(BinaryMessage.Int16);
Object.freeze(BinaryMessage.Int32);
Object.freeze(BinaryMessage.Uint8);
Object.freeze(BinaryMessage.Uint16);
Object.freeze(BinaryMessage.Uint32);
Object.freeze(BinaryMessage.XX);
Object.freeze(BinaryMessage.XX);

export default BinaryMessage;
