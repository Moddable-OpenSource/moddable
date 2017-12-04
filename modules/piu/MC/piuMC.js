/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
import parseBMP from "commodetto/ParseBMP";
import parseRLE from "commodetto/ParseRLE";
import Poco from "commodetto/Poco";
import Resource from "Resource";

import { 
	blendColors,
	hsl,
	hsla,
	rgb,
	rgba,
	Skin,
	Style,
	Behavior,
	Transition,
	template,
	Template,
	Content,
	Label,
	Link,
	Text,
	Port,
	Container,
	Column,
	Layout,
	Row,
	Scroller,
	DeferLink,
	TouchLink,
} from "All";
export * from "All";

export class CLUT extends Resource {
	constructor(path) {
		super(path);
	}
	get colors() @ "PiuCLUT_get_colors"
}
Object.freeze(CLUT.prototype);

// PiuTexture.c

export class Texture @ "PiuTextureDelete" {
	constructor(it) {
		let path;
		let alphaBitmap;
		let colorBitmap;
		if (typeof(it) == "object")
			it = it.path;
		if (it.endsWith(".png")) {
			let name = it.slice(0, -4);
			path = name + "-alpha.bm4";
			if (Resource.exists(path)) {
				alphaBitmap = parseRLE(new Resource(path));
			}
			else {
				path = name + "-alpha.bmp";
				if (Resource.exists(path)) {
					alphaBitmap = parseBMP(new Resource(path));
				}
				path = name + "-color.bmp";
				if (Resource.exists(path)) {
					colorBitmap = parseBMP(new Resource(path));
				}
			}
		}
		else if (it.endsWith("-alpha.bmp"))
			alphaBitmap = parseBMP(new Resource(it));
		else if (it.endsWith("-color.bmp"))
			colorBitmap = parseBMP(new Resource(it));
		if (alphaBitmap || colorBitmap)
			this._create(alphaBitmap, colorBitmap);
		else
			throw new URIError("Texture " + it + " not found!");
	}
	_create(alphaBitmap, colorBitmap) @ "PiuTexture_create"
	get width() @ "PiuTexture_get_width"
	get height() @ "PiuTexture_get_height"
	static template(it) {
		return function() {
			let map = global.assetMap;
			let texture;
			if (map)
				texture = map.get(it);
			else
				map = global.assetMap = new Map;
			if (!texture) {
				texture = new Texture(it);
				map.set(it, texture);
			}
			return texture;
		}
	}
}
Object.freeze(Texture.prototype);

// PiuDie.c

var die = {
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
export var Die = Template(die);
Object.freeze(die);

// PiuImage.c

var image = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuImage_create",
	
	get frameCount() @ "PiuImage_get_frameCount",
	get frameIndex() @ "PiuImage_get_frameIndex",
	
	set frameIndex(it) @ "PiuImage_set_frameIndex",
};
export var Image = Template(image);
Object.freeze(image);

// PiuApplication.c

var application = {
	__proto__: Container.prototype,
	_create($, it) @ "PiuApplication_create",
	
	get clut() @ "PiuApplication_get_clut",
	
	set clut(it) @ "PiuApplication_set_clut",
	
	animateColors(colors) @ "PiuApplication_animateColors",
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
	return self;
}
Application.prototype = application;
Application.template = template;
Object.freeze(application);

// PiuView.c

class View @ "PiuViewDelete" {
	constructor(application, it) {
		let screen = global.screen ? global.screen : require("screen");
		it.rotation = this.rotation;
		if (screen.pixelFormat == Bitmap.CLUT16)
			screen.clut = new Resource("main.cct");
		let poco = new Poco(screen, it);
		this._create(application, it, screen, poco, poco.rectangle());
		screen.context = this;
		screen.start(5);
	}
	_create(application, it, screen, poco, rectangle) @ "PiuView_create"
	
	get rotation() @ "PiuView_get_rotation" 
	
	onIdle() @ "PiuView_onIdle"
	onMessage() @ "PiuView_onMessage"
	onTouchBegan(index, x, y, ticks) @ "PiuView_onTouchBegan"
	onTouchEnded(index, x, y, ticks) @ "PiuView_onTouchEnded"
	onTouchMoved(index, x, y, ticks) @ "PiuView_onTouchMoved"
}
Object.freeze(View.prototype);

global.blendColors = blendColors;
global.hsl = hsl;
global.hsla = hsla;
global.rgb = rgb;
global.rgba = rgba;

global.Texture = Texture;
global.Skin = Skin;
global.Style = Style;
global.Behavior = Behavior;
global.Transition = Transition;

global.Content = Content;
global.Image = Image;
global.Label = Label;
global.Port = Port;
global.Text = Text;
global.Link = Link;

global.Container = Container;
global.Column = Column;
global.Layout = Layout;
global.Row = Row;
global.Scroller = Scroller;

global.Die = Die;
global.Application = Application;

class Locals  @ "PiuLocalsDelete" {
	constructor(name, language) @ "PiuLocalsCreate"
	get language() @ "PiuLocals_get_language"
	set language(it) @ "PiuLocals_set_language"
	get(id) @ "PiuLocals_get"
}
Object.freeze(Locals.prototype);
global.Locals = Locals;

var 
c_send,
commandListLength,
context,
regionLength,
send,
;
