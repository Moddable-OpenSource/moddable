/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

import {FirmataSerial, FirmataTCPClient, FirmataTCPServer} from "firmataserver";
import Poco from "commodetto/Poco";

export default function() {
	if (global.screen) {
		let poco = new Poco(screen);
		poco.begin(0, 0, screen.width, screen.height);
		poco.fillRectangle(poco.makeColor(0, 0, 255), 0, 0, screen.width, screen.height);
		poco.end();
	}

	new FirmataSerial;
//	new FirmataSerial({baud: 921400});
//	new FirmataTCPClient({address: "192.168.1.19"});
//	new FirmataTCPServer;
}
