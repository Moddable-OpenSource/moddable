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
screen.checkImage("c2db89cd187ac7335a4741b2ffdb41ca");

unconstrainedContent.moveBy(10,10);
screen.checkImage("ee11158d656e62745c91557bcf73d8a7");
constrainedContent.moveBy(10,10);	// Does nothing
screen.checkImage(undefined);
