/*---
description: 
flags: [onlyStrict]
---*/

const content = new Content(null, {
	name: "CONTENT"
});
const subcontainer = new Layout(null, {
	name: "CONTAINER",
	contents: [ content ]
});
const container = new Layout(null, {
	contents: [ subcontainer ]
});

assert.sameValue(container.content("CONTAINER"), subcontainer, `container.content should return subcontainer`);
assert.sameValue(container.content(0), subcontainer, `$container.content should return subcontainer`);

assert.sameValue(container.content("CONTENT"), undefined, `container.content should return undefined`);
assert.sameValue(container.content("NONEXISTENT"), undefined, `container.content should return undefined`);
assert.sameValue(container.content(10), undefined, `container.content should return undefined`);