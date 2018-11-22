/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

import CLI from "cli";

CLI.install(function(command, parts) {
	switch (command) {
		case "modules":
			for (let name in require.cache)
				this.line(name);
			break;

		case "help":
			this.line("modules - list loaded modules");
			break;

		default:
			return false;
	}
	return true;
});
