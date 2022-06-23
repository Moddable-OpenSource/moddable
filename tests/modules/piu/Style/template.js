/*---
description: 
flags: [onlyStrict]
---*/

let CssNamingConvention = Style.template({ font:"16px Open Sans" });
let cssNamingConvention = new CssNamingConvention;
let FileName = Style.template({ font: "OpenSans-Regular-16" });
let fileName = new FileName;

let RedStyle = Style.template({ color: "red" });
let redStyle = new RedStyle;
let BlueStyle = Style.template({ color: "#0000FF" });
let blueStyle = new BlueStyle;
let MulticoloredStyle = Style.template({ color: ["red", "#00FF00", "blue", "white"] });
let multicoloredStyle = new MulticoloredStyle;

let CenterStyle = Style.template({ horizontal: "center" });
let centerStyle = new CenterStyle;
let LeftStyle = Style.template({ horizontal: "left" });
let leftStyle = new LeftStyle;
let MiddleStyle = Style.template({ vertical: "middle" });
let middleStyle = new MiddleStyle;
let TopStyle = Style.template({ vertical: "top" });
let topStyle = new TopStyle;

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