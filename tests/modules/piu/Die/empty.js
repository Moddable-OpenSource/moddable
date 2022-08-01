/*---
description: 
flags: [onlyStrict]
---*/

const container = new Die(null, {
	contents: [
		new Content
	]
});

assert.sameValue(container.empty(), container, `empty should return container`);
