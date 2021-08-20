/*
 * Copyright (c) 2021 Satoshi Tanaka
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import RMT from "pins/rmt";

const IrSendPin = 9; 	// M5StickC transmitter pin
let rmt = new RMT({pin: IrSendPin, channel: 0, divider: 80, direction: "tx",
					tx_config: {
						loop_en: 0,
						carrier_freq_hz: 38000,
						carrier_duty_percent: 33,
						carrier_level: 1,
						carrier_en: 1,
						idle_level: 0,
						idle_output_en: 0
					}});

rmt.onWritable = function() {}

//  power button of Sony TV 
let rawData = [2380, 602, 1184, 604, 588, 604, 1186, 604, 588, 604, 1184, 604, 588, 604, 588, 604, 1186, 604, 588, 604, 588, 604,  588, 604, 588, 25704, 2382, 604, 1184, 604, 586, 606, 1186, 604, 588, 604, 1186, 602, 588, 606, 588, 604, 1186, 604, 588, 604, 588, 608, 584, 606, 588, 25712, 2382, 602, 1186, 604, 588, 604, 1184, 604, 588, 604, 1186, 604, 586, 606, 588, 604, 1186, 602,  592, 600,  588, 604,  588, 604, 588];

button.a.onChanged = function(){
	if(button.a.read()) return;

	rmt.write(1, rawData);
}
