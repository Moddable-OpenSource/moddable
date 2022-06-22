/*---
description: 
flags: [onlyStrict]
---*/

const RedSkin = Skin.template({ fill: "red" });
const YellowBlueSkin = Skin.template({ fill: ["yellow", "blue"] });

new Application(null, {
	Skin: Skin.template({ fill: "white" }),
	contents: [
		new Container(null, { 
			height: 100, width: 100, Skin: RedSkin,
			contents: [
				new Content(null, { height: 50, width: 50, Skin: YellowBlueSkin, state: 1 })
			]
		})
	]
});
screen.checkImage("4cd45f78f21c5af77e1d0a80816fee7a");
