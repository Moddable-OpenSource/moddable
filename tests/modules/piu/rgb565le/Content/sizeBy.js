/*---
description: 
flags: [onlyStrict]
---*/

const unconstrainedContent = new Content(null, { 
    top: 0, left: 0, height: 100, width: 100,
    skin: new Skin({fill: "blue"}),
});

const constrainedContent = new Content(null, { 
    top: 100, left: 100, bottom: 120, right: 40,
    skin: new Skin({fill: "red"}),
});

new Application(null, {
    skin: new Skin({ fill: "white" }),
    contents: [ unconstrainedContent, constrainedContent ]
});
screen.checkImage("b5589673cb4f66d22b579bf0543d4419");

unconstrainedContent.sizeBy(100,100);
screen.checkImage("65a2bb7977ad86949a182ade90539594");
constrainedContent.sizeBy(100,100);	// Does nothing
screen.checkImage(undefined);