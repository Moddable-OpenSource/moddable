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
screen.checkImage("985606a9e36a608e7d51f25b829a08a8");

let timeline = new Timeline();
timeline.to(redBlueSquare, { x: 160, state: 1 }, 100, null, 0);

timeline.seekTo(50);
screen.checkImage("45e25695a789657d6abceee88fa69b16");

timeline.seekTo(100);
screen.checkImage("61d6c43f0f47ce05e6b8e39420fdda5b");