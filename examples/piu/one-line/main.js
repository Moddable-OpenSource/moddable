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
 
import WiFi from "wifi";
import {Server} from "websocket"
import {} from "piu/MC";

import Timeline from "piu/Timeline";
import WipeTransition from "piu/WipeTransition";

const BLACK = "black"
const TRANSPARENT = "transparent"
const WHITE = "white";

const whiteSkin = new Skin({ fill:WHITE });
const blackSkin = new Skin({ fill:BLACK });
const backgroundSkin = new Skin({ fill:[WHITE, BLACK] });
const labelStyle = new Style({ font:"600 28px Open Sans", color:[BLACK, WHITE], horizontal:"left" });

let strings = [
	"Moddable Wi-Fi",
	"ws:/192.168.4.1/",
];

let LineContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:backgroundSkin, state:$.state,
	Behavior: class extends Behavior {
		onDisplayed(container) {
			let label = container.first;
			let delta = label.width - container.width;
			let duration = (delta > 0) ? 10 * delta : 0;
			this.scrollDelta = (delta > 0) ? 0 - delta : 0;
			this.scrollFrom = 250 / (duration + 500);
			this.scrollTo = (250 + duration) / (duration + 500);
			container.duration = duration + 500;
			container.time = 0;
			container.start();
		}
		onFinished(container) {
			container.bubble("onScrolled");
		}
		onTimeChanged(container) {
			let label = container.first;
			let fraction = container.fraction;
			if ((this.scrollFrom <= fraction) && (fraction <= this.scrollTo)) {
				label.x = Math.round(this.scrollDelta * (fraction - this.scrollFrom) / (this.scrollTo - this.scrollFrom));
			}
		}
	},
	contents: [
		Label($, { left:0, top:0, bottom:0, string:$.string, state:$.state } ),
	]
}));

let LineApplication = Application.template($ => ({
	style:labelStyle,
	Behavior: class extends Behavior {
		onCreate(application) {
			WiFi.accessPoint({
				ssid: "Moddable",
				password: "12345678",
				channel: 8,
				interval: 75,
				hidden: false
			});
			this.server = new Server({});
			this.server.callback = function (message, value)
			{
				switch (message) {
					case 1:
						trace("main.js: socket connect.\n");
						break;

					case 2:
						trace("main.js: websocket handshake success\n");
						break;

					case 3:
						trace(`main.js: websocket message received: ${value}\n`);
						this.write(value);		// echo
						application.delegate("onReceived", value);
						break;

					case 4:
						trace("main.js: websocket close\n");
						break;
				}
			}
			this.index = 0;
			this.state = 0;
			application.add(new LineContainer({ state:this.state, string:strings[this.index]}));
			application.first.delegate("onDisplayed");
		}
		onReceived(application, value) {
			this.index = strings.length - 1;
			strings.push(value);
			application.run(null);
			this.onScrolled(application);
		}
		onScrolled(application) {
			this.index++;
			if (this.index >= strings.length)
				this.index = 0;
			this.state = this.state ? 0 : 1;
			let transition = new WipeTransition(500, Math.quadEaseOut, "middle");
			application.run(transition, application.first, new LineContainer({ state:this.state, string:strings[this.index]}));
		}
		onTransitionEnded(application) {
			application.first.delegate("onDisplayed");
		}
	},
}));

export default function () {
	new LineApplication({ }, { displayListLength:1024, touchCount:0 });
}
