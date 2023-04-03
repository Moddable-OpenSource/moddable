/*---
description: 
flags: [onlyStrict]
---*/

let redCenterStyle = new Style({ font:"16px Open Sans", color: "red", horizontal: "center" });
let multicoloredLeftStyle = new Style({ font:"16px Open Sans", color: ["blue", "red"], horizontal: "left" });

const samplePort = new Port(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	Behavior: class extends Behavior {
		onDraw(port) {
			port.drawStyle("Red centered", redCenterStyle, 0, 0, port.width, 30);
			port.drawStyle("Blue left", multicoloredLeftStyle, 0, 30, port.width, 30);
			port.drawStyle("Purple left", multicoloredLeftStyle, 0, 60, port.width, 30, true, 0.5);
			port.drawStyle("Red left", multicoloredLeftStyle, 0, 90, port.width, 30, true, 1);
			port.drawStyle("This will not be truncated", multicoloredLeftStyle, 0, 120, 100, 30, false);
			port.drawStyle("This will be truncated", multicoloredLeftStyle, 0, 150, 100, 30, true);
		}
	}
})

new Application(null, {
	displayListLength: 3000,
	skin: new Skin({ fill: "white" }),
	contents: [ samplePort ]
});

screen.checkImage("9a4185765e2fb7681426ff750a06b5b0");