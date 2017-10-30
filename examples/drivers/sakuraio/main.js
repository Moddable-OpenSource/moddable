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

import SakuraIO from "sakuraio";
import Timer from "timer";

const lte = new SakuraIO;

trace(`Product ID  ${lte.getProductID()}\n`);
trace(`Unique ID  ${lte.getUniqueID()}\n`);
trace(`Firmware version  ${lte.getFirmwareVersion()}\n`);

let echo = lte.echoback([11]);
if ((1 != echo.length) || (11 !== echo[0]))
	throw new Error("bad echo");

let counter = 0;

Timer.repeat(() => {
	if (!(lte.getConnectionStatus() & 0x80)) {
		trace("Waiting to come online\n");
		return;
	}

	trace(`Connection status  ${lte.getConnectionStatus()}\n`);
	trace(`Signal quality  ${lte.getSignalQuality()}\n`);
	trace(`Unix UTC Time ${lte.getUnixTime().toUTCString()}\n`);

	lte.enqueueInt32(0, ++counter);

	let txQueueLength = lte.getTxQueueLength();
	trace(`txQueueLength: available ${txQueueLength.available}, queued ${txQueueLength.queued}\n`);
	if (txQueueLength.queued > 30) {
		trace("clear\n");
		lte.clearTx();
	}
	else if (txQueueLength.queued > 5) {
		trace("send\n");
		lte.send();
	}

	let rxQueueLength = lte.getRxQueueLength()
	trace(`rxQueueLength: available ${rxQueueLength.available}, queued ${rxQueueLength.queued}\n`);
	while (rxQueueLength.queued--) {
		let item = lte.dequeue();
		trace(`dequeue: channel ${item.channel}, type ${item.type}, value ${item.value}\n`);
	}
}, 1000);
