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
screen.checkImage("73d09edd8afd47d9038203b6c975f44c");

scroller.scrollBy(240, 0);
screen.checkImage("55d0b393a70f5c0571c3116cf162d49f");

scroller.scrollBy(-240, 320);
screen.checkImage("5e2573543bdcd9f294fa7911ff8d27ac");

scroller.scrollBy(0, -320);
screen.checkImage("73d09edd8afd47d9038203b6c975f44c");