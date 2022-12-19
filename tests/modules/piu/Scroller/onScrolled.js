/*---
description: 
flags: [onlyStrict, async]
---*/

let count = 0;

class ScrollBehavior extends $TESTMC.Behavior {
	onScrolled(content) {
		count++;
		if (content == container) {
			assert.sameValue(count, 2, "`onScrolled` should have been triggered 3 times");
			$DONE();
		}
	}
}

const content = new Content(null, { left: 0, top: 0, height: 320, width: 240, Behavior: ScrollBehavior });
const content2 = new Content(null, { left: 0, top: 320, height: 320, width: 240 });
const container = new Container(null, { top: 0, left: 0, contents: [content, content2], Behavior: ScrollBehavior });

const scroller = new Scroller(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	contents: [ container ],
	Behavior: ScrollBehavior
});

new Application(null, { 
	contents: [ scroller ] 
});

scroller.scrollTo(1, 1);

$TESTMC.timeout(50, "`onScrolled` should have been triggered");