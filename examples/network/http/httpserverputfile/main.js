/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

import {Server} from "http"
import {File} from "file";
import Net from "net";
import config from "mc/config";

(new Server({})).callback = function(message, value) {
	switch (message) {
		case Server.status:					// request status received
			const path = config.file.root + value.slice(1);		// file path is HTTP path. strip leading "/" and add file system prefix
			File.delete(path);
			this.file = new File(path, true);
			break;

		case Server.headersComplete:		// prepare for request body
			return true;					// provide request body in fragments
	
		case Server.requestFragment:		// request body fragment
			this.file.write(this.read(ArrayBuffer));
			break;

		case Server.requestComplete:		// request body received
			this.file.close();
			break;
	}
}

trace(`Available on Wi-Fi "${Net.get("SSID")}"\n`)
trace(`curl http://${Net.get("IP")}/test.txt --upload-file "@/users/[your directory path here]/test.txt" -v\n`);
