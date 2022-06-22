/*---
description: 
flags: [onlyStrict]
---*/

const invalidPathTypes = [123, {}, [], null, undefined];
for (let path of invalidPathTypes) {
	let BadTemplate = Texture.template({ path });
	assert.throws(TypeError, () => {
		let invalid = new BadTemplate;
	}, `Texture constructor should throw an error when invalid path property ${path} is specified`);
}

const invalidPathURIs = ["", "nonexistent.jpg"];
for (let path of invalidPathURIs) {
	let BadTemplate = Texture.template({ path });
	assert.throws(URIError, () => {
		let invalid = new BadTemplate;
	}, `Texture constructor should throw an error when invalid path property ${path} is specified`);
}

let CircleTemplate = Texture.template({ path: "circleish.png" });
const circle = new CircleTemplate;
assert.sameValue(circle.height, 40, `Texture height should be 40`);
assert.sameValue(circle.width, 40, `Texture width should be 40`);