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
import Timer from "timer";

const PASSWORDMODE = true; //Set to true to replace input with asterisks, false for clear text. 
const WhiteSkin = Skin.template({fill:"white"});
const OpenSans18 = Style.template({ font: "semibold 18px Open Sans", color: "black", horizontal:"center", vertical:"middle" });
const OpenSans20 = Style.template({ font: "20px Open Sans", color: "black", horizontal:"left", vertical:"middle"});

let theString = "";
let keyboardUp = true;
let timerID = undefined;

let MainCon = Column.template($ => ({
  left: 0, right: 0, top: 0, bottom: 0, active: true, name: "mainContainer",
  Skin: WhiteSkin,
  contents:[
    Label($, {left: 25, right: 0, top: 0, height: 76, string: "", Style: OpenSans20}),
    Container($, {
      left: 0, right: 0, top: 0, bottom: 0, name: "keyboardContainer", contents: [
        Keyboard($, {style: new OpenSans18(), doTransition: true})
      ]
    }),
  ],
	Behavior: class extends Behavior {
    onCreate(column, data){
      this.data = data;
    }
		onTouchEnded(column){
			if (!keyboardUp){
				keyboardUp = true;
				column.content("keyboardContainer").add(Keyboard(this.data, {style: new OpenSans18(), doTransition: true}));
			}
		}
	}
}));

export default new Application({}, {
	commandListLength:2048,
	displayListLength:2600,
	touchCount:1,
	Behavior: class extends Behavior {
		onCreate(application) {
			application.add(new MainCon());
		}
    onKeyUp(application, key){
      if (key == BACKSPACE){
        theString = theString.slice(0, -1);
      }else if(key == SUBMIT){
        trace("String is: " + theString + "\n");
    		theString = "";
    		application.content("mainContainer").content("keyboardContainer").first.delegate("doKeyboardTransitionOut");
      }else{
        theString += key;
      }
      if (PASSWORDMODE && theString.length > 0){
        if (timerID) Timer.clear(timerID);
        if (key != BACKSPACE){
          application.first.first.string = "*".repeat(theString.length - 1) + theString.charAt(theString.length - 1);
          timerID = Timer.set(id => {timerID = undefined; application.first.first.string = "*".repeat(theString.length);}, 500);
        }else{
          application.first.first.string = "*".repeat(theString.length);
        }
      }else{
        application.first.first.string = theString;
      }
    }
    onKeyboardTransitionFinished(application){
      let keyboardContainer = application.content("mainContainer").content("keyboardContainer");
      keyboardContainer.remove(keyboardContainer.first);
    	keyboardUp = false;
    }
	},
});
