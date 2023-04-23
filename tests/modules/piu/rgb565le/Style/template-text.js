/*---
description: 
flags: [onlyStrict]
---*/

const sampleStyle = Style.template({ font:"16px Open Sans", color: "black", horizontal: "left" });
const topMarginStyle = Style.template({ top: 30 });
const leftMarginStyle = Style.template({ left: 30 });
const redStyle = Style.template({ color: "red" });
const centerStyle = Style.template({ horizontal: "center" });
const blueRightStyle = Style.template({ color: "blue", horizontal: "right" });
const leadingStyle = Style.template({ leading: 50, color: "red" });

const text1 = new Text(null, { top: 0, left: 0, right: 0, string: "Black left aligned" });
const text2 = new Text(null, { Style: redStyle, top: 30, left: 0, right: 0, string: "Red left aligned" });
const text3 = new Text(null, { Style: blueRightStyle, top: 30, left: 0, right: 0, string: "Blue right aligned" });
const text4 = new Text(null, { Style: centerStyle, top: 60, left: 0, right: 0, string: "Black center aligned" });
const text5 = new Text(null, { Style: topMarginStyle, top: 90, left: 0, right: 0, string: "Black with top margin" });
const text6 = new Text(null, { Style: leftMarginStyle, top: 90, left: 0, right: 0, string: "Black with left margin" });
const text7 = new Text(null, { top: 150, left: 0, right: 0, string: "Multiline\nno leading property set" });
const text8 = new Text(null, { Style: leadingStyle, top: 150, left: 0, right: 0, string: "Multiline\nleading property set"});

new Application(null, {
	displayListLength: 5000,
	Style: sampleStyle, skin: new Skin({ fill: "white" }),
    contents: [ text1, text2, text3, text4, text5, text6, text7, text8 ]
});
screen.checkImage("0f452f1e7f1494749ef48f962be01882");