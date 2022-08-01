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

const redSquare = new Content(null, { 
	left: 0, top: 0, width: 80, height: 80,
	skin: new Skin({ fill: "red" })
});
application.add(redSquare);
screen.checkImage("968025f9e885cc809dfaa88ad8b354ed");

let timeline = new Timeline();
timeline.on(redSquare, { x: [0, 160, 160, 0, 0], y: [0, 0, 160, 160, 0] }, 400, null, 0);

timeline.seekTo(100);
screen.checkImage("8542ce81489c2d7136757496860c4707");

timeline.seekTo(200);
screen.checkImage("2db45c784aa67e5ae563c030b2a080ea");

timeline.seekTo(300);
screen.checkImage("8cc31b12e7b6cc86260de61fb975a0ae");

timeline.seekTo(400);
screen.checkImage("2116db69ca4ebd87d2effb2a43527df5");