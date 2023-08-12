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

class WakeBehavior extends View.Behavior {
	onCreate(container, view) {
		super.onCreate(container, view);
	}
	onDisplayed(container) {
		screen.dither = true;
		this.countdown = this.view.motionDetected ? 7 : 5;
		container.duration = 1000;
		container.start();
	}
	onFinished(container) {
		let pane = container.last;
		const label = pane.first;
		const fade = label.next;
		this.countdown--;
		if (this.countdown) {
			label.string = this.countdown;
			fade.state = 1;
			pane.visible = this.countdown <= 5;
			pane = pane.previous;
			pane.visible = this.countdown > 5;
			container.time = 0;
			container.start();
		}
		else {
			pane.visible = false;
			pane = pane.previous;
			pane.visible = false;
			pane = pane.previous;
			pane.visible = true;
			pane = pane.previous;
			pane.visible = true;
			container.defer("onSleep");
		}
	}
	onSleep(container) {
		controller.sleep({ duration:60000, accelerometer:true }, 999);
	}
	onTimeChanged(container) {
		const fade = container.last.last;
		const fraction = container.fraction;
		fade.state = 1 - Math.quadEaseIn(container.fraction);
	}
	onUndisplaying(container) {
		container.stop();
		screen.dither = false;
	}
}

const WakeContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:WakeBehavior,
	contents: [
		Container($, {
			left:0, width:128, top:0, height:64, visible:false,
			contents: [
				Label($, {left:0, width:128, top:4, height:26, style:assets.styles.wake, string:"Moddable Four" }),
				Label($, {left:0, width:128, top:30, height:26, string:"is asleep" }),
			], 
		}),
		Container($, {
			left:0, width:128, top:64, height:64, skin:assets.skins.focus, state:1, visible:false,
			contents: [
				Label($, {left:0, width:128, top:4, height:26, state:1, string:"Shake" }),
				Label($, {left:0, width:128, top:30, height:26, state:1, string:"to wake" }),
			], 
		}),
		Container($, {
			left:0, width:128, top:0, height:128, visible:$.motionDetected,
			contents: [
				Container($, {
					left:0, right:0, top:12, height:72,
					contents: [
						Content($, { skin:assets.skins.wake }),
					]
				}),
				Label($, { left:0, right:0, height:24, bottom:12, string:"Shaken" }),
			], 
		}),
		Container($, {
			left:0, width:128, top:0, height:128, visible:!$.motionDetected,
			contents: [
				Label($, {left:0, right:0, top:0, bottom:0, style:assets.styles.time, string:"5" }),
				Content($, {left:0, right:0, top:0, bottom:0, state:1, skin:assets.skins.blackFade}),
			], 
		}),
	]
}));

class WakeTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		let content = screen.last;
		if (!content.visible)
			content = content.previous;
		this.from(content, { x:screen.x + screen.width }, 200, Math.quadEaseOut, 0);
	}
}

export default class extends View {
	static get Behavior() { return WakeBehavior }
	
	constructor(data) {
		super(data);
		this.motionDetected = false;
	}
	get Template() { return WakeContainer }
	get Timeline() { return WakeTimeline }
};
