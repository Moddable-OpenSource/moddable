/*---
description: 
flags: [onlyStrict]
---*/

let data = {};

new Application(data, { anchor: "ANCHORABLE_CONTENT" })
assert.sameValue(data["ANCHORABLE_CONTENT"], application, "Application should have an anchor");