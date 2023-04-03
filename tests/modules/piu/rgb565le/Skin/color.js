/*---
description: 
flags: [onlyStrict]
---*/

const redSkin = new Skin({ fill: "red" });
const yellowBlueSkin = new Skin({ fill: ["yellow", "blue"] });
const leftLineSkin = new Skin({ fill: "black", stroke: "red", borders: { left: 10 } });
const rightLineSkin = new Skin({ fill: "black", stroke: "yellow", borders: { right: 10 } });
const topLineSkin = new Skin({ fill: "black", stroke: "green", borders: { top: 10 } });
const bottomLineSkin = new Skin({ fill: "black", stroke: "blue", borders: { bottom: 10 } });
const fullOutlineSkin = new Skin({ fill: "black", stroke: "red", borders: { left: 10, right: 10, top: 10, bottom: 10 } });

const coloredContent = new Content(null, { height: 100, width: 100, skin: redSkin });

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: coloredContent
});
screen.checkImage("e2af99ea42c6356c98bced63d96c75c7");

coloredContent.skin = yellowBlueSkin;
screen.checkImage("122f053c7f4ef57c8333e111754b8fb9");
coloredContent.state = 0.5;
screen.checkImage("ba9a32f96bf86e5e351650f9a239ae1a");
coloredContent.state = 1;
screen.checkImage("2b75c9d610c4fbf65c161caa9da4aa93");

coloredContent.skin = leftLineSkin;
screen.checkImage("1ba217d09b6c7ee6472bed4d1bf33735");

coloredContent.skin = rightLineSkin;
screen.checkImage("9782cf6f6b9b21e1495f4cdf5e7c27c4");

coloredContent.skin = topLineSkin;
screen.checkImage("d74722d4a49ca2240218937233e9ed7f");

coloredContent.skin = bottomLineSkin;
screen.checkImage("7c77b91b390f8b4761f2ab366bfe35a4");

coloredContent.skin = fullOutlineSkin;
screen.checkImage("c5029bf7e14efc252250b2a892a6a16a");
