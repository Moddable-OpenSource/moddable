/*---
description: 
flags: [onlyStrict]
---*/

let sampleStyle = new Style({ font:"16px Open Sans", color: "black", horizontal: "left" });
let redStyle = new Style({ color: "red" });
let centerStyle = new Style({ horizontal: "center" });
let blueRightStyle = new Style({ color: "blue", horizontal: "right" });

let sampleText = new Text(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	style: sampleStyle,
	blocks: [
		{ spans: [
			"Lorem ipsum dolor ",
			{ style: redStyle, spans: "sit amet, consectetur" },
			" adipiscing elit."
		]},
		{ style: centerStyle, spans: "In dignissim " },
		{ style: blueRightStyle, spans: "hendrerit ultricies." }
	]
});

new Application(null, {
	displayListLength: 3000,
	skin: new Skin({ fill: "white" }),
    contents: [ sampleText ]
});
screen.checkImage("9a2e61a99e34e39ffe2ef8ad0571bfb0");