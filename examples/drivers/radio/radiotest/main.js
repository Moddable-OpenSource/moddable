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

/* Demonstrates the use of the Mighty Gecko radio. */
/* Deploy on two Mighty Geckos. Pressing either button */
/* sends button state to the other device for display */
/* while alternately blinking the two  LEDs on the device. */

import Timer from "timer";
import Digital from "pins/digital";
import Monitor from "monitor";
import Radio from "radio";

// packet: 32 bytes
// long[0] - senderAddress
// long[1] - tag 'mdbn' = 0x6d64626e;
// long[2] - seqNum
// long[3] - packetType
// long[4] - button0 state
// long[5] - button1 state
// long[6-7] - unused

const kModdableTag = 0x6d64626e;

export default function() {
	let radio = new Radio();

	let sendBuffer = new ArrayBuffer(8 * 4);
	let sendLongs = new Uint32Array(sendBuffer);

	let seqNum = 0;
	let packetType = 1;

	let receivedPacket = 0;

	radio.listen(1);

	radio.onMessage = function(msg) {
		let longs = new Uint32Array(msg);
		receivedPacket++;

		let requesterID = longs[0];
		let tag = longs[1];
		let seqNum = longs[2];
		let type = longs[3];
		let button0 = longs[4];
		let button1 = longs[5];
		trace("rcvd from: ", requesterID, " tag: ", tag, " seqNum ", seqNum, " type ", type, " button0 ", button0, " button1 ", button1, "\n"); 
	}

	sendLongs[0] = radio.getUnique();
	sendLongs[1] = kModdableTag;
	sendLongs[3] = packetType;

	let led1 = new Digital({pin: 4, port: "gpioPortF", mode: Digital.Output});
	let led2 = new Digital({pin: 5, port: "gpioPortF", mode: Digital.Output});

	let monitor1 = new Monitor({pin: 6, port: "gpioPortF", mode: Digital.InputPullUp, edge: Monitor.Falling});
	let monitor2 = new Monitor({pin: 7, port: "gpioPortF", mode: Digital.InputPullUp, edge: Monitor.Rising});

	function doRadioSend() {
		sendLongs[2] = seqNum++;
		sendLongs[4] = monitor1.read();
		sendLongs[5] = monitor2.read();
		radio.postMessage(sendBuffer);
	}

	monitor1.onChanged = function() {
		trace("Button 0, ", this.read(), " times\n");
		doRadioSend();
	}
	monitor2.onChanged = function() {
		trace("Button 1, ", this.read(), " times\n");
		doRadioSend();
	}

	let count = 0;
	Timer.repeat(() => {
		trace(`repeat ${++count} \n`);
		led1.write(~count & 1);
		led2.write(count & 1);
	}, 400);
}
