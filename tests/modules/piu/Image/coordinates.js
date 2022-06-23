/*---
description: 
flags: [onlyStrict]
---*/

const coordinateProperties = ["left", "right", "width", "top", "bottom", "height"];

const content = new Image(null, ({ path: "screen2.cs" }));

new Application(null, {
    contents: [ content ]
});

let coordinates = content.coordinates;
for (let property of coordinateProperties) {
    assert.sameValue(coordinates[property], undefined, `all coordinate properties should be undefined`);
}

let newCoordinates = { left: 10, right: 20, height: 30, width: 40 };
content.coordinates = newCoordinates;
coordinates = content.coordinates;
for (let property of coordinateProperties) {
    assert.sameValue(coordinates[property], newCoordinates[property], `coordinates of content should match newCoordinates`);
}