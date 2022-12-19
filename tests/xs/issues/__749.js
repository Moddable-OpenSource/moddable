/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/749
flags: [onlyStrict]
---*/

var f32 = new Float32Array(127);
f32.sort(function () {
	return 1;
});
