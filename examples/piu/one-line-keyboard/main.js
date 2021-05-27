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

import {} from "piu/MC";

import {Keyboard, BACKSPACE, SUBMIT} from "keyboard";
import {Client} from "websocket"

const OpenSans18 = Style.template({ font: "semibold 18px Open Sans", color: "black", horizontal:"center", vertical:"middle" });
const OpenSans20 = Style.template({ font: "normal normal normal 20px Open Sans", color: "black", horizontal:"left", vertical:"middle"});

let theString = "";

let KeyboardApplication = Application.template($ => ({
  	skin: new Skin({ fill:"white" }),
	Behavior: class extends Behavior {
		onCreate(application) {
			this.ws = new Client({host: "192.168.4.1"});
			this.ws.callback = function(message, value)
			{
				switch (message) {
					case 1:
						trace("socket connect\n");
						break;

					case 2:
						trace("websocket handshake success\n");
						break;

					case 3:
						trace(`websocket message received: ${value}\n`);
						break;

					case 4:
						trace("websocket close\n");
						break;
				}
			}
		}
    onKeyUp(application, key){
      if (key == BACKSPACE){
        theString = theString.slice(0, -1);
      }else if(key == SUBMIT){
        trace("String is: " + theString + "\n");
    	application.behavior.ws.write(theString);
    	theString = "";
      }else{
        theString += key;
      }
      application.first.string = theString;
    }
	},
	contents:[
		Label($, {left: 25, right: 0, top: 0, height: 76, string: "", Style: OpenSans20}),
		Container($, {
			left: 0, right: 0, top: 76, bottom: 0, contents: [
				Keyboard($, {style: new OpenSans18(), doTransition: false})
			]
		}),
	],
}));

export default function () {
	new KeyboardApplication({ }, { commandListLength:2048, displayListLength:2600, touchCount:1 });
}
