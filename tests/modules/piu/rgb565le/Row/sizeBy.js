/*---
description: 
flags: [onlyStrict]
---*/

const Square = Content.template($ => ({
	top: 10, bottom: 10, left: 10, right: 10,
	skin: new Skin({ fill: $ })
}));

const redSquare = new Square("red");
const yellowSquare = new Square("yellow");
const blueSquare = new Square("blue");

const row = new Row(null, {
	top: 0, height: 320, left: 0, width: 240,
	skin: new Skin({ fill: "white" }),
	contents: [ redSquare, yellowSquare, blueSquare ]
});

new Application(null, { skin: new Skin({ fill: "black" }), contents: [row] });

screen.checkImage("6d7fcc18ace763d7c254b955ddd00b98");

row.sizeBy(-100, -100);
screen.checkImage("cb7c3d074edc3094b503e4150d70de91");

row.sizeBy(50, 50);
screen.checkImage("d5cdd0397159408055c428a84d683ba9");