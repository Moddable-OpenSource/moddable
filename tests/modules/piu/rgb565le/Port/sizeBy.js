/*---
description: 
flags: [onlyStrict]
---*/

const unconstrainedContent = new Port(null, { 
    top: 0, left: 0, height: 100, width: 100,
    skin: new Skin({fill: "blue"}),
});

const constrainedContent = new Port(null, { 
    top: 100, left: 100, bottom: 120, right: 40,
    skin: new Skin({fill: "red"}),
});

new Application(null, {
    skin: new Skin({ fill: "white" }),
    contents: [ unconstrainedContent, constrainedContent ]
});
screen.checkImage("d235f0a3214b775d084b4e5d3427c7ff");

unconstrainedContent.sizeBy(100,100);
screen.checkImage("8d9f73b7a6ead542e5c21b087bc0d85d");
constrainedContent.sizeBy(100,100);	// Does nothing
screen.checkImage(undefined);