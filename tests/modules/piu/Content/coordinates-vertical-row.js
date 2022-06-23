/*---
description: 
flags: [onlyStrict]
---*/

/*
From https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md#coordinates

When a content's container is a row object:
   left and right coordinates are relative to their previous and next properties
 * top and bottom coordinates are relative to their container
   If height, top, and bottom coordinates are all specified, the height will be overruled
*/

const fixedWidth = 100;
const fixedHeight = 100;

const fixedSizeContent1 = new Content(null, { left: 0, width: fixedWidth, top: 0, height: fixedHeight });
const fixedSizeContent2 = new Content(null, { left: 0, width: fixedWidth, top: 0, height: fixedHeight });
const relativeSizeContent = new Content(null, { left: 0, right: 0, top: 0, bottom: 0 });

const relativeSizeRow = new Row(null, {
    top: 0, bottom: 0, left: 0, right: 0,
    contents: [
        fixedSizeContent1,
        relativeSizeContent,
        fixedSizeContent2
    ]
});

new Application(null, { contents: [relativeSizeRow] });

assert.sameValue(fixedSizeContent1.height, fixedHeight, `fixedSizeContent1 height should be ${fixedHeight}`);
assert.sameValue(fixedSizeContent2.height, fixedHeight, `fixedSizeContent2 height should be ${fixedHeight}`);
assert.sameValue(relativeSizeContent.height, relativeSizeRow.height, `relativeSizeContent height should be ${relativeSizeRow.height}`);
assert.sameValue(fixedSizeContent1.y, 0, `fixedSizeContent1 y should be 0`);
assert.sameValue(fixedSizeContent2.y, 0, `fixedSizeContent2 y should be 0`);
assert.sameValue(relativeSizeContent.y, 0, `relativeSizeContent y should be 0`);

// Change height of fixed size contents in row
// This should have no effect on height/y coordinates of other contents in row
fixedSizeContent1.height = 0;
assert.sameValue(fixedSizeContent1.height, 0, `fixedSizeContent1 height should be 0`);
assert.sameValue(fixedSizeContent2.height, fixedHeight, `fixedSizeContent2 height should be ${fixedHeight}`);
assert.sameValue(relativeSizeContent.height, relativeSizeRow.height, `relativeSizeContent height should be ${relativeSizeRow.height}`);
assert.sameValue(fixedSizeContent1.y, 0, `fixedSizeContent1 y should be 0`);
assert.sameValue(fixedSizeContent2.y, 0, `fixedSizeContent2 y should be 0`);
assert.sameValue(relativeSizeContent.y, 0, `relativeSizeContent y should be 0`);

fixedSizeContent2.height = 0;
assert.sameValue(fixedSizeContent1.height, 0, `fixedSizeContent1 height should be 0`);
assert.sameValue(fixedSizeContent2.height, 0, `fixedSizeContent2 height should be 0`);
assert.sameValue(relativeSizeContent.height, relativeSizeRow.height, `relativeSizeContent height should be ${relativeSizeRow.height}`);
assert.sameValue(fixedSizeContent1.y, 0, `fixedSizeContent1 y should be 0`);
assert.sameValue(fixedSizeContent2.y, 0, `fixedSizeContent2 y should be 0`);
assert.sameValue(relativeSizeContent.y, 0, `relativeSizeContent y should be 0`);