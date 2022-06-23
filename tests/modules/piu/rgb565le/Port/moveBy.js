/*---
description: 
flags: [onlyStrict]
---*/

const unconstrainedContent = new Port(null, { 
    top: 0, left: 0, height: 100, width: 100,
    skin: new Skin({fill: "blue"}),
});

const constrainedContent = new Port(null, { 
    top: 0, left: 0, bottom: 220, right: 140,
    skin: new Skin({fill: "red"}),
});

new Application(null, {
    skin: new Skin({ fill: "white" }),
    contents: [ unconstrainedContent, constrainedContent ]
});
screen.checkImage("db4dc3d8246223a45feb8ee57ac5117d");

unconstrainedContent.moveBy(100,100);
screen.checkImage("e5d0a2aca1c1dbe11aa1e50d6f8f8c1d");
constrainedContent.moveBy(100,100);	// Does nothing
screen.checkImage(undefined);