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

const relativeSizeCol = new Column(null, {
    top: 0, bottom: 0, left: 0, right: 0,
    contents: [
        fixedSizeContent1,
        relativeSizeContent,
        fixedSizeContent2
    ]
});

new Application(null, { contents: [relativeSizeCol] });

assert.sameValue(fixedSizeContent1.width, fixedWidth, `fixedSizeContent1 width should be ${fixedWidth}`);
assert.sameValue(fixedSizeContent2.width, fixedWidth, `fixedSizeContent2 width should be ${fixedWidth}`);
assert.sameValue(relativeSizeContent.width, relativeSizeCol.width, `relativeSizeContent width should be ${relativeSizeCol.width}`);
assert.sameValue(fixedSizeContent1.x, 0, `fixedSizeContent1 x should be 0`);
assert.sameValue(fixedSizeContent2.x, 0, `fixedSizeContent2 x should be 0`);
assert.sameValue(relativeSizeContent.x, 0, `relativeSizeContent x should be 0`);

// Change width of fixed size contents in column
// This should have no effect on width/x coordinates of other contents in column
fixedSizeContent1.width = 0;
assert.sameValue(fixedSizeContent1.width, 0, `fixedSizeContent1 width should be 0`);
assert.sameValue(fixedSizeContent2.width, fixedWidth, `fixedSizeContent2 width should be ${fixedWidth}`);
assert.sameValue(relativeSizeContent.width, relativeSizeCol.width, `relativeSizeContent width should be ${relativeSizeCol.width}`);
assert.sameValue(fixedSizeContent1.x, 0, `fixedSizeContent1 x should be 0`);
assert.sameValue(fixedSizeContent2.x, 0, `fixedSizeContent2 x should be 0`);
assert.sameValue(relativeSizeContent.x, 0, `relativeSizeContent x should be 0`);

fixedSizeContent2.width = 0;
assert.sameValue(fixedSizeContent1.width, 0, `fixedSizeContent1 width should be 0`);
assert.sameValue(fixedSizeContent2.width, 0, `fixedSizeContent2 width should be 0`);
assert.sameValue(relativeSizeContent.width, relativeSizeCol.width, `relativeSizeContent width should be ${relativeSizeCol.width}`);
assert.sameValue(fixedSizeContent1.x, 0, `fixedSizeContent1 x should be 0`);
assert.sameValue(fixedSizeContent2.x, 0, `fixedSizeContent2 x should be 0`);
assert.sameValue(relativeSizeContent.x, 0, `relativeSizeContent x should be 0`);