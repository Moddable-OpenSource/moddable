/*---
description: 
flags: [onlyStrict]
---*/

const scroller = new Scroller(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	contents: [
        Container(null, {
        	top: 0, left: 0,
        	contents: [
		        new Content(null, { left: 0, top: 0, height: 320, width: 240 }),
				new Content(null, { left: 0, top: 320, height: 320, width: 120 }),
				new Content(null, { left: 240, top: 0, height: 160, width: 240 }),
        	]
        })
	]
});

new Application(null, { 
	contents: [ scroller ] 
});

scroller.scrollTo(240, 0);
assert.sameValue(scroller.scroll.x, 240, "scroll x should be 240");
assert.sameValue(scroller.scroll.y, 0, "scroll y should be 0");

scroller.scrollBy(-240, 320);
assert.sameValue(scroller.scroll.x, 0, "scroll x should be 0");
assert.sameValue(scroller.scroll.y, 320, "scroll y should be 320");

scroller.scrollTo(0, 0);
assert.sameValue(scroller.scroll.x, 0, "scroll x should be 0");
assert.sameValue(scroller.scroll.y, 0, "scroll y should be 0");

scroller.scrollTo(1000, 1000);
assert.sameValue(scroller.scroll.x, 1000, "scroll x should be 1000");
assert.sameValue(scroller.scroll.y, 1000, "scroll y should be 1000");

scroller.scrollTo(-100, -100);
assert.sameValue(scroller.scroll.x, -100, "scroll x should be -100");
assert.sameValue(scroller.scroll.y, -100, "scroll y should be -100");
