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

screen.checkImage("8539c4d17b2e12949a49468f7a6dbf10");

// TO DO: should other `Style` properties (e.g. `horizontal`, `leading`) be tested? I'm guessing they don't make a difference so probably not