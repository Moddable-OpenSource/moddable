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

import Console from "console";
import CLI from "cli";

CLI.install(function (command, params) {
	switch (command) {
		case "example":
			this.line(`example with ${params.length} parameters`);
			break;
		case "help":
			this.line(`example [params] - display number of parameters`);
			break;
		default:
			return false;
	}
	return true;
});

let console = new Console;
