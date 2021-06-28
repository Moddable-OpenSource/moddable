/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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

/*
 * Send a SMS message via SIM7100 module
 */

import Timer from "timer";
import SIM7100 from "sim7100";

const PHONE = "---Phone number here---";
const MESSAGE = "Hello Antarctica!";

let sim7100 = new SIM7100();

sim7100.onReady = function() {
	sim7100.reset();
	sim7100.sendSMS(PHONE, MESSAGE);
}

sim7100.onSMSSendComplete = function(device) {
	sim7100.stop();
}

sim7100.start();

