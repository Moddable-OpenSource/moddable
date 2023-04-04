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
timeline.from(redBlueSquare, { x: 160, state: 1 }, 100, null, 0);

timeline.seekTo(0);
screen.checkImage("ff0e9f5b646271f87f91292f2e3e3595");

timeline.seekTo(50);
screen.checkImage("67f4e5a0551ba5b8de5dd25f81d2fc57");

timeline.seekTo(100);
screen.checkImage("619e94217f8414b27af47d2d4948753c");