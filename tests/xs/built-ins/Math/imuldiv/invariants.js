/*---
description: 
flags: [onlyStrict]
---*/

const values = [-2147483648, -65536, -32768, -1, 0, 1, 32767, 65535, 2147483647];

for (let i = 0; i < values.length; i++) {
	for (let j = 0; j < values.length; j++) {
		const x = values[i];
		const y = values[j];
		assert.sameValue(Math.imul(x, y), Math.imuldiv(x, y, 1), `imul(${x}, ${y})`);
		assert.sameValue(Math.idiv(x, y), Math.imuldiv(x, 1, y), `idiv(${x}, ${y})`);
	}
}
