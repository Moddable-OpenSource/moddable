/*---
description: 
flags: [onlyStrict]
---*/

/*
From https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md#coordinates

When a content's container is an application, container, scroller, or layout object:
   top, bottom, left, and right coordinates are all relative to their container
* If width, left, and right coordinates are all specified, the left and right coordinates will be overruled
   If left and right are both unspecified, the content will be centered horizontally in its container with the width specified (or a width of 0, if unspecified)
* If height, top, and bottom coordinates are all specified, the top and bottom coordinates will be overruled
   If top and bottom are both unspecified, the content will be centered vertically in its container with the height specified (or a height of 0, if unspecified)
*/

const fixedSize = 100;

const fixedHeightContent = new Content(null, { height: fixedSize, top: 0, bottom: 0 });
const fixedWidthContent = new Content(null, { width: fixedSize, left: 0, right: 0 });
const fixedHeightWidthContent = new Content(null, { height: fixedSize, top: 0, bottom: 0, width: fixedSize, left: 0, right: 0 });

new Application(null, { contents: [fixedHeightContent, fixedWidthContent] });

assert.sameValue(fixedHeightContent.height, fixedSize, `height property should overrule top/bottom properties`);
assert.sameValue(fixedWidthContent.width, fixedSize, `width property should overrule left/right properties`);
assert.sameValue(fixedHeightWidthContent.height, fixedSize, `height property should overrule top/bottom properties`);
assert.sameValue(fixedHeightWidthContent.width, fixedSize, `width property should overrule left/right properties`);

application.empty();

const containers = [
    new Container(null, { top: 0, bottom: 0, left: 0, right: 0 }),
    new Layout(null, { top: 0, bottom: 0, left: 0, right: 0 }),
    new Scroller(null, { top: 0, bottom: 0, left: 0, right: 0 }),
    new Container(null, { top: 0, height: 300, left: 0, width: 300 }),
    new Layout(null, { top: 0, height: 300, left: 0, width: 300 }),
    new Scroller(null, { top: 0, height: 300, left: 0, width: 300 })
]

for (let container of containers) {
    container.add(fixedHeightContent);
    container.add(fixedWidthContent);
    container.add(fixedHeightWidthContent);
    application.add(container);
    assert.sameValue(fixedHeightContent.height, fixedSize, `height property should overrule top/bottom properties`);
    assert.sameValue(fixedWidthContent.width, fixedSize, `width property should overrule left/right properties`);
    assert.sameValue(fixedHeightWidthContent.height, fixedSize, `height property should overrule top/bottom properties`);
    assert.sameValue(fixedHeightWidthContent.width, fixedSize, `width property should overrule left/right properties`);
    container.empty();
    application.remove(container);
}