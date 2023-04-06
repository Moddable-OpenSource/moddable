/*---
description: 
flags: [onlyStrict]
---*/

const Square = Content.template($ => ({
	top: 10, left: 10, width: 50, height: 50,
	skin: new Skin({ fill: $ })
}));

const redSquare = new Square("red");
const yellowSquare = new Square("yellow");
const blueSquare = new Square("blue");

const row = new Row(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	skin: new Skin({ fill: "white" }),
	contents: [ redSquare, yellowSquare ]
});

new Application(null, { contents: [row] });

screen.checkImage("ddd1c35bcb968e0b1874435bef0cb891");

row.replace(redSquare, blueSquare);
screen.checkImage("4a3a59e2c1190449bbd3d317010dde8b");

row.replace(yellowSquare, redSquare);
screen.checkImage("2217238d8a298ad0b24b73f5bb966d3a");

row.replace(blueSquare, yellowSquare);
screen.checkImage("c31ad9dd4aab576e6a7edd5597a7a381");