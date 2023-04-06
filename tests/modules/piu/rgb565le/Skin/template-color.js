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
screen.checkImage("82afd81be64e948dd85f671adbef88d1");
