/*---
description: 
flags: [onlyStrict]
---*/

const unconstrainedContent = new Content(null, { 
    top: 0, left: 0, height: 100, width: 100,
    skin: new Skin({fill: "blue"}),
});

const constrainedContent = new Content(null, { 
    top: 0, left: 0, bottom: 220, right: 140,
    skin: new Skin({fill: "red"}),
});

new Application(null, {
    skin: new Skin({ fill: "white" }),
    contents: [ unconstrainedContent, constrainedContent ]
});
screen.checkImage("f30e02a81ef52d72e5e3bfcbeaa5d913");

unconstrainedContent.moveBy(100,100);
screen.checkImage("a970d06ba1ddce2d4941a74b373354a2");
constrainedContent.moveBy(100,100);	// Does nothing
screen.checkImage(undefined);