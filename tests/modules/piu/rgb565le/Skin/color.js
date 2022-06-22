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
screen.checkImage("b2342b9d128b17b544c8a1e7c4ff652d");

coloredContent.skin = yellowBlueSkin;
screen.checkImage("d44667614bc9e548158a618772fd2f6b");
coloredContent.state = 0.5;
screen.checkImage("de828f8c1a0914087bfd71deb774ab91");
coloredContent.state = 1;
screen.checkImage("141cf1bc835257c9d536611dee810a64");

coloredContent.skin = leftLineSkin;
screen.checkImage("59c41005a2fa5b72d2aa036330228ce9");

coloredContent.skin = rightLineSkin;
screen.checkImage("d4f4c64424263e265193465271173f42");

coloredContent.skin = topLineSkin;
screen.checkImage("1874cc0291da6fa7732e1cc646050d57");

coloredContent.skin = bottomLineSkin;
screen.checkImage("7aae357b48566774829e9b77667f8cbf");

coloredContent.skin = fullOutlineSkin;
screen.checkImage("cf36005cbf294cac5d27dce0910eebdb");
