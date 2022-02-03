/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
 * Copyright (c) Wilberforce
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

import { Bridge, HTTPServer } from "bridge/webserver";
import Net from "net"

const hotspot = new Map;

// iOS 8/9
hotspot.set("/library/test/success.html",{status: 302,body: "Success"});
hotspot.set("/hotspot-detect.html",{status: 302,body: "Success"});

// Windows
hotspot.set("/ncsi.txt",{status: 302,body: "Microsoft NCSI"});
hotspot.set("/connecttest.txt",{status: 302,body: "Microsoft Connect Test"});
hotspot.set("/redirect",{status: 302,body: ""}); // Win 10

// Android
hotspot.set("/mobile/status.php", {status:302}); // Android 8.0 (Samsung s9+)
hotspot.set("/generate_204", {status:302}); // Android actual redirect
hotspot.set("/gen_204", {status:204}); // Android 9.0

export class BridgeHotspot extends Bridge {
	constructor() {
		super();
	}
	handler(req, message, value, etc) {
		switch (message) {
			case HTTPServer.status:
				
				req.redirect=hotspot.get(value); // value is path
				if ( req.redirect) return; // Hotspot url match
				delete req.redirect;
				return this.next?.handler(req, message, value, etc);
			case HTTPServer.header: {
					if ( "host" === value ) {
						req.host=etc;
						trace(`BridgeHotspot: http://${req.host}${req.path}\n`);
					}
					return this.next?.handler(req, message, value, etc);
				}			
			case HTTPServer.prepareResponse:

				if( req.redirect) {
					let apIP=Net.get("IP", "ap");
					let redirect={
						headers: [ "Content-type", "text/plain", "Location",`http://${apIP}`],
						...req.redirect
					};
					trace(`Hotspot match: http://${req.host}${req.path}\n`);
					trace(JSON.stringify(redirect),'\n');

					return redirect;
				}
		}
		return this.next?.handler(req, message, value, etc);
	}
}
Object.freeze(hotspot);

/* TO DO
add dns constructor flag. then becomes self contained.
*/
