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
} from "piu/Screen";

export class DeviceBehavior extends Behavior {
	onConfigure(container) {
	}
	onCreate(container, device) {
		this.device = device;
	}
	onMessage(container, buffer) {
	}
	postMessage(container, buffer) {
		container.first.postMessage(buffer);
	}
	traceInput(message, buffer, state = 0) {
		let messagesPane = model.MESSAGES;
		messagesPane.behavior.input(messagesPane, message, buffer, state);
	}
	traceOutput(message, buffer, state = 0) {
		let messagesPane = model.MESSAGES;
		messagesPane.behavior.output(messagesPane, message, buffer, state);
	}
}

export class DeviceWorker {
	constructor(screen, device, id) {
		this.device = device;
		this.id = id;
		this.onCreate(screen, device);
	}
	bindContent(content) {
		if (content.behavior)
			content.behavior.worker = this;
		content.active = true;
	}
	onCreate(screen, device) {
	}
	onDelete(screen, device) {
	}
	onmessage(message) {
		debugger;
	}
	postMessage(message) {
		if (this.id)
			model.SCREEN.postMessage(message, this.id);
	}
	traceInput(message, buffer, state = 0) {
		let messagesPane = model.MESSAGES;
		messagesPane.behavior.input(messagesPane, message, buffer, state);
	}
	traceOutput(message, buffer, state = 0) {
		let messagesPane = model.MESSAGES;
		messagesPane.behavior.output(messagesPane, message, buffer, state);
	}
	unbindContent(content) {
		content.active = false;
		if (content.behavior)
			delete content.behavior.worker;
	}
}

class DeviceScreenBehavior extends Behavior {
	onCreate(screen, device) {
		model.SCREEN = screen;
		this.device = device;
		this.workers = new Array(1);
	}
	onCreateWorker(screen, name) {
		let device = this.device;
		let Workers = this.device.Workers;
		if (name in Workers) {
			let workers = this.workers;
			let id = workers.length;
			let worker = new Workers[name](screen, device, id);
			workers.push(worker);
			return id;
		}
	}
	onDeleteWorker(screen, id) {
		let workers = this.workers;
		let worker = this.workers[id];
		if (worker) {
			worker.onDelete(screen, this.device);
			delete worker.id;
			delete this.workers[id];
		}
	}
	onMessage(screen, message, id) {
		if (id) {
			let worker = this.workers[id];
			if (worker)
				worker.onmessage(message);
		}
		else
			screen.container.bubble("onMessage", message);
	}
}

export var DeviceContainer = Container.template($ => ({
	Behavior:DeviceBehavior
}));

export var DeviceScreen = Screen.template($ => ({
	Behavior:DeviceScreenBehavior,
}));
