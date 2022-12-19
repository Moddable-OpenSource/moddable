/*---
description: 
flags: [onlyStrict, module]
---*/

import Timeline from "piu/Timeline";

new Application(null, {
	skin: new Skin({ 
		fill: "white"
	})
});

const redBlueSquare = new Content(null, { 
	left: 0, top: 0, width: 80, height: 80,
	skin: new Skin({ fill: ["red", "blue"] })
});
application.add(redBlueSquare);
screen.checkImage("968025f9e885cc809dfaa88ad8b354ed");

let timeline = new Timeline();
timeline.from(redBlueSquare, { x: 160, state: 1 }, 100, null, 0);

timeline.seekTo(0);
screen.checkImage("9f07dd746630e93967acd098fac9a258");

timeline.seekTo(50);
screen.checkImage("659fb968e12737d8792cae2498e6db34");

timeline.seekTo(100);
screen.checkImage("68d6f2c3e9e998e8ae3a50250e91d002");