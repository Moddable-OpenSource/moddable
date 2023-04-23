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
screen.checkImage("985606a9e36a608e7d51f25b829a08a8");

let timeline = new Timeline();
timeline.on(redSquare, { x: [0, 160, 160, 0, 0], y: [0, 0, 160, 160, 0] }, 400, null, 0);

timeline.seekTo(100);
screen.checkImage("dc4747180e8449c7e5839e748755bac6");

timeline.seekTo(200);
screen.checkImage("b87e1f080edc1627004d9ccdfe1370be");

timeline.seekTo(300);
screen.checkImage("ac199d492118495537dac5db5867d77a");

timeline.seekTo(400);
screen.checkImage("37fbd351438871b912c6f340a629b330");