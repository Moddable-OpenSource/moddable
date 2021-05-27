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

import WipeTransition from "piu/WipeTransition";
import CombTransition from "piu/CombTransition";

let appStyle = new Style({ font:"semibold 28px Open Sans", left:5, right:5 });
let skins = [
	new Skin({ fill:"#192eab" }),
	new Skin({ fill:"white" }),
];
let centerStyles = [
	new Style({ color:"white" }),
	new Style({ color:"#192eab" }),
];
let leftStyles =  [
	new Style({ color:"white", horizontal:"left" }),
	new Style({ color:"#192eab", horizontal:"left" }),
];
let rightStyles =  [
	new Style({ color:"white", horizontal:"right" }),
	new Style({ color:"#192eab", horizontal:"right" }),
];
let parameters = [
	{ title:"Comb", transition:CombTransition, first:"horizontal", last:4 },
	{ title:"Comb", transition:CombTransition, first:"vertical", last:4 },
	{ title:"Comb", transition:CombTransition, first:"horizontal", last:8 },
	{ title:"Comb", transition:CombTransition, first:"vertical", last:8 },
	{ title:"Wipe", transition:WipeTransition, first:"left" },
	{ title:"Wipe", transition:WipeTransition, first:"right" },
	{ title:"Wipe", transition:WipeTransition, last:"top" },
	{ title:"Wipe", transition:WipeTransition, last:"bottom" },
	{ title:"Wipe", transition:WipeTransition, first:"center" },
	{ title:"Wipe", transition:WipeTransition, last:"middle" },
	{ title:"Wipe", transition:WipeTransition, first:"center", last:"middle" },
	{ title:"Wipe", transition:WipeTransition, first:"left", last:"top" },
	{ title:"Wipe", transition:WipeTransition, first:"right", last:"top" },
	{ title:"Wipe", transition:WipeTransition, first:"right", last:"bottom" },
	{ title:"Wipe", transition:WipeTransition, first:"left", last:"bottom" },
	{ title:"Wipe", transition:WipeTransition, first:"center", last:"top" },
	{ title:"Wipe", transition:WipeTransition, first:"right", last:"middle" },
	{ title:"Wipe", transition:WipeTransition, first:"center", last:"bottom" },
	{ title:"Wipe", transition:WipeTransition, first:"left", last:"middle" },
];

let WipeContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:skins[$ % skins.length], 
	Behavior: class extends Behavior {
		onCreate(container, $) {
			let parameter = parameters[$ % parameters.length];
			let columnParams = { }
			let lastParams = { height:30 }
			let firstParams = { height:30 }
			if (parameter.title == "Wipe") {
				switch (parameter.first) {
				case "left":
					columnParams.left = 0;
					lastParams.left = 0;
					lastParams.string = "LEFT";
					lastParams.style = leftStyles[$ % skins.length];
					firstParams.left = 0;
					firstParams.style = leftStyles[$ % skins.length];
					break;
				case "right":
					columnParams.right = 0;
					lastParams.right = 0;
					lastParams.string = "RIGHT";
					lastParams.style = rightStyles[$ % skins.length];
					firstParams.right = 0;
					firstParams.style = rightStyles[$ % skins.length];
					break;
				case "center":
					lastParams.string = "CENTER";
					lastParams.style = centerStyles[$ % skins.length];
					firstParams.style = centerStyles[$ % skins.length];
					break;
				default:
					lastParams = null;
					firstParams.style = centerStyles[$ % skins.length];
					break;
				}
				switch (parameter.last) {
				case "top":
					firstParams.string = "TOP";
					columnParams.top = 0;
					break;
				case "bottom":
					firstParams.string = "BOTTOM";
					columnParams.bottom = 0;
					break;
				case "middle":
					firstParams.string = "MIDDLE";
					break;
				default:
					firstParams = null;
					break;
				}
			}
			else {
				firstParams.string = parameter.first.toUpperCase();
				firstParams.style = centerStyles[$ % skins.length];
				lastParams.string = "COMB " + parameter.last;
				lastParams.style = centerStyles[$ % skins.length];
			}
			let column = new Column($, columnParams);
			if (firstParams)
				column.add(new Label($, firstParams));
			if (lastParams)
				column.add(new Label($, lastParams));
			container.add(column);
		}
	},
}));

export default new Application(null, {
	commandListLength:1024,
	displayListLength:2048, 
	style:appStyle,
	touchCount:1,
	Behavior: class extends Behavior {
		onFinished(application) {
			let parameter = parameters[this.index % parameters.length];
			let transition = new parameter.transition(250, Math.quadEaseOut, parameter.first, parameter.last);
			this.index++;
			application.run(transition, application.first, new WipeContainer(this.index));
		}
		onCreate(application) {
			this.index = 0;
			application.add(new WipeContainer(0));
			application.duration = 250;
			application.start();
		}
		onTransitionEnded(application) {
			application.time = 0;
			application.start();
		}
	},
});

