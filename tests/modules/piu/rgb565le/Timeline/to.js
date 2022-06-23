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
timeline.to(redBlueSquare, { x: 160, state: 1 }, 100, null, 0);

timeline.seekTo(50);
screen.checkImage("fbb65b1ffb85d7202496668a1beb957c");

timeline.seekTo(100);
screen.checkImage("e97cec263fc0cda96595808b0a255da0");