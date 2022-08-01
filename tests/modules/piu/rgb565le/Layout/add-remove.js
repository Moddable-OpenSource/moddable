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

const container = new Layout(null, { 
	left: 0, right: 0, top: 0, bottom: 0,
	contents: [ redSquare, yellowSquare, blueSquare ]
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ container ]
});

screen.checkImage("713a020343f665de3e0d40c69637d7a2");

container.remove(yellowSquare);
screen.checkImage("dbc5b5c73ab256a2680366008fbad163");

container.remove(blueSquare);
screen.checkImage("48a415682ff21704cad15609593d2c86");

container.remove(redSquare);
screen.checkImage("3028cca0e0b447b31496a4603a35bc60");

container.add(yellowSquare);
screen.checkImage("922b5ec8afa5dd26a8f512da399c6d2a");

container.add(blueSquare);
screen.checkImage("dbc5b5c73ab256a2680366008fbad163");

container.add(redSquare);
screen.checkImage("48a415682ff21704cad15609593d2c86");