/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/446
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function test() {
	var numberCount = 0;
	var stringCount = 0;
	var booleanCount = 0;
	var symbolCount = typeof numberCount;
	var spy;
	spy = new Proxy({}, {
		set: function () {
			numberCount += 1;
			return true;
		}
	});
	Object.setPrototypeOf(Number.prototype, spy);
	0 .test262 = null;
	spy = new Proxy({}, {
		set: function () {
			{
	  symbolCount[symbolCount] += 1;
	}
			return true;
		}
	});
	Object.setPrototypeOf(String.prototype, spy);
	''.test262 = null;
	spy = new Proxy({}, {
		set: function () {
			booleanCount += 1;
			return true;
		}
	});
	Object.setPrototypeOf(Boolean.prototype, spy);
	true.test262 = null;
	spy = new Proxy({}, {
		set: function () {
			symbolCount += 1;
			return true;
		}
	});
	Object.setPrototypeOf(Symbol.prototype, spy);
	Symbol().test262 = null;
}
test();
