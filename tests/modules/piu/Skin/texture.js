/*---
description: 
flags: [onlyStrict]
---*/

const circleTexture = new Texture({ path: "circleish.png" });

let circleMaskSkin = new Skin({
	texture: circleTexture,
	color: "red"
});
assert.sameValue(circleMaskSkin.texture, circleTexture, "`circleMaskSkin` texture should be `circleTexture`");
assert.sameValue(circleMaskSkin.color[0], 0xFF0000FF, "`circleMaskSkin` color should be red");
assert.sameValue(circleMaskSkin.color[1], 0xFF0000FF, "`circleMaskSkin` color should be red");
assert.sameValue(circleMaskSkin.color[2], 0xFF0000FF, "`circleMaskSkin` color should be red");
assert.sameValue(circleMaskSkin.color[3], 0xFF0000FF, "`circleMaskSkin` color should be red");
assert.sameValue(circleMaskSkin.height, 0, "`circleMaskSkin` height should default to 0");
assert.sameValue(circleMaskSkin.width, 0, "`circleMaskSkin` width should default to 0");

circleMaskSkin = new Skin({
	texture: circleTexture,
	height: 20, width: 40
});
assert.sameValue(circleMaskSkin.height, 20, "`circleMaskSkin` height should be 20");
assert.sameValue(circleMaskSkin.width, 40, "`circleMaskSkin` width should be 40");

circleMaskSkin = new Skin({
	texture: circleTexture,
	x: 10, y: 20, height: 30, width: 40
});
assert.sameValue(circleMaskSkin.bounds.x, 10, "`circleMaskSkin.bounds.x` should be 10");
assert.sameValue(circleMaskSkin.bounds.y, 20, "`circleMaskSkin.bounds.y` should be 20");
assert.sameValue(circleMaskSkin.bounds.height, 30, "`circleMaskSkin.bounds.height` should be 30");
assert.sameValue(circleMaskSkin.bounds.width, 40, "`circleMaskSkin.bounds.width` should be 40");

circleMaskSkin = new Skin({
	texture: circleTexture,
	states: 10, variants: 20
});
assert.sameValue(circleMaskSkin.states, 10, "`circleMaskSkin.states` should be 10");
assert.sameValue(circleMaskSkin.variants, 20, "`circleMaskSkin.variants` should be 10");

circleMaskSkin = new Skin({
	texture: circleTexture,
	tiles: { top: 1, bottom: 2, left: 3 }
});
assert.sameValue(circleMaskSkin.tiles.top, 1, "`circleMaskSkin.tiles.top` should be 1");
assert.sameValue(circleMaskSkin.tiles.bottom, 2, "`circleMaskSkin.tiles.bottom` should be 2");
assert.sameValue(circleMaskSkin.tiles.left, 3, "`circleMaskSkin.tiles.left` should be 3");
assert.sameValue(circleMaskSkin.tiles.right, 0, "`circleMaskSkin.tiles.right` should be 0");

const readOnlyProperties = ["bottom", "color", "bounds", "height", "left", "right", "states", "texture", "tiles", "top", "variants", "width"];
for (let property of readOnlyProperties) {
	assert.throws(TypeError, () => {
		circleMaskSkin[property] = 123;
	}, `${property} property of a skin object should be read-only`);
}