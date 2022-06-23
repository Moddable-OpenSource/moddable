/*---
description: 
flags: [onlyStrict]
---*/

const invalidPathTypes = [123, {}, [], null, undefined];
for (let path of invalidPathTypes) {
	assert.throws(TypeError, () => {
		let invalid = new Texture({ path });
	}, `Texture constructor should throw an error when invalid path property ${path} is specified`);
}

const invalidPathURIs = ["", "nonexistent.jpg"];
for (let path of invalidPathURIs) {
	assert.throws(URIError, () => {
		let invalid = new Texture({ path });
	}, `Texture constructor should throw an error when invalid path property ${path} is specified`);
}

const circle = new Texture({ path: "circleish.png" });
assert.sameValue(circle.height, 40, `Texture height should be 40`);
assert.sameValue(circle.width, 40, `Texture width should be 40`);