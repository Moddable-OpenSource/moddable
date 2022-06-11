/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import Bitmap from "commodetto/Bitmap";
import parseBMP from "commodetto/parseBMP";
import parseRLE from "commodetto/parseRLE";
import Poco from "commodetto/Poco";
import _Resource from "Resource";
globalThis.Resource = _Resource;

import {} from "piu/All";
export * from "piu/All";

export class CLUT extends Resource {
	constructor(path) {
		super(path);
	}
	get colors() @ "PiuCLUT_get_colors"
}
Object.freeze(CLUT.prototype);
global.CLUT = CLUT;

// PiuTexture.c

export class Texture @ "PiuTextureDelete" {
	constructor(it, alphaBitmap, colorBitmap) {
		if (alphaBitmap || colorBitmap) {
			this._create(alphaBitmap, colorBitmap);
			return;
		}
		let archive;
		if (typeof(it) == "object") {
			archive = it.archive
			if (it.alpha && it.color) {
				alphaBitmap = parseBMP(new Resource(it.alpha, archive));
				colorBitmap = parseBMP(new Resource(it.color, archive));
				this._create(alphaBitmap, colorBitmap);
				return;
			}
			it = it.path;
		}
		if (it.endsWith(".png")) {
			let name = it.slice(0, -4);
			let path = name + "-alpha.bm4";
			if (Resource.exists(path, archive)) {
				alphaBitmap = parseRLE(new Resource(path, archive));
			}
			else {
				path = name + "-alpha.bmp";
				if (Resource.exists(path, archive)) {
					alphaBitmap = parseBMP(new Resource(path, archive));
				}
				path = name + "-color.bmp";
				if (Resource.exists(path, archive)) {
					colorBitmap = parseBMP(new Resource(path, archive));
				}
			}
		}
		else if (it.endsWith("-alpha.bmp"))
			alphaBitmap = parseBMP(new Resource(it, archive));
		else if (it.endsWith("-color.bmp"))
			colorBitmap = parseBMP(new Resource(it, archive));
		if (alphaBitmap || colorBitmap)
			this._create(alphaBitmap, colorBitmap);
		else
			throw new URIError("Texture " + it + " not found!");
	}
	_create(alphaBitmap, colorBitmap) @ "PiuTexture_create"
	get width() @ "PiuTexture_get_width"
	get height() @ "PiuTexture_get_height"
	static template(i) {
		const it = i;
		return function() {
			let texture;
			if (global.assetMap)
				texture = assetMap.get(it);
			else
				global.assetMap = new Map;
			if (!texture) {
				texture = new Texture(it);
				assetMap.set(it, texture);
			}
			return texture;
		}
	}
}
Object.freeze(Texture.prototype);
global.Texture = Texture;

// PiuDie.c

const die = {
	__proto__: Container.prototype,
	_create($, it) @ "PiuDie__create",
	and(x, y, width, height) @ "PiuDie_and",
	attach(content) @ "PiuDie_attach",
	cut() @ "PiuDie_cut",
	detach() @ "PiuDie_detach",
	empty() @ "PiuDie_empty",
	fill() @ "PiuDie_fill",
	or(x, y, width, height) @ "PiuDie_or",
	set(x, y, width, height) @ "PiuDie_set",
	sub(x, y, width, height) @ "PiuDie_sub",
	xor(x, y, width, height) @ "PiuDie_xor",
};
export const Die = Template(die);
Object.freeze(die);
global.Die = Die;

// PiuImage.c

const image = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuImage_create",
// 	_createWith(it) {
// 		const it = it.path;
// 		if (it.endsWith(".png")) {
// 			let name = it.slice(0, -4);
// 			let path = name + "-alpha.bm4";
// 			if (Resource.exists(path)) {
// 				alphaBitmap = parseRLE(new Resource(path));
// 			}
// 			else {
// 				path = name + "-alpha.bmp";
// 				if (Resource.exists(path)) {
// 					alphaBitmap = parseBMP(new Resource(path));
// 				}
// 				path = name + "-color.bmp";
// 				if (Resource.exists(path)) {
// 					colorBitmap = parseBMP(new Resource(path));
// 				}
// 			}
// 		}
// 		else if (it.endsWith("-alpha.bmp"))
// 			alphaBitmap = parseBMP(new Resource(it));
// 		else if (it.endsWith("-color.bmp"))
// 			colorBitmap = parseBMP(new Resource(it));
// 		if (alphaBitmap || colorBitmap)
// 			this._createWithBitmaps(alphaBitmap, colorBitmap);
// 		else
// 			this._createWithColorCells(it);
// 	}
// 	_createWithBitmaps(alphaBitmap, colorBitmap) @ "PiuImage_createWithBitmaps",
// 	_createWithColorCells() @ "PiuImage_createWithColorCells",
	
	get frameCount() @ "PiuImage_get_frameCount",
	get frameIndex() @ "PiuImage_get_frameIndex",
	
	set frameIndex(it) @ "PiuImage_set_frameIndex",
	
	parseBMP(resource) { return parseBMP(resource); },
	parseRLE(resource) { return parseRLE(resource); },
};
export const Image = Template(image);
Object.freeze(image);
global.Image = Image;

// PiuApplication.c

const application = {
	__proto__: Container.prototype,
	_create($, it) @ "PiuApplication_create",
	
	get clut() @ "PiuApplication_get_clut",
	get rotation() @ "PiuApplication_get_rotation",
	
	set clut(it) @ "PiuApplication_set_clut",
	set rotation(it) @ "PiuApplication_set_rotation",
	
	animateColors(colors) @ "PiuApplication_animateColors",
	keyDown(key) @ "PiuApplication_keyDown",
	keyUp(key) @ "PiuApplication_keyUp",
	postMessage(json) @ "PiuApplication_postMessage",
	purge() @ "PiuApplication_purge",
}
export function Application($, it = {}) {
	let self = (this) ? this : Object.create(application);
	it._DeferLink = DeferLink;
	it._Skin = Skin;
	it._Style = Style;
	it._Texture = Texture;
	it._TouchLink = TouchLink;
	it._View = View;
	global.application = self;
	self._create($, it);
	global.screen.context.onDisplayReady();
	return self;
}
Application.prototype = application;
Application.template = template;
Object.freeze(application);
global.Application = Application;

// PiuView.c

class View @ "PiuViewDelete" {
	constructor(application, it) {
		let screen = global.screen;
		it.rotation = this.rotation;
		let poco = this.poco = new Poco(screen, it);
		this._create(application, it, screen, poco, poco.rectangle());
		if (screen.pixelFormat == Bitmap.CLUT16)
			application.clut = new Resource("main.cct");
		screen.context = this;
	}
	_create(application, it, screen, poco, rectangle) @ "PiuView_create"
	
	get rotation() @ "PiuView_get_rotation" 
	
	onDisplayReady() @ "PiuView_onDisplayReady"
	onIdle() @ "PiuView_onIdle"
	onMessage() @ "PiuView_onMessage"
	onTouchBegan(index, x, y, ticks) @ "PiuView_onTouchBegan"
	onTouchEnded(index, x, y, ticks) @ "PiuView_onTouchEnded"
	onTouchMoved(index, x, y, ticks) @ "PiuView_onTouchMoved"
}
Object.freeze(View.prototype);








