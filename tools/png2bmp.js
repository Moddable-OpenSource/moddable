/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

import { TOOL } from "tool";
import Bitmap from "commodetto/Bitmap";
import BMPOut from "commodetto/BMPOut";
import Convert from "commodetto/Convert";
import PNG from "commodetto/ReadPNG";
import PixelsOut from "commodetto/PixelsOut";
import File from "file";

var formatNames = {
	monochrome: "monochrome",
	gray16: "gray16",
	gray256: "gray256",
	rgb332: "rgb332",
	rgb565le: "rgb565le",
	rgb565be: "rgb565be",
	clut16: "clut16",
	argb4444: "argb4444",
	bgra32: "bgra32",
};

var formatValues = {
	monochrome: Bitmap.Monochrome,
	gray16: Bitmap.Gray16,
	gray256: Bitmap.Gray256,
	rgb332: Bitmap.RGB332,
	rgb565le: Bitmap.RGB565LE,
	rgb565be: Bitmap.RGB565BE,
	clut16: Bitmap.CLUT16,
	argb4444: Bitmap.ARGB4444,
	bgra32: Bitmap.BGRA32,
};

export default class extends TOOL {
	constructor(argv) {
		super(argv);
		this.alpha = true;
		this.alphaFormat = formatValues.gray16;
		this.bm4 = false;
		this.color = true;
		this.colorFormat = 0;
		this.name = "";
		this.rotation = 0;
		this.inputPaths = [];
		this.outputPath = null;
		this.temporary = false;
		var argc = argv.length;
		for (var argi = 1; argi < argc; argi++) {
			var option = argv[argi], name, path;
			switch (option) {
			case "-a":
				this.color = false;
				break;
			case "-c":
				this.alpha = false;
				break;
			case "-clut":
				argi++;	
				if (argi >= argc)
					throw new Error("-clut: no path!");
				path = this.resolveFilePath(argv[argi]);
				if (this.clut)
					throw new Error("-clut '" + name + "': too many CLUTs!");
				if (!path)
					throw new Error("'" + name + "': file not found!");
				this.clut = path;
				break;
			case "-f":
				argi++;	
				if (argi >= argc)
					throw new Error("-f: no format!");
				name = argv[argi];
				if (this.colorFormat)
					throw new Error("-f '" + name + "': too many formats!");
				name = name.toLowerCase();
				if (name in formatNames)
					name = formatNames[name];
				else
					throw new Error("-f '" + name + "': unknown format!");
				this.colorFormat = formatValues[name];
				break;
			case "-m":
				if (this.colorFormat)
					throw new Error("-m: too many formats!");
				this.alphaFormat = formatValues.monochrome;
				this.colorFormat = formatValues.monochrome;
				break;
			case "-n":
				argi++;	
				if (argi >= argc)
					throw new Error("-n: no name!");
				name = argv[argi];
				if (this.name)
					throw new Error("-n '" + name + "': too many names!");
				this.name = name;
				break;
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
			case "-r":
				argi++;	
				if (argi >= argc)
					throw new Error("-r: no rotation!");
				name = parseInt(argv[argi]);
				if ((name != 0) && (name != 90) && (name != 180) && (name != 270))
					throw new Error("-r: " + name + ": invalid rotation!");
				this.rotation = name;
				break;
			case "-t":
				this.temporary = true;
				break;
			case "-4":
				this.bm4 = true;
				break;
			default:
				name = argv[argi];
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': file not found!");
				this.inputPaths.push(path);
				break;
			}
		}
		if (!this.colorFormat) {
			this.colorFormat = 7;
		}
		if (this.inputPaths.length == 0) {
			throw new Error("no file!");
		}
		if (!this.outputPath)
			this.outputPath = this.splitPath(this.inputPaths[0]).directory;
	}
	checkPNG(png) {
		let pngChannels = png.channels;
		if (png.depth != 8)
			return false;
		if ((png.channels == 1) && (png.palette)) {
			png.palette = new Uint8Array(png.palette);
			return true;
		}
		return (png.channels == 2) || (png.channels == 3) || (png.channels == 4);
	}
	pad(width) {
		const colorDepth = (this.color) ? Bitmap.depth(this.colorFormat) : 32;
		const alphaDepth = (this.alpha) ? Bitmap.depth(this.alphaFormat) : 32;
		const depth = Math.min(colorDepth, alphaDepth);
		const alignment = this.bm4 ? 8 : 32;
		const multiple = (depth < alignment) ? (alignment / depth) : 1;
		const overflow = width & (multiple - 1);
		if (overflow)
			width += multiple - overflow;
		return width;
	}
	rotate90(buffer, width, height) {
		let result = new Uint8Array(height * width * 4);
		let v = 0;
		while (v < height) {
			let u = 0;
			while (u < width) {
				let x = height - 1 - v;
				let y = u;
				let src = ((v * width) + u) * 4;
				let dst = ((y * height) + x) * 4;
				result[dst++] = buffer[src++];
				result[dst++] = buffer[src++];
				result[dst++] = buffer[src++];
				result[dst++] = buffer[src++];
				u++;
			}	
			v++;
		}
		return result;
	}
	rotate180(buffer, width, height) {
		let result = new Uint8Array(height * width * 4);
		let v = 0;
		while (v < height) {
			let u = 0;
			while (u < width) {
				let x = width - 1 - u;
				let y = height - 1 - v;
				let src = ((v * width) + u) * 4;
				let dst = ((y * width) + x) * 4;
				result[dst++] = buffer[src++];
				result[dst++] = buffer[src++];
				result[dst++] = buffer[src++];
				result[dst++] = buffer[src++];
				u++;
			}	
			v++;
		}
		return result;
	}
	rotate270(buffer, width, height) {
		let result = new Uint8Array(height * width * 4);
		let v = 0;
		while (v < height) {
			let u = 0;
			while (u < width) {
				let x = v;
				let y = width - 1 - u;
				let src = ((v * width) + u) * 4;
				let dst = ((y * height) + x) * 4;
				result[dst++] = buffer[src++];
				result[dst++] = buffer[src++];
				result[dst++] = buffer[src++];
				result[dst++] = buffer[src++];
				u++;
			}	
			v++;
		}
		return result;
	}
	transferLine(png, buffer, offset) {
		let pngLine = png.read();
		let pngOffset = 0;
		let pngX = 0;
		let pngWidth = png.width;
		if (png.channels == 1) {
			let palette = png.palette;
			while (pngX < pngWidth) {
				let index = 4 * pngLine[pngOffset++];
				buffer[offset++] = palette[index++];
				buffer[offset++] = palette[index++];
				buffer[offset++] = palette[index++];
				buffer[offset++] = palette[index++];
				pngX++;
			}
		}
		else if (png.channels == 2) {
			while (pngX < pngWidth) {
				let gray = pngLine[pngOffset++];
				buffer[offset++] = gray;
				buffer[offset++] = gray;
				buffer[offset++] = gray;
				buffer[offset++] = pngLine[pngOffset++];
				pngX++;
			}
		}
		else if (png.channels == 3) {
			while (pngX < pngWidth) {
				buffer[offset++] = pngLine[pngOffset++];
				buffer[offset++] = pngLine[pngOffset++];
				buffer[offset++] = pngLine[pngOffset++];
				buffer[offset++] = 255;
				pngX++;
			}
		}
		else {
			while (pngX < pngWidth) {
				buffer[offset++] = pngLine[pngOffset++];
				buffer[offset++] = pngLine[pngOffset++];
				buffer[offset++] = pngLine[pngOffset++];
				buffer[offset++] = pngLine[pngOffset++];
				pngX++;
			}
		}
	} 
	
	run() {
		let inputPaths = this.inputPaths;
		let c = inputPaths.length;
		let width, height;
		let buffer;
		let offset = 0;
		let y = 0;
		if (c == 1) {
			let pngPath = inputPaths[0];
			let png = new PNG(this.readFileBuffer(pngPath));
			if (!this.checkPNG(png))
				throw new Error("'" + pngPath + "': invalid PNG format!");
			let pngWidth = png.width;
			let pngHeight = png.height;
			if ((this.rotation == 0) || (this.rotation == 180)) {
				width = this.pad(pngWidth);
				height = pngHeight;
			}
			else {
				width = pngWidth;
				height = this.pad(pngHeight);
			}
			buffer = new Uint8Array(width * height * 4);
			while (y < pngHeight) {
				let x = 0;
				this.transferLine(png, buffer, offset);
				offset += (pngWidth * 4);
				x += pngWidth;
				while (x < width) {
					buffer[offset++] = 0;
					buffer[offset++] = 0;
					buffer[offset++] = 0;
					buffer[offset++] = 255;
					x++;
				}
				y++;
			}
		}
		else {
			let pngs = new Array(c).fill();
			let pngWidth, pngHeight;
			for (var i = 0; i < c; i++) {
				let pngPath = inputPaths[i];
				let png = new PNG(this.readFileBuffer(pngPath));
				if (!this.checkPNG(png))
					throw new Error("'" + pngPath + "': invalid PNG format!");
				if (i == 0) {
					pngWidth = png.width;
					pngHeight = png.height;
				}
				else {
					if (pngWidth != png.width)
						throw new Error("'" + pngPath + "': invalid width!");
					if (pngHeight != png.height)
						throw new Error("'" + pngPath + "': invalid height!");
				}
				pngs[i] = png;
			}
			let stripWidth = pngWidth * i;
			if ((this.rotation == 0) || (this.rotation == 180)) {
				width = this.pad(stripWidth);
				height = pngHeight;
			}
			else {
				width = stripWidth;
				height = this.pad(pngHeight);
			}
			buffer = new Uint8Array(width * height * 4);
			while (y < pngHeight) {
				let x = 0;
				for (var i = 0; i < c; i++) {
					this.transferLine(pngs[i], buffer, offset);
					offset += (pngWidth * 4);
					x += pngWidth;
				}
				while (x < width) {
					buffer[offset++] = 0;
					buffer[offset++] = 0;
					buffer[offset++] = 0;
					buffer[offset++] = 255;
					x++;
				}
				y++;
			}
		}
		while (y < height) {
			let x = 0;
			while (x < width) {
				buffer[offset++] = 0;
				buffer[offset++] = 0;
				buffer[offset++] = 0;
				buffer[offset++] = 255;
				x++;
			}
			y++;
		}
		let tmp;
		switch (this.rotation) {
		case 90:
			buffer = this.rotate90(buffer, width, height);
			tmp = width;
			width = height;
			height = tmp;
			break;
		case 180:
			buffer = this.rotate180(buffer, width, height);
			break;
		case 270:
			buffer = this.rotate270(buffer, width, height);
			tmp = width;
			width = height;
			height = tmp;
			break;
		}
			
		let colorPath, colorOut, colorConvert, colorLine;
		let alphaPath, alphaOut, alphaConvert, alphaLine;
		
		let convertRGBA32Line = new Uint8Array(width * 4);
		let convertGray256Line = new Uint8Array(width);

		if (this.color) {
			let parts = this.splitPath(inputPaths[0]);
			if (this.name)
				parts.name = this.name;
			parts.directory = this.outputPath;
			parts.name += "-color";
			parts.extension = this.bm4 ? ".bm4" : ".bmp";
			colorPath = this.joinPath(parts);
			let dictionary = {width, height, pixelFormat:this.colorFormat, path:colorPath};
			let clut;
			if (this.clut)
				clut = dictionary.clut = this.readFileBuffer(this.clut);
			colorOut = new (this.bm4 ? BM4Out : BMPOut)(dictionary);
			colorConvert = new Convert(Bitmap.RGBA32, this.colorFormat, clut);
			colorLine = new Uint8Array(colorOut.pixelsToBytes(width));
			colorOut.begin(0, 0, width, height);
		}
	
		if (this.alpha) {
			let parts = this.splitPath(inputPaths[0]);
			if (this.name)
				parts.name = this.name;
			parts.directory = this.outputPath;
			parts.name += "-alpha";
			parts.extension = this.bm4 ? ".bm4" : ".bmp";
			alphaPath = this.joinPath(parts);
			let dictionary = {width, height, pixelFormat:this.alphaFormat, path:alphaPath};
			alphaOut = new (this.bm4 ? BM4Out : BMPOut)(dictionary);
			alphaConvert = new Convert(Bitmap.Gray256, this.alphaFormat);
			alphaLine = new Uint8Array(alphaOut.pixelsToBytes(width));
			alphaOut.begin(0, 0, width, height);
		}
	
		offset = 0;
		y = 0;
		while (y < height) {
			let colorIndex = 0;
			let alphaIndex = 0;
			while (alphaIndex < width) {
				let r = convertRGBA32Line[colorIndex++] = buffer[offset++];
				let g = convertRGBA32Line[colorIndex++] = buffer[offset++];
				let b = convertRGBA32Line[colorIndex++] = buffer[offset++];
				let a = convertRGBA32Line[colorIndex++] = buffer[offset++];
				convertGray256Line[alphaIndex++] = 255 - a;
			}
			if (this.color) {
				colorConvert.process(convertRGBA32Line.buffer, colorLine.buffer);
				colorOut.send(colorLine.buffer);
			}
			if (this.alpha) {
				alphaConvert.process(convertGray256Line.buffer, alphaLine.buffer);
				alphaOut.send(alphaLine.buffer);
			}
			y++;
		}
		if (this.color)
			colorOut.end();
		if (this.alpha)
			alphaOut.end();
	}
}

class BM4Out extends PixelsOut {
	constructor(dictionary) {
		super(dictionary);

		const pixelFormat = this.pixelFormat;
		this.depth = Bitmap.depth(pixelFormat);

		if ((Bitmap.Gray16 != pixelFormat) &&
			(Bitmap.Gray256 != pixelFormat) &&
			(Bitmap.RGB565LE != pixelFormat) &&
			(Bitmap.RGB332 != pixelFormat) &&
			(Bitmap.CLUT16 != pixelFormat) &&
			(Bitmap.Monochrome != pixelFormat))
			throw new Error("unsupported BM4 pixel fornat");

		this.file = new File(dictionary.path, 1);
	}
	begin(x, y, width, height) {
		const file = this.file;

		// header: 'md', version, Commodetto pixel format, width and height (big endian)
		file.write(	'm'.charCodeAt(0),
					'd'.charCodeAt(0),
					0,
					this.pixelFormat,
					this.width >> 8,
					this.width & 0xFF,
					this.height >> 8,
					this.height & 0xFF);
	}
	send(pixels, offset = 0, count = pixels.byteLength - offset) {
		if ((0 == offset) && (count == pixels.byteLength) && (pixels instanceof ArrayBuffer))		//@@ file.write should support HostBuffer
			this.file.write(pixels);
		else {
			let bytes = new Uint8Array(pixels);
			while (count--)
				this.file.write(bytes[offset++]);
		}
	}
	end() {
		this.file.close();
		delete this.file;
	}
}
