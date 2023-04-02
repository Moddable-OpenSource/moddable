/*---
description: 
flags: [onlyStrict]
---*/

const ColoredSquare = Content.template($ => ({
	height: 0, width: 0,
	skin: new Skin({ fill: $ }),
}));

class LayoutBehavior extends $TESTMC.Behavior {
	onMeasureHorizontally(layout, width) {
		let squareWidth = width/2;
		let square = layout.first;
		while (square) {
			square.sizeBy(squareWidth-square.width, 0);
			square = square.next;
		}
		return width;
	}
	onMeasureVertically(layout, height) {
		let squareHeight = height/2;
		let square = layout.first;
		while (square) {
			square.sizeBy(0, squareHeight-square.height);
			square = square.next;
		}
		return height;
	}
}

const sampleLayout = new Layout(null, {
	width: 240, height: 320,
	skin: new Skin({fill: "black"}),
	contents: [
		new ColoredSquare("red", {top: 0, left: 0}),
		new ColoredSquare("yellow", {top: 0, right: 0}),
		new ColoredSquare("green", {bottom: 0, left: 0}),
		new ColoredSquare("blue", {bottom: 0, right: 0}),
	],
	Behavior: LayoutBehavior
});

new Application(null, { 
	skin: new Skin({fill: "white"}),
	contents: [sampleLayout] 
});
screen.checkImage("66213203e043a729f4c429377459b462");

sampleLayout.width = 100;
screen.checkImage("231ffdc4dfccc96e289b2af12f7bde8b");

sampleLayout.height = 100;
screen.checkImage("6bc1354916fdd97bda81cfe86ec8c87e");