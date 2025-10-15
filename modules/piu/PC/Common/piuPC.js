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

import {} from "piu/All";

globalThis.__jsx__ = function(Tag, attributes) { return native("Piu__jsx__").call(this, Tag, attributes); }

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
globalThis.Component = Component;

export class TextComponent {
	constructor($, it) {
		this.spans = it.contents;
		this.style = it.style;
	}
}
Object.freeze(TextComponent.prototype);
globalThis.TextComponent = TextComponent;

export class LinkComponent extends TextComponent {
	constructor($, it) {
		super($, it);
		this.link = new Link($, { behavior:this });
	}
}
Object.freeze(LinkComponent.prototype);
globalThis.LinkComponent = LinkComponent;

// PiuTexture.c

export class Texture extends Native("PiuTextureDelete") {
	constructor(it) { super(); native("PiuTextureCreate").call(this, it); }
	get width() { return native("PiuTexture_get_width").call(this); }
	get height() { return native("PiuTexture_get_height").call(this); }
	static template(it) {
		return function() {
			let map = globalThis.assetMap;
			let texture;
			if (map)
				texture = map.get(it);
			else
				map = globalThis.assetMap = new Map;
			if (!texture) {
				texture = new Texture(it);
				map.set(it, texture);
			}
			return texture;
		}
	}
}
Object.freeze(Texture.prototype);
globalThis.Texture = Texture;

// PiuRoundContent.c

var roundContent = {
	__proto__: Content.prototype,
	_create($, it) { return native("PiuRoundContent_create").call(this, $, it); },

	get radius() { return native("PiuRoundContent_get_radius").call(this); },
	
	set radius(it) { native("PiuRoundContent_set_radius").call(this, it); },
};
export var RoundContent = Template(roundContent);
Object.freeze(roundContent);
globalThis.RoundContent = RoundContent;

// PiuField.c

var field = {
	__proto__: Content.prototype,
	_create($, it) { return native("PiuField_create").call(this, $, it); },

	get placeholder() { return native("PiuField_get_placeholder").call(this); },
	get string() { return native("PiuField_get_string").call(this); },
	
	set placeholder(it) { native("PiuField_set_placeholder").call(this, it); },
	set string(it) { native("PiuField_set_string").call(this, it); },
	
	focus() { return native("PiuField_focus").call(this); },
};
export var Field = Template(field);
Object.freeze(field);
globalThis.Field = Field;

// PiuApplication.c

var application = {
	__proto__: Container.prototype,
	_create($, it) { return native("PiuApplication_create").call(this, $, it); },
	get cursor() { return native("PiuApplication_get_cursor").call(this); },
	get title() { return native("PiuApplication_get_title").call(this); },
	set cursor(it) { native("PiuApplication_set_cursor").call(this, it); },
	set title(it) { native("PiuApplication_set_title").call(this, it); },
	createMenus() { return native("PiuApplication_createMenus").call(this); },
	gotoFront() { return native("PiuApplication_gotoFront").call(this); },
	invalidateMenus() { return native("PiuApplication_invalidateMenus").call(this); },
	purge() { return native("PiuApplication_purge").call(this); },
	quit() { return native("PiuApplication_quit").call(this); },
	updateMenus() { return native("PiuApplication_updateMenus").call(this); },
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
	globalThis.controlKey = false;
	globalThis.optionKey = false;
	globalThis.shiftKey = false;
	self._create($, it);
	return self;
}
Application.prototype = application;
Application.template = template;
Object.freeze(application);
globalThis.Application = Application;

// PiuView.c

class View extends Native("PiuViewDelete") {
	constructor(it) { super(); native("PiuViewCreate").call(this, it); }
}
Object.freeze(View.prototype);

// PiuService.c

export class Service extends Native("ServiceProxyDelete") {
	constructor(thread, module) { super(); native("ServiceProxyCreate").call(this, thread, module); }
	get(target, key) {
		let handler = this;
		return function(...params) {
			return new Promise(function(resolve, reject) {
				target.call(handler, key, params, resolve, reject)
			});
		}
	}
};
globalThis.Service = Service;

globalThis.cursors = {
	get arrow() { return native("PiuCursors_get_arrow").call(this); },
	get cross() { return native("PiuCursors_get_cross").call(this); },
	get iBeam() { return native("PiuCursors_get_iBeam").call(this); },
	get link() { return native("PiuCursors_get_link").call(this); },
	get notAllowed() { return native("PiuCursors_get_notAllowed").call(this); },
	get resizeColumn() { return native("PiuCursors_get_resizeColumn").call(this); },
	get resizeRow() { return native("PiuCursors_get_resizeRow").call(this); },
};
Object.freeze(globalThis.cursors);

globalThis.system = {
	get applicationPath() { return native("PiuSystem_get_applicationPath").call(this); },
	get localDirectory() { return native("PiuSystem_get_localDirectory").call(this); },
	get platform() { return native("PiuSystem_get_platform").call(this); },

	launchPath(url) { return native("PiuSystem_launchPath").call(this, url); },
	launchURL(url) { return native("PiuSystem_launchURL").call(this, url); },
	
	// clipboard
	getClipboardString() { return native("PiuSystem_getClipboardString").call(this); },
	setClipboardString(it) { return native("PiuSystem_setClipboardString").call(this, it); },

	// dialogs
	alert(dictionary, callback) { return native("PiuSystem_alert").call(this, dictionary, callback); },
	openDirectory(dictionary, callback) { return native("PiuSystem_openDirectory").call(this, dictionary, callback); },
	openFile(dictionary, callback) { return native("PiuSystem_openFile").call(this, dictionary, callback); },
	saveDirectory(dictionary, callback) { return native("PiuSystem_saveDirectory").call(this, dictionary, callback); },
	saveFile(dictionary, callback) { return native("PiuSystem_saveFile").call(this, dictionary, callback); },

	// files
	copyFile(from, to) { return native("PiuSystem_copyFile").call(this, from, to); },
	deleteDirectory(path) { return native("PiuSystem_deleteDirectory").call(this, path); },
	deleteFile(path) { return native("PiuSystem_deleteFile").call(this, path); },
	ensureDirectory(path) { return native("PiuSystem_ensureDirectory").call(this, path); },
	
	fileExists(path) { return native("PiuSystem_fileExists").call(this, path); },
	getFileInfo(path) { return native("PiuSystem_getFileInfo").call(this, path); },
	getSymbolicLinkInfo(path) { return native("PiuSystem_getSymbolicLinkInfo").call(this, path); },
	
	readFileBuffer(path) { return native("PiuSystem_readFileBuffer").call(this, path); },
	readFileString(path) { return native("PiuSystem_readFileString").call(this, path); },
	readPreferenceString(key) { return native("PiuSystem_readPreferenceString").call(this, key); },
	
	renameDirectory(path, name) { return native("PiuSystem_renameDirectory").call(this, path, name); },
	renameFile(path, name) { return native("PiuSystem_renameFile").call(this, path, name); },
	
	writeFileBuffer(path, data) { return native("PiuSystem_writeFileBuffer").call(this, path, data); },
	writeFileString(path, data) { return native("PiuSystem_writeFileString").call(this, path, data); },
	writePreferenceString(key, data) { return native("PiuSystem_writePreferenceString").call(this, key, data); },
	
	DirectoryIterator: class extends Native("PiuSystem_DirectoryIteratorDelete") {
		constructor(path) { super(); native("PiuSystem_DirectoryIteratorCreate").call(this, path); }
		next() { return native("PiuSystem_DirectoryIterator_next").call(this); }
	},
	DirectoryNotifier: class extends Native("PiuSystem_DirectoryNotifierDelete") {
		constructor(path, callback) { super(); native("PiuSystem_DirectoryNotifierCreate").call(this, path, callback); }
		close() { return native("PiuSystem_DirectoryNotifier_close").call(this); }
	},
	
	// paths
	buildPath(directory, name, extension) { return native("PiuSystem_buildPath").call(this, directory, name, extension); },
	getPathDirectory(path) { return native("PiuSystem_getPathDirectory").call(this, path); },
	getPathExtension(path) { return native("PiuSystem_getPathExtension").call(this, path); },
	getPathName(path) { return native("PiuSystem_getPathName").call(this, path); },
}
Object.freeze(globalThis.system);

