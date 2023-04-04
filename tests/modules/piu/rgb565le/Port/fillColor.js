/*---
description: 
flags: [onlyStrict]
---*/

const samplePort = new Port(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	skin: new Skin({fill: "white"}),
	Behavior: class extends Behavior {
		onDraw(port) {
			port.fillColor("blue", 50, 50, port.width-100, port.height-100);
			port.fillColor("red", 0, 0, 20, 20);
		}
	}
})

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ samplePort ]
});

screen.checkImage("8f4e27bb2e671bfb96eceb2674e8f903");