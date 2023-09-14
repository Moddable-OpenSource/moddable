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
import Menu from "Menu";

class SetTimeBehavior extends View.Behavior {
	changeFocus(view, anchor) {
		let former = view[this.focus];
		let current = view[anchor];
		former.state = 0;
		former.first.state = 0;
		former.last.state = 0;
		current.state = 1;
		current.first.state = 1;
		current.last.state = 1;
		this.focus = anchor;
	}
	onCreate(container, view) {
		super.onCreate(container, view);
		container.duration = 250;
		this.focus = "HOUR";
	}
	onDisplaying(container) {
		const view = this.view;
		this.changeFocus(view, "HOUR");
	}
	onFinished(container) {
		const focus = this.view[this.focus];
		focus.swap(focus.first, focus.last);
		controller.onJogDialTurned(application, 0);
	}
	onJogDialReleased(container) {
		const view = this.view;
		switch (this.focus) {
		case "HOUR":
			this.changeFocus(view, "MINUTE");
			view.UP.x = view.DOWN.x = container.x + container.width - 11;
			break;
		case "MINUTE":
			this.changeFocus(view, "HOUR");
			view.UP.x = view.DOWN.x = container.x + 2;
			break;
		}
		return true;
	}
	onJogDialTurned(container, delta) {
		if (container.running)
			return false;
		const view = this.view;
		delta >>= 2;
		switch (this.focus) {
		case "HOUR":
			let hour = view.hour + delta;
			hour %= 24;
			if (hour < 0)
				hour += 24
			view.hour = hour;
			view.HOUR.last.string = hour;
			break;
		case "MINUTE":
			let minute = view.minute + delta;
			minute %= 60;
			if (minute < 0)
				minute += 60
			view.minute = minute;
			view.MINUTE.last.string = String(minute).padStart(2, '0');
			break;
		}
		this.delta = delta;
		container.time = 0;
		container.start();
		return true;
	}
	onTimeChanged(container) {
		const fraction = container.fraction;
		const focus = this.view[this.focus];
		const current = focus.last;
		const former = focus.first;
		if (this.delta > 0) {
			former.y = focus.y - (focus.height * fraction);
			current.y = focus.y + (focus.height * (1 - fraction));
		}
		else {
			former.y = focus.y + (focus.height * fraction);
			current.y = focus.y - (focus.height * (1 - fraction));
		}
	}
	onUndisplaying(container) {
		const view = this.view;
		const date = new Date();
		date.setHours(view.hour, view.minute, 0);
		application.delegate("setTime", Math.floor(date.valueOf() / 1000));
	}
}

const SetTimeContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:SetTimeBehavior,
	contents: [
		Container($, { 
			anchor:"HOUR", left:0, width:58, top:24, height:80, skin:assets.skins.focus, style:assets.styles.hour, clip:true,
			contents: [
				Label($, { left:0, right:0, top:0, height:80, string:$.hour }),
				Label($, { left:0, right:0, top:80, height:80 }),
			],
		}),
		Label($, { left:0, width:128, top:24, height:72, style:assets.styles.time, string:":" }),
		Container($, { 
			anchor:"MINUTE", right:0, width:58, top:24, height:80, skin:assets.skins.focus, style:assets.styles.minute, clip:true,
			contents: [
				Label($, { left:0, right:0, top:0, height:80, string:($.minute < 10) ? "0" + $.minute : $.minute }),
				Label($, { left:0, right:0, top:80, height:80 }),
			],
		}),
		Content($, { anchor:"UP", bottom:93, left:2, skin:assets.skins.arrows, state:1 }),
		Content($, { anchor:"DOWN", top:93, left:2, skin:assets.skins.arrows, state:1, variant:1 }),
	]
}));

class SetTimeTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super();
		const hour = screen.first;
		const colon = hour.next;
		const minute = colon.next;
		if (controller.going != direction) {
			this.from(minute, { x:screen.x - minute.width }, 200, Math.quadEaseOut, 0);
			this.from(colon, { x:screen.x - colon.width }, 200, Math.quadEaseOut, -100);
			this.from(hour, { x:screen.x - hour.width }, 200, Math.quadEaseOut, -100);
		}
		else {
			this.from(hour, { x:screen.x + screen.width }, 200, Math.quadEaseOut, 0);
			this.from(colon, { x:screen.x + screen.width }, 200, Math.quadEaseOut, -100);
			this.from(minute, { x:screen.x + screen.width }, 200, Math.quadEaseOut, -100);
		}
	}
}

export default class extends View {
	static get Behavior() { return SetTimeBehavior }
	
	constructor(data) {
		super(data);
		const date = new Date();
		this.hour = date.getHours();
		this.minute = date.getMinutes();
	}
	get Template() { return SetTimeContainer }
	get Timeline() { return SetTimeTimeline }
};

class SetTimeMenuItemBehavior {
	onCreate(port, $) {
		this.color = "white";
		this.hours = 0;
		this.hoursData = [ { "x": 7, "y": 28, "sw": 13, "sh": 36, "sx": 93, "sy": 38 }, { "x": 7, "y": 29, "sw": 16, "sh": 38, "sx": 392, "sy": 0 }, { "x": 8, "y": 29, "sw": 20, "sh": 38, "sx": 312, "sy": 0 }, { "x": 9, "y": 29, "sw": 23, "sh": 38, "sx": 220, "sy": 0 }, { "x": 9, "y": 29, "sw": 26, "sh": 38, "sx": 116, "sy": 0 }, { "x": 10, "y": 28, "sw": 29, "sh": 38, "sx": 0, "sy": 0 }, { "x": 10, "y": 27, "sw": 31, "sh": 37, "sx": 456, "sy": 0 }, { "x": 10, "y": 25, "sw": 34, "sh": 35, "sx": 118, "sy": 38 }, { "x": 10, "y": 24, "sw": 35, "sh": 34, "sx": 254, "sy": 38 }, { "x": 10, "y": 22, "sw": 37, "sh": 31, "sx": 394, "sy": 38 }, { "x": 10, "y": 20, "sw": 38, "sh": 29, "sx": 37, "sy": 75 }, { "x": 10, "y": 17, "sw": 38, "sh": 26, "sx": 189, "sy": 75 }, { "x": 10, "y": 15, "sw": 38, "sh": 23, "sx": 341, "sy": 75 }, { "x": 10, "y": 12, "sw": 38, "sh": 20, "sx": 0, "sy": 106 }, { "x": 9, "y": 9, "sw": 38, "sh": 16, "sx": 152, "sy": 106 }, { "x": 9, "y": 7, "sw": 36, "sh": 13, "sx": 304, "sy": 106 }, { "x": 9, "y": 7, "sw": 38, "sh": 16, "sx": 190, "sy": 106 }, { "x": 10, "y": 8, "sw": 38, "sh": 20, "sx": 38, "sy": 106 }, { "x": 10, "y": 9, "sw": 38, "sh": 23, "sx": 379, "sy": 75 }, { "x": 10, "y": 9, "sw": 38, "sh": 26, "sx": 227, "sy": 75 }, { "x": 10, "y": 10, "sw": 38, "sh": 29, "sx": 75, "sy": 75 }, { "x": 10, "y": 10, "sw": 37, "sh": 31, "sx": 431, "sy": 38 }, { "x": 10, "y": 10, "sw": 35, "sh": 34, "sx": 289, "sy": 38 }, { "x": 10, "y": 10, "sw": 34, "sh": 35, "sx": 152, "sy": 38 }, { "x": 10, "y": 10, "sw": 31, "sh": 37, "sx": 0, "sy": 38 }, { "x": 10, "y": 10, "sw": 29, "sh": 38, "sx": 29, "sy": 0 }, { "x": 9, "y": 10, "sw": 26, "sh": 38, "sx": 142, "sy": 0 }, { "x": 9, "y": 10, "sw": 23, "sh": 38, "sx": 243, "sy": 0 }, { "x": 8, "y": 10, "sw": 20, "sh": 38, "sx": 332, "sy": 0 }, { "x": 7, "y": 9, "sw": 16, "sh": 38, "sx": 408, "sy": 0 }, { "x": 6, "y": 8, "sw": 12, "sh": 36, "sx": 106, "sy": 38 }, { "x": 9, "y": 9, "sw": 16, "sh": 38, "sx": 424, "sy": 0 }, { "x": 12, "y": 10, "sw": 20, "sh": 38, "sx": 352, "sy": 0 }, { "x": 15, "y": 10, "sw": 23, "sh": 38, "sx": 266, "sy": 0 }, { "x": 17, "y": 10, "sw": 26, "sh": 38, "sx": 168, "sy": 0 }, { "x": 20, "y": 10, "sw": 29, "sh": 38, "sx": 58, "sy": 0 }, { "x": 22, "y": 10, "sw": 31, "sh": 37, "sx": 31, "sy": 38 }, { "x": 24, "y": 10, "sw": 34, "sh": 35, "sx": 186, "sy": 38 }, { "x": 25, "y": 10, "sw": 35, "sh": 34, "sx": 324, "sy": 38 }, { "x": 27, "y": 10, "sw": 37, "sh": 31, "sx": 468, "sy": 38 }, { "x": 28, "y": 10, "sw": 38, "sh": 29, "sx": 113, "sy": 75 }, { "x": 29, "y": 9, "sw": 38, "sh": 26, "sx": 265, "sy": 75 }, { "x": 29, "y": 9, "sw": 38, "sh": 23, "sx": 417, "sy": 75 }, { "x": 29, "y": 8, "sw": 38, "sh": 20, "sx": 76, "sy": 106 }, { "x": 29, "y": 7, "sw": 38, "sh": 16, "sx": 228, "sy": 106 }, { "x": 28, "y": 7, "sw": 36, "sh": 13, "sx": 340, "sy": 106 }, { "x": 29, "y": 9, "sw": 38, "sh": 16, "sx": 266, "sy": 106 }, { "x": 29, "y": 12, "sw": 38, "sh": 20, "sx": 114, "sy": 106 }, { "x": 29, "y": 15, "sw": 38, "sh": 23, "sx": 455, "sy": 75 }, { "x": 29, "y": 17, "sw": 38, "sh": 26, "sx": 303, "sy": 75 }, { "x": 28, "y": 20, "sw": 38, "sh": 29, "sx": 151, "sy": 75 }, { "x": 27, "y": 22, "sw": 37, "sh": 31, "sx": 0, "sy": 75 }, { "x": 25, "y": 24, "sw": 35, "sh": 34, "sx": 359, "sy": 38 }, { "x": 24, "y": 25, "sw": 34, "sh": 35, "sx": 220, "sy": 38 }, { "x": 22, "y": 27, "sw": 31, "sh": 37, "sx": 62, "sy": 38 }, { "x": 20, "y": 28, "sw": 29, "sh": 38, "sx": 87, "sy": 0 }, { "x": 17, "y": 29, "sw": 26, "sh": 38, "sx": 194, "sy": 0 }, { "x": 15, "y": 29, "sw": 23, "sh": 38, "sx": 289, "sy": 0 }, { "x": 12, "y": 29, "sw": 20, "sh": 38, "sx": 372, "sy": 0 }, { "x": 9, "y": 29, "sw": 16, "sh": 38, "sx": 440, "sy": 0 } ];
		this.hoursTexture = new Texture({ path: "hours.png" });
		this.minutes = 0;
		this.minutesData = [ { "x": 6, "y": 38, "sw": 11, "sh": 46, "sx": 348, "sy": 0 }, { "x": 6, "y": 39, "sw": 15, "sh": 47, "sx": 288, "sy": 0 }, { "x": 7, "y": 39, "sw": 20, "sh": 48, "sx": 0, "sy": 0 }, { "x": 8, "y": 38, "sw": 24, "sh": 47, "sx": 192, "sy": 0 }, { "x": 8, "y": 37, "sw": 28, "sh": 47, "sx": 80, "sy": 0 }, { "x": 9, "y": 36, "sw": 32, "sh": 45, "sx": 369, "sy": 0 }, { "x": 9, "y": 34, "sw": 36, "sh": 44, "sx": 0, "sy": 48 }, { "x": 10, "y": 32, "sw": 39, "sh": 41, "sx": 144, "sy": 48 }, { "x": 10, "y": 30, "sw": 41, "sh": 39, "sx": 300, "sy": 48 }, { "x": 10, "y": 27, "sw": 44, "sh": 36, "sx": 464, "sy": 48 }, { "x": 10, "y": 24, "sw": 45, "sh": 32, "sx": 132, "sy": 92 }, { "x": 10, "y": 21, "sw": 47, "sh": 28, "sx": 312, "sy": 92 }, { "x": 10, "y": 17, "sw": 47, "sh": 24, "sx": 0, "sy": 128 }, { "x": 9, "y": 13, "sw": 48, "sh": 20, "sx": 188, "sy": 128 }, { "x": 9, "y": 9, "sw": 47, "sh": 15, "sx": 380, "sy": 128 }, { "x": 9, "y": 6, "sw": 46, "sh": 11, "sx": 94, "sy": 152 }, { "x": 9, "y": 6, "sw": 47, "sh": 15, "sx": 427, "sy": 128 }, { "x": 9, "y": 7, "sw": 48, "sh": 20, "sx": 236, "sy": 128 }, { "x": 10, "y": 8, "sw": 47, "sh": 24, "sx": 47, "sy": 128 }, { "x": 10, "y": 8, "sw": 47, "sh": 28, "sx": 359, "sy": 92 }, { "x": 10, "y": 9, "sw": 45, "sh": 32, "sx": 177, "sy": 92 }, { "x": 10, "y": 9, "sw": 44, "sh": 36, "sx": 0, "sy": 92 }, { "x": 10, "y": 10, "sw": 41, "sh": 39, "sx": 341, "sy": 48 }, { "x": 10, "y": 10, "sw": 39, "sh": 41, "sx": 183, "sy": 48 }, { "x": 9, "y": 10, "sw": 36, "sh": 44, "sx": 36, "sy": 48 }, { "x": 9, "y": 10, "sw": 32, "sh": 45, "sx": 401, "sy": 0 }, { "x": 8, "y": 10, "sw": 28, "sh": 47, "sx": 108, "sy": 0 }, { "x": 8, "y": 10, "sw": 24, "sh": 47, "sx": 216, "sy": 0 }, { "x": 7, "y": 9, "sw": 20, "sh": 48, "sx": 20, "sy": 0 }, { "x": 6, "y": 9, "sw": 15, "sh": 47, "sx": 303, "sy": 0 }, { "x": 5, "y": 8, "sw": 10, "sh": 46, "sx": 359, "sy": 0 }, { "x": 9, "y": 9, "sw": 15, "sh": 47, "sx": 318, "sy": 0 }, { "x": 13, "y": 9, "sw": 20, "sh": 48, "sx": 40, "sy": 0 }, { "x": 17, "y": 10, "sw": 24, "sh": 47, "sx": 240, "sy": 0 }, { "x": 21, "y": 10, "sw": 28, "sh": 47, "sx": 136, "sy": 0 }, { "x": 24, "y": 10, "sw": 32, "sh": 45, "sx": 433, "sy": 0 }, { "x": 27, "y": 10, "sw": 36, "sh": 44, "sx": 72, "sy": 48 }, { "x": 30, "y": 10, "sw": 39, "sh": 41, "sx": 222, "sy": 48 }, { "x": 32, "y": 10, "sw": 41, "sh": 39, "sx": 382, "sy": 48 }, { "x": 34, "y": 9, "sw": 44, "sh": 36, "sx": 44, "sy": 92 }, { "x": 36, "y": 9, "sw": 45, "sh": 32, "sx": 222, "sy": 92 }, { "x": 37, "y": 8, "sw": 47, "sh": 28, "sx": 406, "sy": 92 }, { "x": 38, "y": 8, "sw": 47, "sh": 24, "sx": 94, "sy": 128 }, { "x": 39, "y": 7, "sw": 48, "sh": 20, "sx": 284, "sy": 128 }, { "x": 39, "y": 6, "sw": 47, "sh": 15, "sx": 0, "sy": 152 }, { "x": 38, "y": 6, "sw": 46, "sh": 11, "sx": 140, "sy": 152 }, { "x": 39, "y": 9, "sw": 47, "sh": 15, "sx": 47, "sy": 152 }, { "x": 39, "y": 13, "sw": 48, "sh": 20, "sx": 332, "sy": 128 }, { "x": 38, "y": 17, "sw": 47, "sh": 24, "sx": 141, "sy": 128 }, { "x": 37, "y": 21, "sw": 47, "sh": 28, "sx": 453, "sy": 92 }, { "x": 36, "y": 24, "sw": 45, "sh": 32, "sx": 267, "sy": 92 }, { "x": 34, "y": 27, "sw": 44, "sh": 36, "sx": 88, "sy": 92 }, { "x": 32, "y": 30, "sw": 41, "sh": 39, "sx": 423, "sy": 48 }, { "x": 30, "y": 32, "sw": 39, "sh": 41, "sx": 261, "sy": 48 }, { "x": 27, "y": 34, "sw": 36, "sh": 44, "sx": 108, "sy": 48 }, { "x": 24, "y": 36, "sw": 32, "sh": 45, "sx": 465, "sy": 0 }, { "x": 21, "y": 37, "sw": 28, "sh": 47, "sx": 164, "sy": 0 }, { "x": 17, "y": 38, "sw": 24, "sh": 47, "sx": 264, "sy": 0 }, { "x": 13, "y": 39, "sw": 20, "sh": 48, "sx": 60, "sy": 0 }, { "x": 9, "y": 39, "sw": 15, "sh": 47, "sx": 333, "sy": 0 } ];
		this.minutesTexture = new Texture({ path: "minutes.png" });
	}
	onDisplaying(port) {
		port.interval = 30000;
		port.start();
		this.onTimeChanged(port);
	}
	onTimeChanged(port) {
		const date = new Date();
		const hours = date.getHours();
		const minutes = date.getMinutes();
		this.hours = ((hours % 12) * 5) + Math.floor(minutes / 12);
		this.minutes = minutes;
		port.invalidate();
	}
	onDraw(port) {
		const $ = this.$;
		const x = port.width >> 1;
		const y = port.height >> 1;
		let hand;
		hand = this.hoursData[this.hours];
		port.drawTexture(this.hoursTexture, this.color, x - hand.x, y - hand.y, hand.sx, hand.sy, hand.sw, hand.sh);
		hand = this.minutesData[this.minutes];
		port.drawTexture(this.minutesTexture, this.color, x - hand.x, y - hand.y, hand.sx, hand.sy, hand.sw, hand.sh);
	}
}

const SetTimeMenuItemContainer = Container.template($ => ({
	left:0, width:128, top:0, height:128, clip:true,
	contents: [
		Container($, {
			top:12, skin:assets.skins.time,
			contents: [
				Port($, { Behavior: SetTimeMenuItemBehavior, width:64, height:64 }),
			]
		}),
		Label($, { left:0, right:0, height:24, bottom:12, string:$.label }),
	]
}));

Menu.SetTime = SetTimeMenuItemContainer;

