/*---
description: 
flags: [onlyStrict]
---*/

let sampleStyle = new Style({ font:"16px Open Sans", color: "blue", horizontal: "left" });

const samplePort = new Port(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	style: sampleStyle,
	Behavior: class extends Behavior {
		onDraw(port) {
			port.drawLabel("Lorem ipsum", 0, 0, port.width, 30);
			let string = "dolor sit amet, consectetur";
			let size = sampleStyle.measure(string);
			port.drawLabel(string, port.width-size.width, port.height-size.height, size.width, size.height);
		}
	}
})

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ samplePort ]
});

screen.checkImage("23010a385a1dece7d04f38901d91c994");