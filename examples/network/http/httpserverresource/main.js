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

import {Server} from "http"
import Resource from "Resource"
import Net from "net"

const port = 8080;
const server = new Server({
	port
});
server.callback = function(message, value)
{
	switch (message) {
		case Server.status:
			this.path = value;
			break;

		case Server.prepareResponse:
			try {
				this.data = new Resource(this.path.slice(1));
				this.position = 0;
				return {
					headers: [
						"Content-type", "text/plain",
						"Content-length", this.data.byteLength,
					],
					body: true,
				};
			}
			catch {
				return {
					status: 404,
					headers: [
						"Content-type", "text/plain",
					],
					body: "file not found",
				};
			}
			break;

		case Server.responseFragment:
			if (this.position >= this.data.byteLength)
				return;

			const chunk = this.data.slice(this.position, this.position + value);
			this.position += chunk.byteLength;
			return chunk;
	}
}

trace(`Ready at http://${Net.get("IP")}:${port}/bartleby.txt\n`);
