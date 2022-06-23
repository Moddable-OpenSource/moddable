/*---
description: 
flags: [onlyStrict]
---*/

/*
From https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md#coordinates

When a content's container is a row object:
   left and right coordinates are relative to their previous and next properties
   top and bottom coordinates are relative to their container
 * If height, top, and bottom coordinates are all specified, the top and bottom coordinates will be overruled
*/

const relativeSizeRow = new Row(null, {
    top: 0, bottom: 0, left: 0, right: 0,
    contents: [
        new Content(null, { height: 100, top: 0, bottom: 0 })
    ]
});

const absoluteSizeRow = new Row(null, {
    top: 0, height: 300, left: 0, width: 300,
    contents: [
        new Content(null, { height: 100, top: 0, bottom: 0 })
    ]
});

new Application(null, { contents: [relativeSizeRow, absoluteSizeRow] });

assert.sameValue(relativeSizeRow.first.height, 100, `content height should overrule top and bottom coordinates`);
assert.sameValue(absoluteSizeRow.first.height, 100, `content height should overrule top and bottom coordinates`);