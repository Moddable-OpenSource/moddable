/*---
description: >
  A function, module, or program may reserve at most 65535 scope slots for its
  variables and compiler temporaries. Exceeding that must be a SyntaxError, not
  corrupted byte code (found by ClusterFuzz as a null dereference in
  Enumerator.prototype.next after a chain of 32767 tagged templates).
flags: [onlyStrict]
---*/

function make(count) {
	const names = [];
	for (let i = 0; i < count; i++)
		names.push("a" + i.toString(36));
	return "(function() { let " + names.join(",") + "; })";
}

assert.sameValue(typeof eval(make(65535)), "function", "65535 variables compile");
assert.throws(SyntaxError, () => eval(make(65536)), "65536 variables");
