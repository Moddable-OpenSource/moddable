/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
import Timer from "timer";

const WHITE = "white";
const BLUE = "#192eab"

const whiteSkin = new Skin({ fill:WHITE });
const blueSkin = new Skin({ fill:BLUE });

const logoTexture = new Texture("moddable.png");
const logoSkin = new Skin({ texture: logoTexture, color: WHITE, x: 0, y: 0, width: 200, height: 78 });
const labelStyle = new Style({ font:"semibold 16px Open Sans", color:WHITE, horizontal:"center"});

class RotationLabel extends Behavior {
	onDisplaying(label){
		if (application.rotation === undefined){
			trace("This host does not support hardware rotation.\n");
		}else{
			Timer.repeat(id => {
				application.rotation = (application.rotation + 90) % 360;
				label.string = `application.rotation: ${application.rotation}`;
			}, 3000);
		}
	}
}

let RotationApplication = Application.template($ => ({
	skin:blueSkin,
	contents: [
		Content($, { anchor: "logo", left: 10, right: 10, top: 15, skin: logoSkin}),
		Label($, { left: 0, right: 0, bottom: 20, height: 20, style: labelStyle,string: "application.rotation: 0", Behavior: RotationLabel }),
	]
}));

export default function () {
	new RotationApplication({}, { displayListLength:1024, touchCount:0 });
}
