/*---
description: 
flags: [onlyStrict]
---*/

/*
From https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md#coordinates

When a content's container is a column object:
   top and bottom coordinates are relative to their previous and next properties
   left and right coordinates are relative to their container
 * If width, left, and right coordinates are all specified, the left and right coordinates will be overruled
*/

const relativeSizeCol = new Column(null, {
    top: 0, bottom: 0, left: 0, right: 0,
    contents: [
        new Content(null, { width: 100, left: 0, right: 0 })
    ]
});

const absoluteSizeCol = new Column(null, {
    top: 0, height: 300, left: 0, width: 300,
    contents: [
        new Content(null, { width: 100, left: 0, right: 0 })
    ]
});

new Application(null, { contents: [relativeSizeCol, absoluteSizeCol] });

assert.sameValue(relativeSizeCol.first.width, 100, `content width should overrule left and right coordinates`);
assert.sameValue(absoluteSizeCol.first.width, 100, `content width should overrule left and right coordinates`);

