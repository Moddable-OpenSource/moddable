/*---
description: 
flags: [onlyStrict]
---*/

let data = {};

const ContentTemplate = Application.template($ => ({}));
new ContentTemplate(data, { anchor: "ANCHORABLE_CONTENT" });
assert.sameValue(data["ANCHORABLE_CONTENT"], application, "Application should have an anchor");
