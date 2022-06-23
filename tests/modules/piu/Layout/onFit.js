/*---
description: 
flags: [onlyStrict, async]
---*/

class LayoutBehavior extends $TESTMC.Behavior {
	onFitHorizontally(layout, width) {
		assert.sameValue(width, 20, "Incorrect `width` argument");
		if (this.verticalDone)
			$DONE();
		else
			this.horizontalDone = true;
		return width;
	}
	onFitVertically(layout, height) {
		assert.sameValue(height, 10, "Incorrect `height` argument");
		if (this.horizontalDone)
			$DONE();
		else
			this.verticalDone = true;
		return height;
	}
}

const sampleLayout = new Layout(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	Behavior: LayoutBehavior
});

const sampleContainer = new Container(null, {
	height: 10, width: 20,
	contents: [ sampleLayout ]
})

new Application(null, { 
	contents: [ sampleContainer ] 
});


$TESTMC.timeout(100, "`onFitHorizontally` and `onFitVertically` should have been triggered");