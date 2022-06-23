/*---
description: 
flags: [onlyStrict]
---*/

const ContentTemplate = Image.template($ => ({
    path: $,
}));

const content = new ContentTemplate("screen2.cs", { left: 0, top: 20 });

new Application(null, { skin: new Skin({ fill: "green" }), contents: [content] });

screen.checkImage("55c7bdaf2370545885019ebe4ed10374");
