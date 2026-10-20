/*---
description: StringToNumber rounds a decimal exactly halfway between two Numbers to the one with an even significand
flags: [onlyStrict]
---*/

const cases = [
	["2251799813685248.250", 2251799813685248],
	["4503599627370496.500", 4503599627370496],
	["9007199254740993.000", 9007199254740992],
	["9007199254740993", 9007199254740992],
	["9007199254740995", 9007199254740996],
	["18014398509481986.000", 18014398509481984],
	["23575005085478010", 23575005085478008],
	["1.00000000000000011102230246251565404236316680908203125", 1],
	["1.000000000000000111022302462515654042363166809082031250001", 1.0000000000000002],
	["2.4703282292062327e-324", 0],
	["2.4703282292062328e-324", 5e-324],
];
for (const [string, value] of cases) {
	assert.sameValue(Number(string), value, `Number("${string}")`);
	assert.sameValue(parseFloat(string), value, `parseFloat("${string}")`);
	assert.sameValue(JSON.parse(string), value, `JSON.parse("${string}")`);
	assert.sameValue(Number("-" + string), -value, `Number("-${string}")`);
}

assert.sameValue(2251799813685248.250, 2251799813685248, "literal 2251799813685248.250");
assert.sameValue(4503599627370496.500, 4503599627370496, "literal 4503599627370496.500");
assert.sameValue(9007199254740993, 9007199254740992, "literal 9007199254740993");
assert.sameValue(18014398509481986, 18014398509481984, "literal 18014398509481986");

assert.sameValue(String(1e23), "1e+23", "String(1e23)");
assert.sameValue(String(5e22), "5e+22", "String(5e22)");
assert.sameValue(String(2.5e22), "2.5e+22", "String(2.5e22)");
assert.sameValue(String(1418222073344627.2), "1418222073344627.2", "String(1418222073344627.2)");
