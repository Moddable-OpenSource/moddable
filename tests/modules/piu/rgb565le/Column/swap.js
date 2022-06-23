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

const col = new Column(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	skin: new Skin({ fill: "white" }),
	contents: [ redSquare, yellowSquare, blueSquare ]
});

new Application(null, { contents: [col] });

screen.checkImage("279bb31cdc6cfe54ca3022e7da2c781e");

col.swap(redSquare, blueSquare);
screen.checkImage("fd7ba2d1c7926331c513eebd8fdfe9b7");

col.swap(redSquare, yellowSquare);
screen.checkImage("0e2331f2483d8379f7133e60f00d6dde");
