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

import { 
	blendColors,
	hsl,
	hsla,
	rgb,
	rgba,
	Skin,
	Style,
	Behavior,
	Component,
	Transition,
	template,
	Template,
	Content,
	Label,
	Text,
	TextComponent,
	Link,
	LinkComponent,
	Port,
	Container,
	Column,
	Layout,
	Row,
	Scroller,
	DeferLink,
	TouchLink,
} from "All";

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
	self._create($, it);
	return self;
}
Application.prototype = application;
Application.template = template;
Object.freeze(application);
global.application = null;

// PiuView.c

class View @ "PiuViewDelete" {
	constructor(it) @ "PiuViewCreate"
}
Object.freeze(View.prototype);

var statusBar = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuStatusBar_create",
};
export var StatusBar = Template(statusBar);
Object.freeze(statusBar);

var navigationBar = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuNavigationBar_create",
};
export var NavigationBar = Template(navigationBar);
Object.freeze(navigationBar);

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

global.controlKey = false;
global.optionKey = false;
global.shiftKey = false;

global.cursors = {
	get arrow() @ "PiuCursors_get_arrow",
	get cross() @ "PiuCursors_get_cross",
	get iBeam() @ "PiuCursors_get_iBeam",
	get link() @ "PiuCursors_get_link",
	get notAllowed() @ "PiuCursors_get_notAllowed",
	get resizeColumn() @ "PiuCursors_get_resizeColumn",
	get resizeRow() @ "PiuCursors_get_resizeRow",
};

global.system = {
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
	deleteDirectory(path) @ "PiuSystem_deleteDirectory",
	deleteFile(path) @ "PiuSystem_deleteFile",
	ensureDirectory(path) @ "PiuSystem_ensureDirectory",
	
	fileExists(path) @ "PiuSystem_fileExists",
	getFileInfo(path) @ "PiuSystem_getFileInfo",
	
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
	getPathDirectory(path) @ "PiuSystem_getPathDirectory",
	getPathExtension(path) @ "PiuSystem_getPathExtension",
	getPathName(path) @ "PiuSystem_getPathName",
}

global.blendColors = blendColors;
global.hsl = hsl;
global.hsla = hsla;
global.rgb = rgb;
global.rgba = rgba;

global.Texture = Texture;
global.Skin = Skin;
global.Style = Style;
global.Behavior = Behavior;
global.Component = Component;
global.Transition = Transition;

global.Content = Content;
global.Label = Label;
global.Port = Port;
global.Text = Text;
global.TextComponent = TextComponent;
global.Link = Link;
global.LinkComponent = LinkComponent;

global.Container = Container;
global.Column = Column;
global.Layout = Layout;
global.Row = Row;
global.Scroller = Scroller;


global.Texture = Texture;

global.Field = Field;

global.Application = Application;
global.StatusBar = StatusBar;
global.NavigationBar = NavigationBar;

global.Service = Service;
