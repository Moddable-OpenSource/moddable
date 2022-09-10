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

import {} from "piu/All";

global.__jsx__ = function(Tag, attributes) @ "Piu__jsx__"

export class Component extends Behavior {
	constructor($, it) {
		super();
		let result = this.render($, it);
		let anchor = it.anchor;
		if ($ && anchor)
			$[anchor] = result;
		result.behavior = this;
		let onCreate = this.onCreate;
		if (onCreate)
			onCreate.call(this, result, $, it);
		return result;
	}
	render($, it) {
		debugger;
	}
}
Object.freeze(Component.prototype);
global.Component = Component;

export class TextComponent {
	constructor($, it) {
		this.spans = it.contents;
		this.style = it.style;
	}
}
Object.freeze(TextComponent.prototype);
global.TextComponent = TextComponent;

export class LinkComponent extends TextComponent {
	constructor($, it) {
		super($, it);
		this.link = new Link($, { behavior:this });
	}
}
Object.freeze(LinkComponent.prototype);
global.LinkComponent = LinkComponent;

// PiuTexture.c

export class Texture @ "PiuTextureDelete" {
	constructor(it) @ "PiuTextureCreate"
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

// PiuRoundContent.c

var roundContent = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuRoundContent_create",

	get radius() @ "PiuRoundContent_get_radius",
	
	set radius(it) @ "PiuRoundContent_set_radius",
};
export var RoundContent = Template(roundContent);
Object.freeze(roundContent);
global.RoundContent = RoundContent;

// PiuField.c

var field = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuField_create",

	get placeholder() @ "PiuField_get_placeholder",
	get string() @ "PiuField_get_string",
	
	set placeholder(it) @ "PiuField_set_placeholder",
	set string(it) @ "PiuField_set_string",
	
	focus() @ "PiuField_focus",
};
export var Field = Template(field);
Object.freeze(field);
global.Field = Field;

// PiuApplication.c

var application = {
	__proto__: Container.prototype,
	_create($, it) @ "PiuApplication_create",
	get cursor() @ "PiuApplication_get_cursor",
	get title() @ "PiuApplication_get_title",
	set cursor(it) @ "PiuApplication_set_cursor",
	set title(it) @ "PiuApplication_set_title",
	createMenus() @ "PiuApplication_createMenus",
	gotoFront() @ "PiuApplication_gotoFront",
	invalidateMenus() @ "PiuApplication_invalidateMenus",
	purge() @ "PiuApplication_purge",
	quit() @ "PiuApplication_quit",
	updateMenus() @ "PiuApplication_updateMenus",
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
	global.controlKey = false;
	global.optionKey = false;
	global.shiftKey = false;
	self._create($, it);
	return self;
}
Application.prototype = application;
Application.template = template;
Object.freeze(application);
global.Application = Application;

// PiuView.c

class View @ "PiuViewDelete" {
	constructor(it) @ "PiuViewCreate"
}
Object.freeze(View.prototype);

// PiuService.c

export class Service @ "ServiceProxyDelete" {
	constructor(thread, module) @ "ServiceProxyCreate"
	get(target, key) {
		let handler = this;
		return function(...params) {
			return new Promise(function(resolve, reject) {
				target.call(handler, key, params, resolve, reject)
			});
		}
	}
};
global.Service = Service;


global.cursors = {
	get arrow() @ "PiuCursors_get_arrow",
	get cross() @ "PiuCursors_get_cross",
	get iBeam() @ "PiuCursors_get_iBeam",
	get link() @ "PiuCursors_get_link",
	get notAllowed() @ "PiuCursors_get_notAllowed",
	get resizeColumn() @ "PiuCursors_get_resizeColumn",
	get resizeRow() @ "PiuCursors_get_resizeRow",
};
Object.freeze(global.cursors);

global.system = {
	get applicationPath() @ "PiuSystem_get_applicationPath",
	get localDirectory() @ "PiuSystem_get_localDirectory",
	get platform() @ "PiuSystem_get_platform",

	launchPath(url) @ "PiuSystem_launchPath",
	launchURL(url) @ "PiuSystem_launchURL",
	
	// clipboard
	getClipboardString() @ "PiuSystem_getClipboardString",
	setClipboardString(it) @ "PiuSystem_setClipboardString",

	// dialogs
	alert(dictionary, callback) @ "PiuSystem_alert",
	openDirectory(dictionary, callback) @ "PiuSystem_openDirectory",
	openFile(dictionary, callback) @ "PiuSystem_openFile",
	saveDirectory(dictionary, callback) @ "PiuSystem_saveDirectory",
	saveFile(dictionary, callback) @ "PiuSystem_saveFile",

	// files
	copyFile(from, to) @ "PiuSystem_copyFile",
	deleteDirectory(path) @ "PiuSystem_deleteDirectory",
	deleteFile(path) @ "PiuSystem_deleteFile",
	ensureDirectory(path) @ "PiuSystem_ensureDirectory",
	
	fileExists(path) @ "PiuSystem_fileExists",
	getFileInfo(path) @ "PiuSystem_getFileInfo",
	getSymbolicLinkInfo(path) @ "PiuSystem_getSymbolicLinkInfo",
	
	readFileBuffer(path) @ "PiuSystem_readFileBuffer",
	readFileString(path) @ "PiuSystem_readFileString",
	readPreferenceString(key) @ "PiuSystem_readPreferenceString",
	
	renameDirectory(path, name) @ "PiuSystem_renameDirectory",
	renameFile(path, name) @ "PiuSystem_renameFile",
	
	writeFileBuffer(path, data) @ "PiuSystem_writeFileBuffer",
	writeFileString(path, data) @ "PiuSystem_writeFileString",
	writePreferenceString(key, data) @ "PiuSystem_writePreferenceString",
	
	DirectoryIterator: class @ "PiuSystem_DirectoryIteratorDelete" {
		constructor(path)  @ "PiuSystem_DirectoryIteratorCreate"
		next() @ "PiuSystem_DirectoryIterator_next"
	},
	DirectoryNotifier: class @ "PiuSystem_DirectoryNotifierDelete" {
		constructor(path, callback)  @ "PiuSystem_DirectoryNotifierCreate"
		close() @ "PiuSystem_DirectoryNotifier_close"
	},
	
	// paths
	buildPath(directory, name, extension) @ "PiuSystem_buildPath",
	getPathDirectory(path) @ "PiuSystem_getPathDirectory",
	getPathExtension(path) @ "PiuSystem_getPathExtension",
	getPathName(path) @ "PiuSystem_getPathName",
}
Object.freeze(global.system);




