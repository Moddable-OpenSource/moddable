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
screen.checkImage("1bba9b52874252f7348d1ea13ce5deb9");

unconstrainedContent.sizeBy(-180,-100);
screen.checkImage("2adf0e38d1fd837e6fe4bfb131ce4ff0");
constrainedContent.sizeBy(-100,-100);	// Does nothing
screen.checkImage(undefined);
