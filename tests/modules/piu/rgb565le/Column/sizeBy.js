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

const col = new Column(null, {
	top: 0, height: 320, left: 0, width: 240,
	skin: new Skin({ fill: "white" }),
	contents: [ redSquare, yellowSquare, blueSquare ]
});

new Application(null, { skin: new Skin({ fill: "black" }), contents: [col] });

screen.checkImage("9348fa0dc15709afabf2f80ac9b93205");

col.sizeBy(-100, -100);
screen.checkImage("aa667ead353e2bac49637a16bf09c264");

col.sizeBy(50, 50);
screen.checkImage("fb6656cc60b091b61ed7e994e1e07174");