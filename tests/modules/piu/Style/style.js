/*---
description: 
flags: [onlyStrict]
---*/

let cssNamingConvention = new Style({ font:"16px Open Sans" });
let fileName = new Style({ font: "OpenSans-Regular-16" });

let redStyle = new Style({ color: "red" });
let blueStyle = new Style({ color: "#0000FF" });
let multicoloredStyle = new Style({ color: ["red", "#00FF00", "blue", "white"] });

let centerStyle = new Style({ horizontal: "center" });
let leftStyle = new Style({ horizontal: "left" });
let middleStyle = new Style({ vertical: "middle" });
let topStyle = new Style({ vertical: "top" });

assert.sameValue(redStyle.color[0], 0xFF0000FF, "`redStyle` should be red");
assert.sameValue(redStyle.color[1], 0xFF0000FF, "`redStyle` should be red");
assert.sameValue(redStyle.color[2], 0xFF0000FF, "`redStyle` should be red");
assert.sameValue(redStyle.color[3], 0xFF0000FF, "`redStyle` should be red");

assert.sameValue(blueStyle.color[0], 0x0000FFFF, "`blueStyle` should be blue");
assert.sameValue(blueStyle.color[1], 0x0000FFFF, "`blueStyle` should be blue");
assert.sameValue(blueStyle.color[2], 0x0000FFFF, "`blueStyle` should be blue");
assert.sameValue(blueStyle.color[3], 0x0000FFFF, "`blueStyle` should be blue");

assert.sameValue(multicoloredStyle.color[0], 0xFF0000FF, "`multicoloredStyle` state 0 should be red");
assert.sameValue(multicoloredStyle.color[1], 0x00FF00FF, "`multicoloredStyle` state 1 should be green");
assert.sameValue(multicoloredStyle.color[2], 0x0000FFFF, "`multicoloredStyle` state 2 should be blue");
assert.sameValue(multicoloredStyle.color[3], 0xFFFFFFFF, "`multicoloredStyle` state 3 should be white");

assert.sameValue(centerStyle.horizontal, "center", "`centerStyle` should be center aligned");
assert.sameValue(leftStyle.horizontal, "left", "`leftStyle` should be left aligned");

assert.sameValue(middleStyle.vertical, "middle", "`middleStyle` should be middle aligned");
assert.sameValue(topStyle.vertical, "top", "`topStyle` should be top aligned");

// TO DO: other properties