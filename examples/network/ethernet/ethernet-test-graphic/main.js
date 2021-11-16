/*
 * Copyright (c) 2016-2021 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import {} from "piu/MC";
import Net from "net";
import Ethernet from "ethernet";

const backgroundSkin = new Skin({ fill: "white" });
const textStyle = new Style({ font:"semibold 28px Open Sans", color: ["black", "green", "red"] });

class AppBehavior extends Behavior {
	onDisplaying(application) {
		try {
			Ethernet.start();
			let monitor = new Ethernet((msg, code) => {
				switch (msg) {
					case Ethernet.connected:
						application.first.state = 0;
						application.first.string = "Physical link established...";
						break;

					case Ethernet.gotIP:
						let ip = Net.get("IP", "ethernet");
						application.first.state = 1;
						application.first.string = `Ethernet ready\nIP address: ${ip}`;
						break;

					case Ethernet.disconnected:
						application.first.state = 2;
						application.first.string = "Ethernet disconnected";
						break;
				}
			});
		} catch (e) {
			application.first.state = 2;
			application.first.string = e.message;
		}
	}
}

const EthernetTestApp = Application.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0, skin: backgroundSkin,
	contents: [
		Text($, { left: 30, right: 30, style: textStyle, string: "Ethernet not connected" })
	],
	Behavior: AppBehavior
}));


export default new EthernetTestApp({});
