/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

import {
	Screen,
	ScreenWorker,
} from "piu/Screen";

export class DeviceBehavior extends Behavior {
	onConfigure(container) {
	}
	onCreate(container, device) {
		this.device = device;
	}
	onLaunch(container) {
		if (container.application)
			this.onConfigure(container);
	}
	onJSON(container, json) {
	}
	onMessage(container, message) {
		trace.right(message, "sim");
		this.onJSON(container, JSON.parse(message));
	}
	postJSON(container, json) {
		const message = JSON.stringify(json);
		trace.left(message, "sim");
		this.postMessage(container, message);
	}
	postMessage(container, message) {
		model.SCREEN.postMessage(message);
	}
}

export class DeviceWorker extends ScreenWorker {
	constructor(name, screen) {
		super(name, screen);
	}
	onCreate(screen, device) {
	}
	onDelete(screen, device) {
	}
	traceInput(message, buffer, conversation) {
		if (message)
			trace.right(JSON.stringify(message), conversation);
		if (buffer)
			trace.right(buffer, conversation);
	}
	traceOutput(message, buffer, conversation) {
		if (message)
			trace.left(JSON.stringify(message), conversation);
		if (buffer)
			trace.left(buffer, conversation);
	}
}

class DeviceScreenBehavior extends Behavior {
	onAbort(screen, status) {
		screen.container.bubble("onAbort", status);
	}
	onCreate(screen, device) {
		model.SCREEN = screen;
		this.device = device;
		this.workers = [];
	}
	onDeviceSelected(screen) {
		let device = this.device;
		let Workers = device.Workers;
		let workers = this.workers;
		for (let name in Workers) {
			let worker = new Workers[name](name, screen);
			worker.onCreate(screen, device);
			workers.push(worker);
		}
	}
	onDeviceUnselected(screen) {
		let device = this.device;
		for (let worker of this.workers) {
			worker.onDelete(screen, device);
			worker.close();
		}
		this.workers = [];
	}
	onMessage(screen, message, id) {
		screen.container.bubble("onMessage", message);
	}
	onPixelFormatChanged(screen, pixelFormat) {
		application.distribute("onInfoChanged");
	}
}

export var DeviceContainer = Container.template($ => ({
	Behavior:DeviceBehavior
}));

export var DeviceScreen = Screen.template($ => ({
	Behavior:DeviceScreenBehavior,
}));
