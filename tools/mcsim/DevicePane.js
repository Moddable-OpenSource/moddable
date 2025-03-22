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
	doStartPlayingTouches(screen, path) {
		this.touchBuffer = system.readFileBuffer(path);
		this.touchArray = new Uint32Array(this.touchBuffer);
		this.touchCount = this.touchArray.length;
		this.touchIndex = 0;
		screen.playingTouches = true;
		screen.time = 0;
		screen.start();
		screen.bubble("onPlayingTouches", true);
		screen.postMessage(JSON.stringify({ touches:"startPlaying" }));
	}
	doStopPlayingTouches(screen) {
		screen.stop();
		screen.playingTouches = false;
		delete this.touchIndex;
		delete this.touchCount;
		delete this.touchArray;
		delete this.touchBuffer;
		screen.bubble("onPlayingTouches", false);
		screen.postMessage(JSON.stringify({ touches:"stopPlaying" }));
	}
	doStartRecordingTouches(screen, path) {
		this.touchPath = path;
		this.touchIndex = 0;
		this.touchCount = 1024;
		this.touchBuffer = new ArrayBuffer(this.touchCount * 4, { maxByteLength: 1024 * 1024 });
		this.touchArray = new Uint32Array(this.touchBuffer);
		this.touchZero = this.touchWhen = Date.now();
		screen.recordingTouches = true;
		screen.bubble("onRecordingTouches", true);
		screen.postMessage(JSON.stringify({ touches:"startRecording" }));
	}
	doStopRecordingTouches(screen) {
		this.touchBuffer.resize(this.touchIndex * 4);
		system.writeFileBuffer(this.touchPath, this.touchBuffer);
		screen.recordingTouches = false;
		delete this.touchZero;
		delete this.touchWhen;
		delete this.touchArray;
		delete this.touchBuffer;
		delete this.touchCount;
		delete this.touchIndex;
		screen.bubble("onRecordingTouches", false);
		screen.postMessage(JSON.stringify({ touches:"stopRecording"  }));
	}
	onAbort(screen, status) {
		screen.container.bubble("onAbort", status);
	}
	onCreate(screen, device) {
		model.SCREEN = screen;
		screen.skin = skins.fingerprint;
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
	onRecordTouch(screen, kind, id, x, y) {
		const when = Date.now();
		if ((kind == 3) && (this.touchWhen >= when))
			return;
		if (this.touchIndex == this.touchCount) {
			this.touchCount += 1024;
			this.touchBuffer.resize(this.touchCount * 4);
		}
		let index = this.touchIndex;
		this.touchArray[index++] = when - this.touchZero;
		this.touchArray[index++] = kind;
		this.touchArray[index++] = x;
		this.touchArray[index++] = y;
		this.touchIndex = index;
		this.touchWhen = when;
	}
	onTimeChanged(screen) {
		if (screen.playingTouches) {
			const time = screen.time;
			const count = this.touchCount;
			let index = this.touchIndex;
			while (index < count) {
				const when = this.touchArray[index++];
				const kind = this.touchArray[index++];
				const x = this.touchArray[index++];
				const y = this.touchArray[index++];
				if (time >= when) {
					screen.touch(kind, 0, x, y);
				}
				else {
					index -= 4;
					break;
				} 
			} 
			this.touchIndex = index;
			if (index == count)
				this.doStopPlayingTouches(screen);
		}
	}
}

export var DeviceContainer = Container.template($ => ({
	Behavior:DeviceBehavior
}));

export var DeviceScreen = Screen.template($ => ({
	Behavior:DeviceScreenBehavior,
}));
