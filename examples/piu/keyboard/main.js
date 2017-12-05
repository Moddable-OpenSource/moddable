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

const WhiteSkin = Skin.template({fill:"white"});
const OpenSans18 = Style.template({ font: "semibold 18px Open Sans", color: "black", horizontal:"center", vertical:"middle" });
const OpenSans20 = Style.template({ font: "20px Open Sans", color: "black", horizontal:"left", vertical:"middle"});

let theString = "";
let keyboardUp = true;

function gotKey(key){
  if (key == BACKSPACE){
    theString = theString.slice(0, -1);
  }else if(key == SUBMIT){
    trace("String is: " + theString + "\n");
		theString = "";
		application.distribute("doKeyboardTransitionOut", keyboardTransitionDone);
  }else{
    theString += key;
  }
  application.first.first.string = theString;
}

let keyboardTransitionDone = function(){
	application.first.first.next.remove(application.first.first.next.first);
	keyboardUp = false;
}

let MainCon = Column.template($ => ({
  left: 0, right: 0, top: 0, bottom: 0, active: true,
  Skin: WhiteSkin,
  contents:[
    Label($, {left: 25, right: 0, top: 0, height: 76, string: "", Style: OpenSans20}),
    Container($, {
      left: 0, right: 0, top: 0, bottom: 0, contents: [
        new Keyboard({style: new OpenSans18(), callback: gotKey, doTransition: true})
      ]
    }),
  ],
	Behavior: class extends Behavior {
		onTouchEnded(column){
			if (!keyboardUp){
				keyboardUp = true;
				column.first.next.add(new Keyboard({style: new OpenSans18(), callback: gotKey, doTransition: true}));
			}
		}
	}
}));

export default new Application(null, {
	commandListLength:2048,
	displayListLength:2600,
	touchCount:1,
	Behavior: class extends Behavior {
		onCreate(application) {
			application.add(new MainCon());
		}
	},
});
