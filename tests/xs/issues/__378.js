/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/378
flags: [onlyStrict]
negative:
  type: SyntaxError
---*/

var re = eval('RYYr3HBNdbk/Goz,' + new RegExp('').source + 'f');
var BcaA = new SharedArrayBuffer(null);
var SGHt = new SharedArrayBuffer(null);
var MTnX = Math;
assert.sameValue(re.test(''), true);
