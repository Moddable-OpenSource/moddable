/*---
description: 
flags: [onlyStrict, module]
---*/

/*
Piu color documentation: https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md#color
*/

import { rgb, rgba, hsl, hsla } from "piu/All";

assert.sameValue(rgb(255, 0, 0), 0xFF0000FF, `Incorrect value returned by rgb function`);
assert.sameValue(rgb(0, 255, 0), 0x00FF00FF, `Incorrect value returned by rgb function`);
assert.sameValue(rgb(0, 0, 255), 0x0000FFFF, `Incorrect value returned by rgb function`);
assert.sameValue(rgb(153, 153, 153), 0x999999FF, `Incorrect value returned by rgb function`);

assert.sameValue(rgba(255, 0, 0, 0.6), 0xFF000099, `Incorrect value returned by rgba function`);
assert.sameValue(rgba(0, 255, 0, 0.6), 0x00FF0099, `Incorrect value returned by rgba function`);
assert.sameValue(rgba(0, 0, 255, 0.6), 0x0000FF99, `Incorrect value returned by rgba function`);
assert.sameValue(rgba(153, 153, 153, 0.6), 0x99999999, `Incorrect value returned by rgba function`);

assert.sameValue(hsl(0, 1, 0.5), 0xFF0000FF, `Incorrect value returned by hsl function`);
assert.sameValue(hsl(120, 1, 0.5), 0x00FF00FF, `Incorrect value returned by hsl function`);
assert.sameValue(hsl(240, 1, 0.5), 0x0000FFFF, `Incorrect value returned by hsl function`);
assert.sameValue(hsl(0, 1, 1), 0xFFFFFFFF, `Incorrect value returned by hsl function`);
assert.sameValue(hsl(1, 0, 0), 0x000000FF, `Incorrect value returned by hsl function`);

assert.sameValue(hsla(0, 1, 0.5, 0.6), 0xFF000099, `Incorrect value returned by hsla function`);
assert.sameValue(hsla(120, 1, 0.5, 0.6), 0x00FF0099, `Incorrect value returned by hsla function`);
assert.sameValue(hsla(240, 1, 0.5, 0.6), 0x0000FF99, `Incorrect value returned by hsla function`);
assert.sameValue(hsla(0, 1, 1, 0.6), 0xFFFFFF99, `Incorrect value returned by hsla function`);
assert.sameValue(hsla(1, 0, 0, 0.6), 0x00000099, `Incorrect value returned by hsla function`);

const validColors = [
	// CSS level 2: https://developer.mozilla.org/en-US/docs/Web/CSS/color_value
	"black",
	"silver",
	"gray",
	"white",
	"maroon",
	"red",
	"purple",
	"fuchsia",
	"green",
	"lime",
	"olive",
	"yellow",
	"navy",
	"blue",
	"teal", 
	"aqua",
	"orange",

	// Hexadecimal notations
	"#F00",
	"#F008",
	"#00FF00",
	"#00FF0088",

	// Hexadecimal numbers
	0x0000FFFF,
	0x0000FF88,
	rgb(255, 255, 0),
	rgba(255, 255, 0, 0.5),
	hsl(180, 1, 0.5),
	hsla(180, 1, 0.5, 0.5)
]

for (let fill of validColors) {
	const testSkin = new Skin({ fill });
}

const invalidColors = [
	"notacsscolor",
	undefined,
	null
]

for (let fill of invalidColors) {
	assert.throws(Error, () => {
		const testSkin = new Skin({ fill });
	}, `Error should be thrown when invalid color ${fill} is specified`);
}