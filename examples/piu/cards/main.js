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
import Timeline from "piu/Timeline";

const team = [
	{
		name: "Andy Carle",
		title: "Product & Prototype Engineer",
		address: "andy@moddable.tech",
	},
	{
		name: "Brian Friedkin",
		title: "Principal Engineer",
		address: "brian@moddable.tech",
	},
	{
		name: "Chris Krueger",
		title: "Creative Director",
		address: "chris@moddable.tech",
	},
	{
		name: "Lizzie Prader",
		title: "Software Engineer",
		address: "lizzie@moddable.tech",
	},
	{
		name: "Michael Kellner",
		title: "CFO & Embedded Engineer",
		address: "mkellner@moddable.tech",
	},
	{
		name: "Peter Hoddie",
		title: "Principal",
		address: "peter@moddable.tech",
	},
	{
		name: "Patrick Soquet",
		title: "Software Architect",
		address: "ps@moddable.tech",
	},
];
const WHITE = "white";
const BLUE = "#192eab"

const whiteSkin = new Skin({ fill:WHITE });
const blueSkin = new Skin({ fill:BLUE });
const logoTexture = new Texture("logo.png");
const logoSkin = new Skin({ texture:logoTexture, color:[BLUE, WHITE], x:0, y:0, width:60, height:60 });
const companySkin = new Skin({ texture:logoTexture, color:WHITE, x:60, y:0, width:180, height:60 });
const labelStyle = new Style({ font:"semibold 16px Open Sans", color:WHITE, horizontal:"right", right:30});

let CardApplication = Application.template($ => ({
	skin:whiteSkin,
	Behavior: class extends Behavior {
		onCreate(application, anchors) {
			this.reverse = false;

			this.anchors = anchors;
			let index = this.index = 0;
			let person = team[index];
			for (let i in person)
				anchors[i].string = person[i];
				
			this.timeline = (new Timeline)
				.to(anchors.background.behavior, { x:0 }, 500, Math.quadEaseOut, 500)
				.to(anchors.logo, { x:20, y:15, state:1 }, 500, Math.quadEaseOut, -500)
				.to(anchors.company, { x:40, y:15 }, 500, Math.quadEaseOut, -500)
				.to(anchors.name, { x:0 }, 250, Math.quadEaseOut, -125)
				.to(anchors.title, { x:0 }, 250, Math.quadEaseOut, -125)
				.to(anchors.address, { x:0 }, 250, Math.quadEaseOut)
			application.duration = this.timeline.duration + 500;
			application.start();	
		}
		onDisplaying(application) {
			if (application.height != 320 || application.width != 240)
				trace("WARNING: This application was designed to run on a 240x320 screen.\n");
		}
		onFinished(application) {
			this.reverse = !this.reverse;
			if (!this.reverse) {
				this.index++;
				if (this.index >= team.length)
					this.index = 0;
				let person = team[this.index];
				let anchors = this.anchors;
				for (let i in person)
					anchors[i].string = person[i];
			}
			application.time = 0;
			application.start();
		}
		onTimeChanged(application) {
			let time = application.time;
			if (this.reverse)
				time = application.duration - time;
			this.timeline.seekTo(time);
		}
	},
	contents: [
		Die($, {
			anchor:"background", left:0, right:0, top:0, bottom:0, 
			Behavior: class extends Behavior {
				onCreate(die) {
					this.die = die;
					this.left = 240;
				}
				get x() {
					return this.left;
				}
				set x(it) {
					it = this.left = Math.round(it);
					this.die.set(it, 0, 240 - it, 320);
					this.die.cut();
				}
			},
			contents: [
				Content($, { left:0, right:0, top:0, bottom:0, skin:blueSkin, state:0 } ),
			],
		}),
		Content($, { anchor:"logo", left:90, top:130, skin:logoSkin, state:0 } ),
		Content($, { anchor:"company", left:240, top:15, skin:companySkin, state:1 } ),
		Label($, { anchor:"name", left:240, width:240, top:80, height:20, style:labelStyle, state:0 } ),
		Text($, { anchor:"title", left:240, width:240, top:102, height:45, style:labelStyle, state:0 } ),
		Label($, { anchor:"address", left:240, width:240, top:280, height:20, style:labelStyle, state:0 } ),
	]
}));

export default function () {
	new CardApplication({}, { displayListLength:1024, touchCount:0 });
}
