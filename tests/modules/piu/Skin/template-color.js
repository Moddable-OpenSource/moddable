/*---
description: 
flags: [onlyStrict]
---*/

const invalidColors = [null, undefined, ""];
for (let fill of invalidColors) {
	assert.throws(Error, () => {
		const SkinTemplate = Skin.template({ fill });
		const invalid = new SkinTemplate;
	}, `Skin constructor should throw an error when invalid fill property ${fill} is specified`);
}
for (let stroke of invalidColors) {
	assert.throws(Error, () => {
		const SkinTemplate = Skin.template({ stroke });
		const invalid = new SkinTemplate;
	}, `Skin constructor should throw an error when invalid stroke property ${stroke} is specified`);
}

const SkinTemplate = Skin.template({ fill: "red" });
const sampleSkin = new SkinTemplate;

const readOnlyProperties = ["fill", "stroke", "borders"];
for (let property of readOnlyProperties) {
	assert.throws(TypeError, () => {
		sampleSkin[property] = 123;
	}, `${property} property of a skin object should be read-only`);
}

assert.sameValue(sampleSkin.fill[0], 0xFF0000FF, "`sampleSkin` fill should be red");
assert.sameValue(sampleSkin.fill[1], 0xFF0000FF, "`sampleSkin` fill should be red");
assert.sameValue(sampleSkin.fill[2], 0xFF0000FF, "`sampleSkin` fill should be red");
assert.sameValue(sampleSkin.fill[3], 0xFF0000FF, "`sampleSkin` fill should be red");