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

const blackSkin = new Skin({ fill:"black" });
const whiteSkin = new Skin({ fill:"white" });
const textStyle = new Style({ font:"myFont", left:10, right:10, top:15, bottom:15 });
const centerStyle = new Style({ horizontal:"center" });
const leftStyle = new Style({ horizontal:"left" });
const rightStyle = new Style({ horizontal:"right" });
const justifyStyle = new Style({ horizontal:"justify" });
const absoluteLeadingStyle = new Style({ horizontal:"left", leading:30 });
const relativeLeadingStyle = new Style({ horizontal:"left", leading:-80 });
const otherStyle = new Style({ font:"OpenSans-Semibold-28", color:["blue","red"] });
const redStyle = new Style({ color:["red","blue"] });

const styles = [ centerStyle, leftStyle, rightStyle, justifyStyle, absoluteLeadingStyle, relativeLeadingStyle ];

class LinkBehavior extends Behavior {
	onCreate(link, $) {
		this.$ = $;
	}
	onTouchBegan(link) {
		link.state = 1;
	}
	onTouchEnded(link) {
		link.state = 0;
		trace(this.$ + "\n");
	}
};

class TextBehavior extends Behavior {
	onDisplaying(text) {
		this.index = 0;
		text.duration = 1000;
		text.interval = 1000;
		text.start();
	}
	onFinished(text) {
		let index = this.index + 1;
		if (index >= styles.length) index = 0;
		this.index = index;
		text.style = styles[index];
	
		text.time = 0;
		text.start();
	}
};

let TestApplication = Application.template($ => ({
	skin:blackSkin, style:textStyle,
	contents: [
		Text($, { 
			left:0, right:0, top:0, skin:whiteSkin, style:centerStyle, Behavior:TextBehavior, active:true,
			blocks: [
				{ spans: [
					{ style:redStyle, spans: "Lorem ipsum", link:Link("tutu", { Behavior:LinkBehavior }) },
					" dolor sit amet, consectetur adipiscing elit. ",
					{ style:otherStyle, spans: "Nulla", link:Link("toto", { Behavior:LinkBehavior }) },
					" faucibus sodales ligula eu accumsan."
				]},
				{ spans: [
					"Aliquam consectetur\neleifend",
					{ style:redStyle, link:Link("titi", { Behavior:LinkBehavior }), spans: [
						" molestie. ",
						{ style:otherStyle, spans: "Sed" },
						" dui est, ",
						"suscipit vitae"
					]},
					" consequat a, aliquam eget nisl."
				]},
			]
		}),
	]
}));

export default new TestApplication(null, { displayListLength:4608 });
