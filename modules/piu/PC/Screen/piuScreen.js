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

import {
	Content,
	Template,
} from "piu/All";

export const Screen = Template(Object.freeze({
	__proto__: Content.prototype,
	_create($, it) { return native("PiuScreen_create").call(this, $, it); },
	
	get hole() { return native("PiuScreen_get_hole").call(this); },
	get pixelFormat() { return native("PiuScreen_get_pixelFormat").call(this); },
	get playingTouches() { return native("PiuScreen_get_playingTouches").call(this); },
	get recordingTouches() { return native("PiuScreen_get_recordingTouches").call(this); },
	get running() { return native("PiuScreen_get_running").call(this); },
	get transparency() { return native("PiuScreen_get_transparency").call(this); },
	
	set hole(it) { native("PiuScreen_set_hole").call(this, it); },
	set playingTouches(value) { native("PiuScreen_set_playingTouches").call(this, value); },
	set recordingTouches(value) { native("PiuScreen_set_recordingTouches").call(this, value); },
	set transparency(it) { native("PiuScreen_set_transparency").call(this, it); },
	
	launch(path) { return native("PiuScreen_launch").call(this, path); },
	postMessage(json) { return native("PiuScreen_postMessage").call(this, json); },
	quit() { return native("PiuScreen_quit").call(this); },
	touch(kind, index, x, y) { return native("PiuScreen_touch").call(this, kind, index, x, y); },
	writePNG(path) { return native("PiuScreen_writePNG").call(this, path); },
}));

export class ScreenWorker extends Native("PiuScreenWorkerDelete") {
	constructor(name, screen) { super(); native("PiuScreenWorkerCreate").call(this, name, screen); }
	close() { return native("PiuScreenWorker_close").call(this); }
	onmessage()  { debugger }
	postMessage(message) { return native("PiuScreenWorker_postMessage").call(this, message); }
}