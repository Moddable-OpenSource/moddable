
import _Resource from "Resource";
globalThis.Resource = _Resource;
import parseRLE from "commodetto/parseRLE";

import {} from "piu/All";

// PiuTexture.c

export class Texture @ "PiuTextureDelete" {
	constructor(it, alphaBitmap, colorBitmap) {
		if (alphaBitmap || colorBitmap) {
			this._create(alphaBitmap, colorBitmap);
			return;
		}
		let archive, path;
		const t = typeof it;
		if ("number" === t) {
			this._create(it);
			return;
		}
		if ("object" === t) {
			archive = it.archive;
			it = it.path;
		}
		else {
			archive = null
			path = it;
		}
		if (it.endsWith(".png")) {
			let name = it.slice(0, -4);
			let path = name + "-alpha.bm4";
			if (Resource.exists(path, archive)) {
				alphaBitmap = parseRLE(new Resource(path, archive));
			}
			path = name + "-color.bm4";
			if (Resource.exists(path, archive)) {
				colorBitmap = parseRLE(new Resource(path, archive));
			}
		}
		if (alphaBitmap || colorBitmap)
			this._create(alphaBitmap, colorBitmap);
		else
			throw new URIError("Texture " + it + " not found!");
	}
	_create(alphaBitmap, colorBitmap) @ "PiuTextureCreate"
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
global.Texture = Texture;

// PiuApplication.c

var application = {
	__proto__: Container.prototype,
	_create($, it) @ "PiuApplication_create",
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
		this._create(application, it, screen);
		screen.context = this;
	}
	_create(application, it, screen) @ "PiuView_create"
	
	get rotation() @ "PiuView_get_rotation" 
	get ticks() @ "PiuView_get_ticks" 
	
	onButton(state, which) @ "PiuView_onButton"
	onDisplayReady() @ "PiuView_onDisplayReady"
	onIdle() @ "PiuView_onIdle"
	onMessage() @ "PiuView_onMessage"
	onResize(progress) @ "PiuView_onResize"
	onQuit() @ "PiuView_onQuit"
	onTouchBegan(index, x, y, ticks) @ "PiuView_onTouchBegan"
	onTouchEnded(index, x, y, ticks) @ "PiuView_onTouchEnded"
	onTouchMoved(index, x, y, ticks) @ "PiuView_onTouchMoved"
}
Object.freeze(View.prototype);

const inverter = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuInverter_create",
};
export const Inverter = Template(inverter);
Object.freeze(inverter);
globalThis.Inverter = Inverter;

const roundRect = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuRoundRect_create",
	get corners() @ "PiuRoundRect_get_corners",
	set corners(it) @ "PiuRoundRect_set_corners",
	get radius() @ "PiuRoundRect_get_radius",
	set radius(it) @ "PiuRoundRect_set_radius",
};
export const RoundRect = Template(roundRect);
RoundRect.topLeft = 1;
RoundRect.topRight = 2;
RoundRect.bottomLeft = 4;
RoundRect.bottomRight = 8;
Object.freeze(roundRect);
globalThis.RoundRect = RoundRect;

const screenBuffer = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuScreenBuffer_create",
};
export const ScreenBuffer = Template(screenBuffer);
Object.freeze(screenBuffer);
globalThis.ScreenBuffer = ScreenBuffer;

const svgImage = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuSVGImage_create",
	
	get opacity() @ "PiuSVGImage_get_opacity",
	set opacity(it) @ "PiuSVGImage_set_opacity",
	
	center(x, y) @ "PiuSVGImage_center",
	get cx() @ "PiuSVGImage_get_cx",
	set cx(it) @ "PiuSVGImage_set_cx",
	get cy() @ "PiuSVGImage_get_cy",
	set cy(it) @ "PiuSVGImage_set_cy",
	
	rotate(a) @ "PiuSVGImage_rotate",
	get r() @ "PiuSVGImage_get_r",
	set r(it) @ "PiuSVGImage_set_r",

	scale(x, y) @ "PiuSVGImage_scale",
	get s(it) @ "PiuSVGImage_get_s",
	set s(it) @ "PiuSVGImage_set_s",
	get sx() @ "PiuSVGImage_get_sx",
	set sx(it) @ "PiuSVGImage_set_sx",
	get sy() @ "PiuSVGImage_get_sy",
	set sy(it) @ "PiuSVGImage_set_sy",
	
	translate(x, y) @ "PiuSVGImage_translate",
	get tx() @ "PiuSVGImage_get_tx",
	set tx(it) @ "PiuSVGImage_set_tx",
	get ty() @ "PiuSVGImage_get_ty",
	set ty(it) @ "PiuSVGImage_set_ty",
};
export const SVGImage = Template(svgImage);
Object.freeze(svgImage);
globalThis.SVGImage = SVGImage;

import QRCodeBuffer from "qrcode";

const qrCode = {
	__proto__: Content.prototype,
	_create($, it) { return native("PiuQRCode_create").call(this, $, it); },
	QRCodeBuffer,
	get string() { return native("PiuQRCode_get_string").call(this); },
	set string(it) { native("PiuQRCode_set_string").call(this, it); },
};
export const QRCode = Template(qrCode);
Object.freeze(qrCode);
globalThis.QRCode = QRCode;
