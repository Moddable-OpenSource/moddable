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
screen.checkImage("09e9d41110204beebbb04f83a1f4c4d8");

row.remove(yellowSquare);
screen.checkImage("60a96ce11975686825e1ee1d2be75ef4");

row.remove(blueSquare);
screen.checkImage("8d615e72ef34da297ccb1581fcc463da");

row.remove(redSquare);
screen.checkImage("14e6edc6291901a47e0268a966b4d99e");

row.add(yellowSquare);
row.add(blueSquare);
row.add(redSquare);
screen.checkImage("3fcaf1c292bac116db64eecb4aa04eb8");