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

const blackSkin = new Skin({ fill:"black" });
const labelSkin = new Skin({ fill:"black", stroke:"white", top:1 });
const labelStyle = new Style({ font:"OpenSans-Semibold-16", color:"white" });
let counter = 0;

let SpiralContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:blackSkin,
	contents:[
		Port($, {
			left:0, right:0, top:0, bottom:20, 
			Behavior: class extends Behavior {
				onCreate(port) {
					this.reverse = false;
				}
				onDisplaying(port) {
					this.onFinished(port);
				}
				onDraw(port, x, y, w, h) {
					if ((w == 1) && (h == 1)) {
						port.fillColor(0xFFFFFFFF, x, y, 1, 1);
					}
				}
				onFinished(port) {
					this.reverse = !this.reverse;
					this.a = -1 + (Math.random() * 2);
					this.b = 0.5 + (Math.random() * 4);
					this.c = 1 + (Math.random() * 8);
					this.x = Math.random() * port.width;
					this.y = Math.random() * port.height;
					port.duration = 1000 + (Math.random() * 10000);	
					port.time = 0;
					port.start();
				}
				onTimeChanged(port) {
					let fraction = this.reverse ? port.fraction : 1 - port.fraction;
					let angle = fraction * 2 * Math.PI * this.c;
					let radius = this.a + (this.b * angle);
					let x =	Math.round(this.x + (radius * Math.cos(angle)));
					let y = Math.round(this.y + (radius * Math.sin(angle)));
					port.invalidate(x, y, 1, 1);
					counter++;
				}
			},
		})
	],
}));

let LabelContainer = Container.template($ => ({
	left:0, right:0, height:20, bottom:0, skin:labelSkin, style:labelStyle,
	contents:[
		Label($, { anchor:"count", left:0, right:0, height:20, style:{ horizontal:"left", left:5 },
			Behavior: class extends Behavior {
				onDisplaying(label) {
					label.interval = 500;
					label.start();
				}
				onTimeChanged(label) {
					let seconds = label.time / 1000;;
					let string = "";
					let value = Math.floor(seconds / 3600);
					if (value < 10) string += "0";
					string += value.toString() + ":";
					seconds %= 3600;
					value = Math.floor(seconds / 60);
					if (value < 10) string += "0";
					string += value.toString() + ":";
					seconds %= 60;
					value = Math.floor(seconds);
					if (value < 10) string += "0";
					string += value.toString();
					label.string = string;
				}
			},
		}),
		Label($, { 
			left:0, right:0, height:20, style:{ horizontal:"right", right:5 },
			Behavior: class extends Behavior {
				onDisplaying(label) {
					label.interval = 500;
					label.start();
				}
				onTimeChanged(label) {
					label.string = counter.toString();
				}
			},
		}),
	],
}));

let SpiralApplication = Application.template($ => ({
	Behavior: class extends Behavior {
		onCreate(application) {
			application.first.delegate("onDisplayed");
		}
		onTransitionBeginning(application) {
		}
		onTransitionEnded(application) {
			application.first.delegate("onDisplayed");
		}
	},
	contents: [
		SpiralContainer($, {}),
		LabelContainer($, {}),
	]
}));

export default function () {
	new SpiralApplication({}, { displayListLength:2048, touchCount:0 });
}
