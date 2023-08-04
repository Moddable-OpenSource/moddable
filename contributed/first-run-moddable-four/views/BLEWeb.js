/*
 * Copyright (c) 2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import assets from "assets";
import Timeline from "piu/Timeline";
import View from "View";

import Bitmap from "commodetto/Bitmap";
import UARTServer from "uart";

class BLEWebBehavior extends View.Behavior {
	onAuthenticated(container) {
		const view = this.view;
		view.DISCONNECTED.visible = false;
		view.PASSKEY.visible = false;
		view.CONNECTED.visible = true;
		this.event.auth = 1;
		this.transmit();
	}
	onButtonHold(container) {
		controller.goBack();
		return true;
	}
	onButtonPressed(container, delta) {
		if (this.connected) {
			controller.hold(1000);
			this.event.back = 1;
			this.transmit();
		}
		return true;
	}
	onButtonReleased(container, delta) {
		const view = this.view;
		if (this.connected) {
			if (view.PASSKEY.visible) {
				view.PASSKEY.visible = false;
				view.CONNECTED.visible = true;
			}
			this.event.back = 0;
			this.transmit();
		}
		else
			controller.goBack();
		return true;
	}
	onConnected(container) {
		const view = this.view;
		view.DISCONNECTED.visible = false;
		view.PASSKEY.visible = false;
		view.CONNECTED.visible = true;
		this.connected = true;
	}
	onCreate(container, view) {
		super.onCreate(container, view);
		this.connected = false;
		this.event = {
			auth: 0,
			back: 0,
			dial: 0,
			enter: 0,
			x: 0,
			y: 0,
			z: 0,
		};
		this.server = null;
		this.imageBuffer = new SharedArrayBuffer(32 * 128);
		this.imageBitmap = new Bitmap(128, 128, Bitmap.Monochrome, this.imageBuffer, 0);
		this.imageTexture = new Texture(null, null, this.imageBitmap);
		this.imageSkin = new Skin({ texture: this.imageTexture, width:128, height:128 });
		view.IMAGE.skin = this.imageSkin;
	}
	onDisconnected(container) {
		const view = this.view;
		view.DISCONNECTED.visible = true;
		view.PASSKEY.visible = false;
		view.CONNECTED.visible = false;
		view.IMAGE.visible = false;
		this.connected = false;
		this.event.auth = 0;
	}
	onDisplayed(container) {
		this.server = new UARTServer(container, this.imageBuffer, controller.bluetoothName);
		container.interval = 100;
		container.start();
	}
	onImage(container) {
		const image = this.view.IMAGE;
		image.visible = false;
		image.visible = true;
	}
	onJogDialPressed(container, delta) {
		if (this.connected) {
			this.event.enter = 1;
			this.transmit();
		}
		return true;
	}
	onJogDialReleased(container, delta) {
		if (this.connected) {
			this.event.enter = 0;
			this.transmit();
		}
		return true;
	}
	onJogDialTurned(container, delta) {
		if (this.connected) {
			this.event.dial = delta;
			this.transmit();
		}
	}
	onPasskey(container, passkey) {
		const view = this.view;
		view.PASSKEY.first.string = passkey;
		view.DISCONNECTED.visible = false;
		view.PASSKEY.visible = true;
		view.CONNECTED.visible = false;
	}
	onRename(container, name) {
		controller.bluetoothName = name;
		this.view.NAME.string = name;
	}
	onTimeChanged(container) {
		if (this.connected) {
			const event = this.event;
			const sample = controller.sampleAccelerometer();
			event.x = sample.x;
			event.y = sample.y;
			event.z = sample.z;
			this.transmit();
		}
	}
	onUndisplaying(container) {
		this.offset = 0;
		this.server.close();
		this.server = null;
	}
	transmit() {
		if (this.server)
			this.server.transmit(JSON.stringify(this.event));
	}
}

class BLEWebURLBehavior {
	onCreate(scroller, $) {
		scroller.interval = 40;
	}
	onDisplayed(scroller) {
		scroller.start();
	}
	onUndisplaying(scroller) {
		scroller.stop();
		scroller.scrollTo(0, 0);
	}
	onTimeChanged(scroller) {
		scroller.scrollBy(1, 0);
	}
}

const BLEWebContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:BLEWebBehavior,
	contents: [
		Container($, {
			anchor:"DISCONNECTED", left:0, right:0, top:0, bottom:0,
			contents: [
				Label($, {left:0, width:128, top:0, height:32, string:"Go To" }),
				Scroller($, {
					left:0, width:128, top:32, height:32, clip:true, looping:true, skin:assets.skins.focus, state:1, Behavior:BLEWebURLBehavior,
					contents: [
						Label($, { left:0, top:0, bottom:0, style:assets.styles.url, state:1, string:"https://www.moddable.com/four" }),
					]
				}),
				Label($, {left:0, width:128, top:64, height:32, string:"Connect to" }),
				Label($, {anchor:"NAME", left:0, width:128, top:96, height:32, skin:assets.skins.focus, state:1, string:controller.bluetoothName }),
			], 
		}),
		Container($, {
			anchor:"PASSKEY", left:0, right:0, top:0, bottom:0, visible:false,
			contents: [
				Label($, {left:0, width:128, height:64, style:assets.styles.date }),
			], 
		}),
		Container($, {
			anchor:"CONNECTED", left:0, right:0, top:0, bottom:0, visible:false,
			contents: [
				Label($, {left:0, width:128, top:0, height:32, skin:assets.skins.focus, state:1, string:"Connected" }),
				Label($, {left:0, width:128, top:64, height:32, string:"Hold button" }),
				Label($, {left:0, width:128, top:96, height:32, string:"to disconnect" }),
				Content($, { anchor:"IMAGE", left:0, right:0, top:0, bottom:0, visible:false }),
			], 
		}),
	]
}));

class BLEWebTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super();
		let container = screen.first;
		while (container) {
			if (container.visible) {
				let content = container.first;
				let offset = 0;
				while (content) {
					this.from(content, { x:screen.x + screen.width }, 200, Math.quadEaseOut, offset);
					content = content.next;
					offset = -100;
				}
				break;
			}
			container = container.next;
		}
	}
}

export default class extends View {
	static get Behavior() { return BLEWebBehavior }
	
	constructor(data) {
		super(data);
	}
	get Template() { return BLEWebContainer }
	get Timeline() { return BLEWebTimeline }
};
