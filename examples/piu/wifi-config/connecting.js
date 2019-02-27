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

import WiFi from "wifi";
import ASSETS from "assets";

export const ConnectingScreen = Container.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, Skin: ASSETS.WhiteSkin,
	contents: [
		// new ASSETS.LoadingBubbles({ top: 113, left: 123, squareSize: 15, frequency: 20000, color: [ 230, 0.74, 0.38 ] })
		new ASSETS.LoadingSpinner({ top: 70, left: 110, squareSize: 20, frequency: 1000, color: [ 230, 0.74, 0.38 ] }) // ~#192eab	
	],
	Behavior: class extends Behavior {
		onCreate(column, data) {
			this.data = data;
		}
		onDisplaying(column) {
			application.purge();
			new WiFi(this.data, message => {
			    if (message == "gotIP"){
			    	this.data.doNotRemove = true;
			    	application.delegate("doNext", "NETWORK_LIST_SCAN", this.data);
			    	return;
			    } else if (message == "disconnect") {
					application.delegate("doNext", "CONNECTION_ERROR", this.data);
					return;
			    } else {
			    	trace("message: "+message+"\n");
			    }
			});
		}
	}
}));


