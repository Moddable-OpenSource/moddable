/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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
import Poco from "commodetto/PocoCore";
import _Resource from "Resource";
globalThis.Resource = _Resource;

import {} from "piu/All";
export * from "piu/All";

export class CLUT extends Resource {
	get colors() { return native("PiuCLUT_get_colors").call(this); }
}
Object.freeze(CLUT.prototype);
globalThis.CLUT = CLUT;

// PiuTexture.c

export class Texture extends Native("PiuTextureDelete") {
	constructor(it, alphaBitmap, colorBitmap) {
		super();
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
			}
			path = name + "-color.bm4";
			if (Resource.exists(path, archive)) {
				colorBitmap = parseRLE(new Resource(path, archive));
			}
			else {
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
			native("PiuTexture_create").call(this, alphaBitmap, colorBitmap);
		else
			throw new URIError("Texture " + it + " not found!");
	}
	get width() { return native("PiuTexture_get_width").call(this); }
	get height() { return native("PiuTexture_get_height").call(this); }
	static template(i) {
		const it = i;
		return function() {
			let texture;
			if (globalThis.assetMap)
				texture = assetMap.get(it);
			else
				globalThis.assetMap = new Map;
			if (!texture) {
				texture = new Texture(it);
				assetMap.set(it, texture);
			}
			return texture;
		}
	}
}
Object.freeze(Texture.prototype);
globalThis.Texture = Texture;

// PiuDie.c

const die = {
	__proto__: Container.prototype,
	_create($, it) { native("PiuDie__create").call(this, $, it); },
	and(x, y, width, height) { return native("PiuDie_and").call(this, x, y, width, height); },
	attach(content) { native("PiuDie_attach").call(this, content); },
	cut() { native("PiuDie_cut").call(this); },
	detach() { native("PiuDie_detach").call(this); },
	empty() { return native("PiuDie_empty").call(this); },
	fill() { return native("PiuDie_fill").call(this); },
	or(x, y, width, height) { return native("PiuDie_or").call(this, x, y, width, height); },
	set(x, y, width, height) { return native("PiuDie_set").call(this, x, y, width, height); },
	sub(x, y, width, height) { return native("PiuDie_sub").call(this, x, y, width, height); },
	xor(x, y, width, height) { return native("PiuDie_xor").call(this, x, y, width, height); },
};
export const Die = Template(die);
Object.freeze(die);
globalThis.Die = Die;

// PiuApplication.c

const application = {
	__proto__: Container.prototype,
	_create($, it) { native("PiuApplication_create").call(this, $, it); },
	
	get clut() { return native("PiuApplication_get_clut").call(this); },
	get rotation() { return native("PiuApplication_get_rotation").call(this); },
	
	set clut(it) { native("PiuApplication_set_clut").call(this, it); },
	set rotation(it) { native("PiuApplication_set_rotation").call(this, it); },
	
	animateColors(colors) { native("PiuApplication_animateColors").call(this, colors); },
	keyDown(key) { native("PiuApplication_keyDown").call(this, key); },
	keyUp(key) { native("PiuApplication_keyUp").call(this, key); },
	postMessage(json) { native("PiuApplication_postMessage").call(this, json); },
	purge() { return native("PiuApplication_purge").call(this); },
}
export function Application($, it = {}) {
	let self = (this) ? this : Object.create(application);
	it._DeferLink = DeferLink;
	it._Skin = Skin;
	it._Style = Style;
	it._Texture = Texture;
	it._TouchLink = TouchLink;
	it._View = View;
	globalThis.application = self;
	self._create($, it);
	globalThis.screen.context.onDisplayReady();
	return self;
}
Application.prototype = application;
Application.template = template;
Object.freeze(application);
globalThis.Application = Application;

// PiuView.c

class View extends Native("PiuViewDelete") {
	constructor(application, it) {
		super();
		let screen = globalThis.screen;
		it.rotation = this.rotation;
		let poco = this.poco = new Poco(screen, it);
		this._create(application, it, screen, poco, poco.rectangle());
		if (screen.pixelFormat == Bitmap.CLUT16)
			application.clut = new Resource("main.cct");
		screen.context = this;
	}
	_create(application, it, screen, poco, rectangle) { native("PiuView_create").call(this, application, it, screen, poco, rectangle); }
	
	get rotation() { return native("PiuView_get_rotation").call(this); } 
	get ticks() { return native("PiuView_get_ticks").call(this); } 
	
	onDisplayReady() { native("PiuView_onDisplayReady").call(this); }
	onIdle() { native("PiuView_onIdle").call(this); }
	onMessage() { native("PiuView_onMessage").call(this); }
	onQuit() { native("PiuView_onQuit").call(this); }
	onTouchBegan(index, x, y, ticks) { native("PiuView_onTouchBegan").call(this, index, x, y, ticks); }
	onTouchEnded(index, x, y, ticks) { native("PiuView_onTouchEnded").call(this, index, x, y, ticks); }
	onTouchMoved(index, x, y, ticks) { native("PiuView_onTouchMoved").call(this, index, x, y, ticks); }
}
Object.freeze(View.prototype);

