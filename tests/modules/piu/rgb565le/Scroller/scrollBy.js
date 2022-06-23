/*---
description: 
flags: [onlyStrict]
---*/

const scroller = new Scroller(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	contents: [
        Container(null, {
        	top: 0, left: 0,
        	contents: [
		        new Content(null, { left: 0, top: 0, height: 320, width: 240, skin: new Skin({ fill: "red" }) }),
				new Content(null, { left: 0, top: 320, height: 320, width: 120, skin: new Skin({ fill: "yellow" }) }),
				new Content(null, { left: 240, top: 0, height: 160, width: 240, skin: new Skin({ fill: "blue" }) }),
        	]
        })
	]
});

new Application(null, { 
	skin: new Skin({ fill: "white" }),
	contents: [ scroller ] 
});
screen.checkImage("73328e0ea82021eb0cef0ac14a7832ee");

scroller.scrollBy(240, 0);
screen.checkImage("816483fd8f40eafa124399fdf6c4a2f6");

scroller.scrollBy(-240, 320);
screen.checkImage("62b0f36094da0fed17672454d4a15d64");

scroller.scrollBy(0, -320);
screen.checkImage("73328e0ea82021eb0cef0ac14a7832ee");