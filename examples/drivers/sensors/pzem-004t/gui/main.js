/*
 * Copyright (c) 2023 Moddable Tech, Inc.
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

import PZEM004T from "embedded:sensor/Energy/PZEM004T";
import Timer from "timer";
import config from "mc/config";

const WHITE = "white";
const BLUE = "#192eab"
const whiteSkin = new Skin({ fill:WHITE });
const blueSkin = new Skin({ fill:BLUE });
const labelStyle = new Style({ font:"semibold 28px Open Sans", color:WHITE, horizontal:"left"});
const buttonStyle = new Style({ font:"semibold 28px Open Sans", color:WHITE, horizontal:"center"});

const ButtonSkin = Skin.template({ fill: ["#0033cc", "#668cff"] });
class ButtonBehavior extends Behavior {
	onTouchBegan(button) {
		button.state = 1;	
	}
	onTouchEnded(button) {
		button.state = 0;
		application.distribute("doClear")
	}
}

const Button = Container.template($ => ({
	active: true, Skin: ButtonSkin,
	contents: [
		Label($, { left: 10, right: 10, top: 0, bottom: 0, style: buttonStyle, state: 0, string: "Clear Energy" }),
	],
	Behavior: ButtonBehavior
}));

let EnergyApplication = Application.template($ => ({
	skin:whiteSkin,
	contents: [
		Container($, {
			left: 0, right: 0, top: 0, bottom: 0, skin: blueSkin, anchor: "mainContainer",
			Behavior: class extends Behavior {
				onCreate(container, data){
					this.data = data;

					this.sensor = new PZEM004T({
						sensor: {
							...device.Serial.default,
							transmit: config.transmit,
							receive: config.receive,
							port: config.port
						}
					});

					Timer.set(() => {
						this.sensor.sample((error, sample) => {
							if (error === null)
								application.distribute("onSample", sample);
						});
					}, 0, 1_000);
				}
				onSample(container, sample) {
					const data = this.data;
					data.voltage.string = `Voltage: ${sample.voltage} V`;
					data.current.string = `Current: ${sample.current} A`;
					data.power.string = `Power: ${sample.power} W`;
					data.energy.string = `Energy: ${sample.energy} Wh`;
					data.frequency.string = `Freq: ${sample.frequency} Hz`;
					data.powerfactor.string = `PF: ${sample.powerFactor}`;
				}
				doClear(container) {
					this.sensor.configure({resetEnergy: true});
				}
			},
			contents:[
				Label($, { anchor: "voltage", left: 0, right: 0, top: 10, height: 30, style: labelStyle, state: 0 }),
				Label($, { anchor: "current", left: 0, right: 0, top: 40, height: 30, style: labelStyle, state: 0 }),
				Label($, { anchor: "power", left: 0, right: 0, top: 70, height: 30, style: labelStyle, state: 0 }),
				Label($, { anchor: "energy", left: 0, right: 0, top: 100, height: 30, style: labelStyle, state: 0 }),
				Label($, { anchor: "frequency", left: 0, right: 0, top: 130, height: 30, style: labelStyle, state: 0 }),
				Label($, { anchor: "powerfactor", left: 0, right: 0, top: 160, height: 30, style: labelStyle, state: 0 }),
				Button($, { anchor: "button", left: 20, right: 20, top: 210, bottom: 30})
			]
		})
	]
}));

export default function () {
	new EnergyApplication({}, { displayListLength:2048, touchCount:1 });
}