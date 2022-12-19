/*---
description: 
flags: [onlyStrict]
---*/

const unconstrainedContent = new Image(null, ({
    path: "screen2.cs",
    top: 0, left: 0
}));

const constrainedContent = new Image(null, ({
    path: "screen2.cs"
}));

new Application(null, {
    skin: new Skin({ fill: "white" }),
    contents: [ unconstrainedContent, constrainedContent ]
});
screen.checkImage("ff0b1e62ffde210504996a9ae6ee7435");

unconstrainedContent.moveBy(10,10);
screen.checkImage("cc712649d0e9a60071b090744571130d");
constrainedContent.moveBy(10,10);	// Does nothing
screen.checkImage(undefined);
