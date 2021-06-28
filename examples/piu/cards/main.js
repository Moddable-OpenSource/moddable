/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
		address: "andy@moddable.com",
	},
	{
		name: "Brian Friedkin",
		title: "Principal Engineer",
		address: "brian@moddable.com",
	},
	{
		name: "Chris Krueger",
		title: "Creative Director",
		address: "chris@moddable.com",
	},
	{
		name: "Lizzie Prader",
		title: "Software Engineer",
		address: "lizzie@moddable.com",
	},
	{
		name: "Michael Kellner",
		title: "CFO & Embedded Engineer",
		address: "mkellner@moddable.com",
	},
	{
		name: "Peter Hoddie",
		title: "Principal",
		address: "peter@moddable.com",
	},
	{
		name: "Patrick Soquet",
		title: "Software Architect",
		address: "ps@moddable.com",
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
	contents: [
		Layout($, {
			left: 0, right: 0, top: 0, bottom: 0,
			Behavior: class extends Behavior {
				onCreate(layout, anchors){
					this.reverse = false;
					this.anchors = anchors;
					let index = this.index = 0;
					let person = team[index];
					for (let i in person)
						anchors[i].string = person[i];
				}
				onDisplaying(layout) {
					this.resetTimeline(layout);
					layout.start();
				}
				onFinished(layout) {
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
					layout.time = 0;
					layout.start();

				}
				onTimeChanged(layout) {
					let time = layout.time;
					if (this.reverse)
						time = layout.duration - time;
					this.timeline.seekTo(time);
				}
				resetTimeline(layout) {
					let anchors = this.anchors;
					this.timeline = (new Timeline)
						.on(anchors.background.behavior, { x: [layout.width, 0] }, 500, Math.quadEaseOut, 500)
						.on(anchors.logo, { x: [(layout.width - 60) / 2, 20], y: [(layout.height - 60) / 2, 15], state: [0, 1] }, 500, Math.quadEaseOut, -500)
						.on(anchors.company, { x: [layout.width, 40] }, 500, Math.quadEaseOut, -500)
						.on(anchors.name, { x: [layout.width, 0] }, 250, Math.quadEaseOut, -125)
						.on(anchors.title, { x: [layout.width, 0] }, 250, Math.quadEaseOut, -125)
						.on(anchors.address, { x: [layout.width, 0] }, 250, Math.quadEaseOut)

					layout.duration = this.timeline.duration + 500;
				}
				onFitHorizontally(layout, width) {
					this.anchors.name.width = this.anchors.title.width = this.anchors.address.width = this.anchors.background.width = width;
					this.resetTimeline(layout);
					return width;
				}
				onFitVertically(layout, height) {
					this.anchors.background.height = height;
					this.resetTimeline(layout);
					return height;
				}
			},
			contents:[
				Die($, {
					anchor: "background", left: 0, top: 0, width: 0, height: 0,
					Behavior: class extends Behavior {
						onCreate(die) {
							this.die = die;
							this.left = 0;
						}
						get x() {
							return this.left;
						}
						set x(it) {
							it = this.left = Math.round(it);
							this.die.set(it, 0, application.width - it, application.height);
							this.die.cut();
						}
					},
					contents: [
						Content($, { left: 0, right: 0, top: 0, bottom: 0, skin: blueSkin, state: 0 }),
					],
				}),
				Content($, { anchor: "logo", left: 20, top: 15, skin: logoSkin, state: 1 }),
				Content($, { anchor: "company", left: 40, top: 15, skin: companySkin, state: 1 }),
				Label($, { anchor: "name", left: 0, width: 0, top: 80, height: 20, style: labelStyle, state: 0 }),
				Text($, { anchor: "title", left: 0, width: 0, top: 102, height: 45, style: labelStyle, state: 0 }),
				Label($, { anchor: "address", left: 0, width: 0, bottom: 20, height: 20, style: labelStyle, state: 0 }),
			]
		})
	]
}));

export default function () {
	new CardApplication({}, { displayListLength:2048, touchCount:0 });
}
