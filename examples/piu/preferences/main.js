/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import {} from "piu/MC";
import Preference from "preference";
import WipeTransition from "piu/WipeTransition";
import Timeline from "piu/Timeline";

const RED = 0;
const GREEN = 1;
const BLUE = 2;

const footerSkin = new Skin({ fill: "#202020" });
const backgroundSkins = [
	new Skin({ fill: "red" }), 
	new Skin({ fill: "green" }), 
	new Skin({ fill: "blue" })
];
const openSans20 = new Style({ font: "20px Open Sans", color: ["white", "black"] });
const circleTexture = new Texture({ path:"fill-circle-mask.png" });
const circleSkin = new Skin({ 
	texture: circleTexture, 
	color: ["#202020", "white"],
	x: 0, y: 0, width: 52, height: 52 
});
const outlineCircleTexture = new Texture({ path:"outline-circle-mask.png" });
const outlineCircleSkin = new Skin({ 
	texture: outlineCircleTexture, 
	color: "white",
	x: 0, y: 0, width: 52, height: 52 
});

class CircularButtonBehavior extends Behavior {
	onCreate(label, data) {
		this.pref = data.pref;
		label.state = data.state;
		label.skin = (label.state)? circleSkin : outlineCircleSkin;
	}
	onTouchEnded(label) {
		let pref = this.pref;
		Preference.set("settings", "backgroundColor", pref);
		label.active = false;
		this.select(label);
		label.container.distribute("unselect", pref);
		application.distribute("changeBackgroundColor", pref);
	}
	unselect(label, selected) {
		if (this.pref == selected) return;
		label.skin = outlineCircleSkin;
		label.state = 0;
		label.active = true;
	}
	select(label, state) {
		let timeline = this.timeline = new Timeline();
		label.skin = circleSkin;
		timeline.to(label, { state: 1 }, 300, Math.quadEaseOut, 0);
		label.duration = timeline.duration;
		timeline.seekTo(0);
		label.time = 0;
		label.start();
	}
	onTimeChanged(label) {
		let time = label.time;
		this.timeline.seekTo(time);
	}
	onFinished(label) {
		delete this.timeline;
	}
}

const labels = ["R", "G", "B"];
const CircularButton = Label.template($ => ({
	active: true, top: 14, height: 52, left: 0, right: 0, 
	state: $.state, string: labels[$.pref],
	Behavior: CircularButtonBehavior,
}));

const Background = Content.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0, skin: $
}));

const PreferencesApplication = Application.template($ => ({
	style: openSans20, skin: footerSkin,
	contents: [
		Container($, {
			anchor: "BACKGROUND", top: 0, bottom: 80, left: 0, right: 0,
		}),
		Row($, {
			anchor: "BUTTONS", bottom: 0, height: 80, left: 0, right: 0,
		}),
	],
	Behavior: class extends Behavior {
		onCreate(application, data) {
			this.data = data;
			let background = data.BACKGROUND;
			let buttonRow = data.BUTTONS;
			let backgroundColor = Preference.get("settings", "backgroundColor");
			if (backgroundColor === undefined) backgroundColor = BLUE;
			let options = [RED, GREEN, BLUE];
			for (let i in options) {
				let pref = options[i];
				if (pref == backgroundColor) {
					background.add(new Background(backgroundSkins[pref]));
					buttonRow.add(new CircularButton({ state: 1, pref }));
				} else {
					buttonRow.add(new CircularButton({ state: 0, pref }));
				}
			}
		}
		changeBackgroundColor(application, pref) {
			let background = this.data.BACKGROUND;
			let transition = new WipeTransition(300, Math.quadEaseOut, "center", "bottom");
			background.run(transition, background.first, new Background(backgroundSkins[pref]));
		}
	}
}));

export default new PreferencesApplication({}, { displayListLength: 2048, touchCount: 1 });

