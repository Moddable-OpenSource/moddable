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
	skin: new Skin({ fill: "white" })
});

new Application(null, { contents: [col] });

col.add(redSquare);
col.add(yellowSquare);
col.add(blueSquare);
screen.checkImage("1ba9f0ea204606dee42e3fd8579153ab");

col.remove(yellowSquare);
screen.checkImage("528bee463ceef12591e3c6378540818f");

col.remove(blueSquare);
screen.checkImage("12b6c910baa7363631ad4b692c8ae39c");

col.remove(redSquare);
screen.checkImage("f29d3d7c7bace5943c2e44d85267b5d3");

col.add(yellowSquare);
col.add(blueSquare);
col.add(redSquare);
screen.checkImage("4feef69a7f7eaa93a8fae5a4f0a06f9e");