/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
import Ping from "ping";

const HOST = "test.moddable.com";
const ID = 0;
const INTERVAL = 1000;	// interval between pings

let ping = new Ping({host: HOST, id: ID, interval: INTERVAL}, (message, value, etc) => {
	switch (message) {
		case -1:
			trace(`Error: ${value}\n`);
			break;
		case 1:
			trace(`${value} bytes from ${etc.address}: icmp_seq=${etc.icmp_seq}\n`);
			if (etc.icmp_seq >= 20)
				ping.close();
			break;
		case 2:
			trace(`Request timeout for icmp_seq ${etc.icmp_seq}\n`);
			break;
	}
});

