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

screen.checkImage("1ba9f0ea204606dee42e3fd8579153ab");

col.swap(redSquare, blueSquare);
screen.checkImage("effdec310e249c9686719172a8bcefe2");

col.swap(redSquare, yellowSquare);
screen.checkImage("79848c38bbbe2b134684f316c21dbf21");
