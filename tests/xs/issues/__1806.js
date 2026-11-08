/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/1806
flags: [onlyStrict,async]
includes: [compareArray.js]
---*/

var log = [];
async function* g() { yield 1; return "a"; }
var it = g();
for (let i = 1; i <= 4; i++) it.next().then(r => log.push(i + ":" + JSON.stringify(r)));
var q = Promise.resolve(); for (let i = 1; i <= 8; i++) q = q.then(() => log.push("t" + i));
q.then(() => {
	assert.compareArray(log.slice(0, 6), [
		"t1",
		'1:{"value":1,"done":false}',
		"t2",
		'2:{"value":"a","done":true}',
		'3:{"done":true}',
		'4:{"done":true}'
	], "queued next() requests on a completed async generator settle in one job");
}).then(function() {
	$DONE();
}, function(e) {
	$DONE(e);
});
