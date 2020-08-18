/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
import Bitmap from "commodetto/Bitmap";
import ColorCellOut from "commodetto/ColorCellOut";
import Convert from "commodetto/Convert";
import JPEG from "commodetto/ReadJPEG";
import PNG from "commodetto/ReadPNG";

export default class extends TOOL {
	constructor(argv) {
		super(argv);
		this.directory = false;
		this.format = Bitmap.RGB565LE;
		this.quality = undefined;
		this.rotation = 0;
		this.inputPath = null;
		this.outputPath = null;
		this.windows = this.currentPlatform == "win";
		this.slash = this.windows ? "\\" : "/";
		var argc = argv.length;
		for (var argi = 1; argi < argc; argi++) {
			var option = argv[argi], name, path;
			switch (option) {
			case "-o":
				argi++;	
				if (argi >= argc)
					throw new Error("-o: no directory!");
				name = argv[argi];
				if (this.outputPath)
					throw new Error("-o '" + name + "': too many directories!");
				path = this.resolveDirectoryPath(name);
				if (!path)
					throw new Error("-o '" + name + "': directory not found!");
				this.outputPath = path;
				break;
			case "-q":
				argi++;	
				if (argi >= argc)
					throw new Error("-q: no quality!");
				name = parseInt(argv[argi]);
				if ((name < 0) || (100 < name))
					throw new Error("-q: " + name + ": invalid quality!");
				this.quality = name;
				break;
			case "-r":
				argi++;	
				if (argi >= argc)
					throw new Error("-r: no rotation!");
				name = parseInt(argv[argi]);
				if ((name != 0) && (name != 90) && (name != 180) && (name != 270))
					throw new Error("-r: " + name + ": invalid rotation!");
				this.rotation = name;
				break;
			default:
				name = argv[argi];
				if (this.inputPath)
					throw new Error("'" + name + "': too many files!");
				path = this.resolveFilePath(name);
				if (!path) {
					path = this.resolveDirectoryPath(name);
					if (!path)
						throw new Error("'" + name + "': file not found!");
					this.directory = true;
				}
				this.inputPath = path;
				break;
			}
		}
		if (!this.inputPath) {
			throw new Error("no file!");
		}
		if (!this.outputPath)
			this.outputPath = this.splitPath(this.inputPath).directory;
	}
	compress(buffer, width, height) {
		let writer = new ColorCellOut;
		if (this.quality !== undefined)
			writer.begin(0, 0, width, height, { quality:this.quality / 100 });
		else
			writer.begin(0, 0, width, height);
		let from = 0;
		for (let y = 0; y < height; y += 4) {
			let to = from + (8 * width);
			writer.send(buffer.slice(from, to));
			from = to;
		}
		writer.end();
		return writer.bitmap.buffer;
	}
	copy0(block, pixels, width, height) {
		var buffer = new Uint16Array(block.buffer);	
		let v = 0;
		while (v < block.height) {
			let u = 0;
			while (u < block.width) {
				let x = block.x + u;
				let y = block.y + v;
				let src = ((v * block.width) + u);
				let dst = ((y * width) + x);
				pixels[dst] = buffer[src];
				u++;
			}	
			v++;
		}
	}
	copy90(block, pixels, width, height) {
		var buffer = new Uint16Array(block.buffer);	
		let v = 0;
		while (v < block.height) {
			let u = 0;
			while (u < block.width) {
				let x = height - 1 - block.y - v;
				let y = block.x + u;
				let src = ((v * block.width) + u);
				let dst = ((y * height) + x);
				pixels[dst] = buffer[src];
				u++;
			}	
			v++;
		}
	}
	copy180(block, pixels, width, height) {
		var buffer = new Uint16Array(block.buffer);	
		let v = 0;
		while (v < block.height) {
			let u = 0;
			while (u < block.width) {
				let x = width - 1 - block.x - u;
				let y = height - 1 - block.y - v;
				let src = ((v * block.width) + u);
				let dst = ((y * width) + x);
				pixels[dst] = buffer[src];
				u++;
			}	
			v++;
		}
	}
	copy270(block, pixels, width, height) {
		var buffer = new Uint16Array(block.buffer);	
		let v = 0;
		while (v < block.height) {
			let u = 0;
			while (u < block.width) {
				let x = block.y + v;
				let y = width - 1 - block.x - u;
				let src = ((v * block.width) + u);
				let dst = ((y * height) + x);
				pixels[dst] = buffer[src];
				u++;
			}	
			v++;
		}
	}
	pad(dimension) {
		let multiple = 4;
		let overflow = dimension & (multiple - 1);
		if (overflow)
			dimension += multiple - overflow;
		return dimension;
	}
	readGIF(buffer) @ "Tool_readGIF";
	readJPEG(path, frames) {
		var data = this.readFileBuffer(path);
		var jpeg = new JPEG(data, { pixelFormat:Bitmap.RGB565LE });
		var jpegWidth = jpeg.width;
		var jpegHeight = jpeg.height;
		if (this.quality == 0) {
			this.format = Bitmap.JPEG;
			frames.push(data);
			frames.width = jpegWidth;
			frames.height = jpegHeight;
			return;
		}
		var padWidth = this.pad(jpegWidth);
		var padHeight = this.pad(jpegHeight);
		var copy, width, height;
		switch (this.rotation) {
		case 0:
			copy = this.copy0;
			width = padWidth;
			height = padHeight;
			break;
		case 90:
			copy = this.copy90;
			width = padHeight;
			height = padWidth;
			break;
		case 180:
			copy = this.copy180;
			width = padWidth;
			height = padHeight;
			break;
		case 270:
			copy = this.copy270;
			width = padHeight;
			height = padWidth;
			break;
		}
		var pixels = new Uint16Array(width * height);
		for (;;) {
			var block = jpeg.read();
			if (!block)
				break;
			copy.call(this, block, pixels, padWidth, padHeight);
		}
		frames.push(this.compress(pixels.buffer, width, height));
		frames.width = width;
		frames.height = height;
	}
	readPNG(path, frames) {
		var data = this.readFileBuffer(path);
		var png = new PNG(data);
		let pngChannels = png.channels;
		let pngDepth = png.depth;
		let pngWidth = png.width;
		let pngHeight = png.height;
		if (((pngChannels != 3) && (pngChannels != 4)) || (pngDepth != 8))
			throw new Error("'" + pngPath + "': invalid PNG format!");
		if (this.quality == 0) {
			this.format = Bitmap.PNG;
			frames.push(data);
			frames.width = pngWidth;
			frames.height = pngHeight;
			return;
		}
		let padWidth = this.pad(pngWidth);
		let padHeight = this.pad(pngHeight);
		let bufferRGBA32 = new Uint8Array(padWidth * padHeight * 4);
		let offset = 0;
		let y = 0;
		while (y < pngHeight) {
			let pngLine = png.read();
			let pngOffset = 0;
			let x = 0;
			while (x < pngWidth) {
				bufferRGBA32[offset++] = pngLine[pngOffset++];
				bufferRGBA32[offset++] = pngLine[pngOffset++];
				bufferRGBA32[offset++] = pngLine[pngOffset++];
				bufferRGBA32[offset++] = (pngChannels == 4) ? pngLine[pngOffset++] : 255;
				x++;
			}
			while (x < padWidth) {
				bufferRGBA32[offset++] = 0;
				bufferRGBA32[offset++] = 0;
				bufferRGBA32[offset++] = 0;
				bufferRGBA32[offset++] = 255;
				x++;
			}
			y++;
		}
		while (y < padHeight) {
			let x = 0;
			while (x < padWidth) {
				bufferRGBA32[offset++] = 0;
				bufferRGBA32[offset++] = 0;
				bufferRGBA32[offset++] = 0;
				bufferRGBA32[offset++] = 255;
				x++;
			}
			y++;
		}
		let bufferRGB565LE = new Uint16Array(padWidth * padHeight);
		var colorConvert = new Convert(Bitmap.RGBA32, Bitmap.RGB565LE);
		colorConvert.process(bufferRGBA32.buffer, bufferRGB565LE.buffer);
		var pixels, width, height;
		switch (this.rotation) {
		case 0:
			pixels = bufferRGB565LE.buffer;
			width = padWidth;
			height = padHeight;
			break;
		case 90:
			pixels = this.rotate90(bufferRGB565LE.buffer, padWidth, padHeight);
			width = padHeight;
			height = padWidth;
			break;
		case 180:
			pixels = this.rotate180(bufferRGB565LE.buffer, padWidth, padHeight);
			width = padWidth;
			height = padHeight;
			break;
		case 270:
			pixels = this.rotate270(bufferRGB565LE.buffer, padWidth, padHeight);
			width = padHeight;
			height = padWidth;
			break;
		}
		frames.push(this.compress(pixels, width, height));
		frames.width = width;
		frames.height = height;
	}
	rotate90(pixels, width, height) {
		let buffer = new Uint16Array(pixels);
		let result = new Uint16Array(height * width);
		let v = 0;
		while (v < height) {
			let u = 0;
			while (u < width) {
				let x = height - 1 - v;
				let y = u;
				let src = ((v * width) + u);
				let dst = ((y * height) + x);
				result[dst] = buffer[src];
				u++;
			}	
			v++;
		}
		return result.buffer;
	}
	rotate180(pixels, width, height) {
		let buffer = new Uint16Array(pixels);
		let result = new Uint16Array(height * width);
		let v = 0;
		while (v < height) {
			let u = 0;
			while (u < width) {
				let x = width - 1 - u;
				let y = height - 1 - v;
				let src = ((v * width) + u);
				let dst = ((y * width) + x);
				result[dst] = buffer[src];
				u++;
			}	
			v++;
		}
		return result.buffer;
	}
	rotate270(pixels, width, height) {
		let buffer = new Uint16Array(pixels);
		let result = new Uint16Array(height * width);
		let v = 0;
		while (v < height) {
			let u = 0;
			while (u < width) {
				let x = v;
				let y = width - 1 - u;
				let src = ((v * width) + u);
				let dst = ((y * height) + x);
				result[dst] = buffer[src];
				u++;
			}	
			v++;
		}
		return result.buffer;
	}
	run() {
		var fps_numerator;
		var fps_denominator;
		var frames = new Array;
		if (this.directory) {
			var names = this.enumerateDirectory(this.inputPath);
			names.sort();
			var c = names.length;
			for (var i = 0; i < c; i++) {
				var path = this.inputPath + this.slash + names[i];
				var parts = this.splitPath(path);
				if (parts.extension == ".png") {
					this.readPNG(path, frames);
				}
				else if ((parts.extension == ".jpg") || (parts.extension == ".jpeg")) {
					this.readJPEG(path, frames);
				}
			}
			var parts = this.splitPath(this.inputPath);
			var a = parts.extension.match(/\.([0-9]+)fps/);
			fps_numerator = 1;
			fps_denominator = (a && (a.length == 2) && (a[1] !== undefined)) ? parseInt(a[1]) : 30;
		}
		else {
			var parts = this.splitPath(this.inputPath);
			if (parts.extension == ".gif") {
				var array = this.readGIF(this.readFileBuffer(this.inputPath));
				var gifWidth = array.width;
				var gifHeight = array.height;
				var gifDuration = array.reduce((sum, item) => sum + item.delay, 0);
				var width, height;
				switch (this.rotation) {
				case 0:
				case 180:
					width = gifWidth;
					height = gifHeight;
					break;
				case 90:
				case 270:
					width = gifHeight;
					height = gifWidth;
					break;
				}
				var c = array.length;
				for (var i = 0; i < c; i++) {
					var pixels;
					switch (this.rotation) {
					case 0:
						pixels = array[i];
						break;
					case 90:
						pixels = this.rotate90(array[i], gifWidth, gifHeight);
						break;
					case 180:
						pixels = this.rotate180(array[i], gifWidth, gifHeight);
						break;
					case 270:
						pixels = this.rotate270(array[i], gifWidth, gifHeight);
						break;
					}
					frames.push(this.compress(pixels, width, height));
				}
				frames.width = width;
				frames.height = height;
				fps_numerator = gifDuration / c;
				fps_denominator = 1000;
			}
			else if (parts.extension == ".png") {
				this.readPNG(this.inputPath, frames);
				fps_numerator = 1;
				fps_denominator = 1;
			}
			else if ((parts.extension == ".jpg") || (parts.extension == ".jpeg")) {
				this.readJPEG(this.inputPath, frames);
				fps_numerator = 1;
				fps_denominator = 1;
			}
			else
				throw new Error("'" + this.inputPath + "': invalid file!");
		}
		
		var frameCount = frames.length;
		var width = frames.width;
		var height = frames.height;
		parts.directory = this.outputPath;
		parts.extension = ".cs";
		if (this.quality !== undefined)
			parts.extension += this.quality;
		let output = new FILE(this.joinPath(parts), "wb");
		output.writeByte('c'.charCodeAt(0));
		output.writeByte('s'.charCodeAt(0));
		output.writeByte(this.format);
		output.writeByte(0);
		output.writeByte(width & 0xFF);
		output.writeByte(width >> 8);
		output.writeByte(height & 0xFF);
		output.writeByte(height >> 8);
		output.writeByte(frameCount & 0xFF);
		output.writeByte(frameCount >> 8);
		output.writeByte(fps_numerator & 0xFF);
		output.writeByte(fps_numerator >> 8);
		output.writeByte(fps_denominator & 0xFF);
		output.writeByte(fps_denominator >> 8);
		for (var frameIndex = 0; frameIndex < frameCount; frameIndex++) {
			var buffer = frames[frameIndex];
			var frameSize = buffer.byteLength;
			output.writeByte(frameSize & 255);
			output.writeByte(frameSize >> 8);
			output.writeBuffer(buffer);
		}
		// 0 size frame at the end
		output.writeByte(0);
		output.writeByte(0);
		output.close();
	}
}
