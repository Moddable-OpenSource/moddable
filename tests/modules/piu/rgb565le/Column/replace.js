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
	contents: [redSquare, yellowSquare]
});

new Application(null, { contents: [col] });
screen.checkImage("f8c56d97bcf36970357550cf397b89b0");

col.replace(redSquare, blueSquare);
screen.checkImage("7ebf65c2ce4d953cc03187a793caf9cd");

col.replace(yellowSquare, redSquare);
screen.checkImage("e82b1594db1a226b1eb21b7ffd75f364");

col.replace(blueSquare, yellowSquare);
screen.checkImage("0d2de4cf859a9dd4ed02905b5d6859cc");
