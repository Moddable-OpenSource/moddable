/*---
description: 
flags: [onlyStrict, async]
---*/

class LayoutBehavior extends $TESTMC.Behavior {
	onMeasureHorizontally(layout, width) {
		assert.sameValue(width, 20, "Incorrect `width` argument");
		if (this.verticalDone)
			$DONE();
		else
			this.horizontalDone = true;
		return width;
	}
	onMeasureVertically(layout, height) {
		assert.sameValue(height, 10, "Incorrect `height` argument");
		if (this.horizontalDone)
			$DONE();
		else
			this.verticalDone = true;
		return height;
	}
}

const sampleLayout = new Layout(null, {
	height: 10, width: 20,
	Behavior: LayoutBehavior
});

new Application(null, { 
	contents: [ sampleLayout ] 
});


$TESTMC.timeout(100, "`onMeasureHorizontally` and `onMeasureVertically` should have been triggered");