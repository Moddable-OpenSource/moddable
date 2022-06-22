/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/818
flags: [onlyStrict,async]
includes: [compareArray.js]
---*/

const results = [];
(async () => {
  async function* iterator() {
    try {
      yield Promise.reject("C");
    } finally {
      results.push("A");
    }
  }
  try {
    for await (const x of iterator());
  } catch (e) {
    results.push("B");
    results.push("C");
  }
})().then(function(x) {
	assert.compareArray(["A", "B", "C"], results, "Order");
}).then(function() {
	$DONE();
}, function(e) {
	$DONE(e);
});


