/*---
description: 
flags: [onlyStrict]
---*/

/*
From https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md#coordinates

When a content's container is a column object:
   top and bottom coordinates are relative to their previous and next properties
 * left and right coordinates are relative to their container
   If width, left, and right coordinates are all specified, the width will be overruled

*/

const fixedWidth = 100;
const fixedHeight = 100;

const fixedSizeContent1 = new Content(null, { left: 0, width: fixedWidth, top: 0, height: fixedHeight });
const fixedSizeContent2 = new Content(null, { left: 0, width: fixedWidth, top: 0, height: fixedHeight });
const relativeSizeContent = new Content(null, { left: 0, right: 0, top: 0, bottom: 0 });

const absoluteSizeCol = new Column(null, {
    top: 0, height: 300, left: 0, width: 300,
    contents: [
        fixedSizeContent1,
        relativeSizeContent,
        fixedSizeContent2
    ]
});

new Application(null, { contents: [absoluteSizeCol] });

assert.sameValue(fixedSizeContent1.width, fixedWidth, `fixedSizeContent1 width should be ${fixedWidth}`);
assert.sameValue(fixedSizeContent2.width, fixedWidth, `fixedSizeContent2 width should be ${fixedWidth}`);
assert.sameValue(relativeSizeContent.width, absoluteSizeCol.width, `relativeSizeContent width should be ${absoluteSizeCol.width}`);
assert.sameValue(fixedSizeContent1.x, 0, `fixedSizeContent1 x should be 0`);
assert.sameValue(fixedSizeContent2.x, 0, `fixedSizeContent2 x should be 0`);
assert.sameValue(relativeSizeContent.x, 0, `relativeSizeContent x should be 0`);

// Change width of fixed size contents in column
// This should have no effect on width/x coordinates of other contents in column
fixedSizeContent1.width = 0;
assert.sameValue(fixedSizeContent1.width, 0, `fixedSizeContent1 width should be 0`);
assert.sameValue(fixedSizeContent2.width, fixedWidth, `fixedSizeContent2 width should be ${fixedWidth}`);
assert.sameValue(relativeSizeContent.width, absoluteSizeCol.width, `relativeSizeContent width should be ${absoluteSizeCol.width}`);
assert.sameValue(fixedSizeContent1.x, 0, `fixedSizeContent1 x should be 0`);
assert.sameValue(fixedSizeContent2.x, 0, `fixedSizeContent2 x should be 0`);
assert.sameValue(relativeSizeContent.x, 0, `relativeSizeContent x should be 0`);

fixedSizeContent2.width = 0;
assert.sameValue(fixedSizeContent1.width, 0, `fixedSizeContent1 width should be 0`);
assert.sameValue(fixedSizeContent2.width, 0, `fixedSizeContent2 width should be 0`);
assert.sameValue(relativeSizeContent.width, absoluteSizeCol.width, `relativeSizeContent width should be ${absoluteSizeCol.width}`);
assert.sameValue(fixedSizeContent1.x, 0, `fixedSizeContent1 x should be 0`);
assert.sameValue(fixedSizeContent2.x, 0, `fixedSizeContent2 x should be 0`);
assert.sameValue(relativeSizeContent.x, 0, `relativeSizeContent x should be 0`);

// Change width/left margin of column itself
// This should have an effect on width/x coordinates of contents in column
absoluteSizeCol.width = fixedWidth;
assert.sameValue(fixedSizeContent1.width, 0, `fixedSizeContent1 width should be 0`);
assert.sameValue(fixedSizeContent2.width, 0, `fixedSizeContent2 width should be 0`);
assert.sameValue(relativeSizeContent.width, fixedWidth, `relativeSizeContent width should be ${fixedWidth}`);
assert.sameValue(fixedSizeContent1.x, 0, `fixedSizeContent1 x should be 0`);
assert.sameValue(fixedSizeContent2.x, 0, `fixedSizeContent2 x should be 0`);
assert.sameValue(relativeSizeContent.x, 0, `relativeSizeContent x should be 0`);

absoluteSizeCol.moveBy(100, 0);
assert.sameValue(fixedSizeContent1.width, 0, `fixedSizeContent1 width should be 0`);
assert.sameValue(fixedSizeContent2.width, 0, `fixedSizeContent2 width should be 0`);
assert.sameValue(relativeSizeContent.width, fixedWidth, `relativeSizeContent width should be ${fixedWidth}`);
assert.sameValue(fixedSizeContent1.x, 100, `fixedSizeContent1 x should be 100`);
assert.sameValue(fixedSizeContent2.x, 100, `fixedSizeContent2 x should be 100`);
assert.sameValue(relativeSizeContent.x, 100, `relativeSizeContent x should be 100`);