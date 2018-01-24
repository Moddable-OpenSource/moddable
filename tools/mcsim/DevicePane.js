/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

import {
	Screen,
} from "piu/Screen";

import BIN from "BinaryMessage";

export class DeviceBehavior extends Behavior {
	onConfigure(container) {
	}
	onCreate(container, device) {
		this.device = device;
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
	traceInput(buffer) {
		let messagesPane = model.MESSAGES;
		if (buffer instanceof ArrayBuffer) {
			let device = this.device;
			let message = BIN.parse(device.format, buffer, device.endian);
			messagesPane.behavior.input(messagesPane, message, buffer);
		}
		else
			messagesPane.behavior.input(messagesPane, buffer);
	}
	traceOutput(buffer) {
		let messagesPane = model.MESSAGES;
		if (buffer instanceof ArrayBuffer) {
			let device = this.device;
			let message = BIN.parse(device.format, buffer, device.endian);
			messagesPane.behavior.output(messagesPane, message, buffer);
		}
		else
			messagesPane.behavior.output(messagesPane, buffer);
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
	}
}

export var DeviceContainer = Container.template($ => ({
	Behavior:DeviceBehavior
}));

export var DeviceScreen = Screen.template($ => ({
	Behavior:DeviceScreenBehavior,
}));
