/*---
description: 
flags: [onlyStrict]
---*/

const sampleStyle = new Style({ font:"16px Open Sans", color: "black", horizontal: "left" });
const topMarginStyle = new Style({ top: 30 });
const leftMarginStyle = new Style({ left: 30 });
const redStyle = new Style({ color: "red" });
const centerStyle = new Style({ horizontal: "center" });
const blueRightStyle = new Style({ color: "blue", horizontal: "right" });
const leadingStyle = new Style({ leading: 50, color: "red" });

const label1 = new Label(null, { top: 0, left: 0, right: 0, string: "Black left aligned" });
const label2 = new Label(null, { style: redStyle, top: 30, left: 0, right: 0, string: "Red left aligned" });
const label3 = new Label(null, { style: blueRightStyle, top: 30, left: 0, right: 0, string: "Blue right aligned" });
const label4 = new Label(null, { style: centerStyle, top: 60, left: 0, right: 0, string: "Black center aligned" });
const label5 = new Label(null, { style: topMarginStyle, top: 90, left: 0, right: 0, string: "Black with top margin" });
const label6 = new Label(null, { style: leftMarginStyle, top: 90, left: 0, width: 200, string: "Black with left margin and truncated" });

new Application(null, {
	displayListLength: 5000,
	style: sampleStyle, skin: new Skin({ fill: "white" }),
    contents: [ label1, label2, label3, label4, label5, label6 ]
});
screen.checkImage("b88745fed3304441117048756f3739c9");