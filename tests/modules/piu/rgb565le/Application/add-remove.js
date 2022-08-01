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

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ redSquare, yellowSquare, blueSquare ]
});

screen.checkImage("713a020343f665de3e0d40c69637d7a2");

application.remove(yellowSquare);
screen.checkImage("dbc5b5c73ab256a2680366008fbad163");

application.remove(blueSquare);
screen.checkImage("48a415682ff21704cad15609593d2c86");

application.remove(redSquare);
screen.checkImage("3028cca0e0b447b31496a4603a35bc60");

application.add(yellowSquare);
screen.checkImage("922b5ec8afa5dd26a8f512da399c6d2a");

application.add(blueSquare);
screen.checkImage("dbc5b5c73ab256a2680366008fbad163");

application.add(redSquare);
screen.checkImage("48a415682ff21704cad15609593d2c86");