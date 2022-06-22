/*---
description: 
flags: [onlyStrict]
---*/

let data = {};

const ContentTemplate = Application.template($ => ({ anchor: "ANCHORABLE_CONTENT" }));
new ContentTemplate(data);
assert.sameValue(data["ANCHORABLE_CONTENT"], application, "Application should have an anchor");