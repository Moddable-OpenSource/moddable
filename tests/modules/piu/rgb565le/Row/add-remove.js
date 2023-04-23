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
	skin: new Skin({ fill: "white" })
});

new Application(null, { contents: [row] });

row.add(redSquare);
row.add(yellowSquare);
row.add(blueSquare);
screen.checkImage("ceaea8935eab9131dda6804c99d97563");

row.remove(yellowSquare);
screen.checkImage("44b8d6361542e512a857c9769380deb5");

row.remove(blueSquare);
screen.checkImage("12b6c910baa7363631ad4b692c8ae39c");

row.remove(redSquare);
screen.checkImage("f29d3d7c7bace5943c2e44d85267b5d3");

row.add(yellowSquare);
row.add(blueSquare);
row.add(redSquare);
screen.checkImage("28ee801805ba99b55aca0af88f25d8a0");