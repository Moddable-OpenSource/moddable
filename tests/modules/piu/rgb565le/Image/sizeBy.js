/*---
description: 
flags: [onlyStrict]
---*/

const unconstrainedContent = new Image(null, ({
    path: "screen2.cs",
    top: 0, height: 160, left: 0, width: 240
}));

const constrainedContent = new Image(null, ({
    path: "screen2.cs",
    top: 160, bottom: 0, left: 0, right: 0
}));

new Application(null, {
    skin: new Skin({ fill: "white" }),
    contents: [ unconstrainedContent, constrainedContent ]
});
screen.checkImage("48b08adc8af7e9a73605b7374a07e370");

unconstrainedContent.sizeBy(-180,-100);
screen.checkImage("9dd1faf6d7c42281bea6baf749d6fb73");
constrainedContent.sizeBy(-100,-100);	// Does nothing
screen.checkImage(undefined);
