/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/330
flags: [onlyStrict]
---*/

const source = [
	`]() => {}.call // function call (){[native code]}`,
	`() => {}['call'] // function call (){[native code]}`,
	`() => {}\`\` // undefined`,
	`new () => {} // TypeError: ?: new 9: no constructor`,
	`() => {}() // undefined`,
	`() => {}(Date) // undefined`,
	`() => {}(Date.now) // undefined`,
	`+() => {} // NaN`,
	`!() => {} // false`,
	`delete () => {} // true`,
	`void () => {} // undefined`,
	`typeof () => {} // function`,
	`2 << () => {} // 2`,
	`() => {} instanceof Function // true`,
	`() => {} || 1 // function  (){[native code]}`,
	`'abc' in () => {} // false`,
	`() => {} ? true : false // true`,
];

source.forEach(source => {
	assert.throws(SyntaxError, () => eval(source));
});
