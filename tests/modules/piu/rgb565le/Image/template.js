/*---
description: 
flags: [onlyStrict]
---*/

const ContentTemplate = Image.template($ => ({
    path: $,
}));

const content = new ContentTemplate("screen2.cs", { left: 0, top: 20 });

new Application(null, { skin: new Skin({ fill: "green" }), contents: [content] });

screen.checkImage("353b836179fc716603900b677e6e31b5");
