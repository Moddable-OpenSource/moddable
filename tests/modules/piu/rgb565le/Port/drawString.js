/*---
description: 
flags: [onlyStrict]
---*/

let sampleStyle = new Style({ font:"16px Open Sans" });

const samplePort = new Port(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	Behavior: class extends Behavior {
		onDraw(port) {
			port.drawString("Lorem ipsum", sampleStyle, "red", 0, 0, port.width, 30);
			port.drawString("Lorem ipsum", sampleStyle, "blue", 0, 30, port.width, 30);
		}
	}
})

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ samplePort ]
});

screen.checkImage("d85a11e508e738cf3f90e7db7fe02c82");

// TO DO: should other `Style` properties (e.g. `horizontal`, `leading`) be tested? I'm guessing they don't make a difference so probably not