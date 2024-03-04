/*---
description: 
flags: [async,onlyStrict]
includes: [compareArray.js]
---*/

async function test() {
	const result = [];
	const fr = new FinalizationRegistry((value) => result.push(value));
	let wm = new WeakMap();
	
	let o1 = {};
	fr.register(o1, "o1");
	let s1 = Symbol();
	fr.register(s1, "s1");
	let p1 = { [s1]: 1 };
	wm.set(s1, o1);
	o1 = null;
	s1 = null;
	
	let o2 = {};
	fr.register(o2, "o2");
	let s2 = Symbol();
	fr.register(s2, "s2");
	let p2 = { [s2]: 2 };
	wm.set(s2, o2);
	o2 = null;
	s2 = null;
	p2 = null;
	
	let o3 = {};
	fr.register(o3, "o3");
	let s3 = Symbol();
	fr.register(s3, "s3");
	wm.set(s3, o3);
	o3 = null;
	s3 = null;

	$262.gc();
	return result;
}

const expected = [ "o2", "s2", "o3", "s3" ];
test()
.then(it => {
	assert.compareArray(expected, it);
})
.then($DONE, $DONE);

