/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/123
flags: [onlyStrict]
---*/

let a = [];

function f(){
	a.length = 10000;
	a.fill(10);
	return a;
}

var t = [];

function n(){

	for(var i = 0; i < 1000; i++){
	var tt = "0123456789012345678901234567890123456789"
	t.push([tt]);
	}
	return 999;

}

function m(){

        a.length = 1;


	return 500;
}

class MyArray extends Array {
  static get [Symbol.species]() { return f; }
}

var q = a;
q.length = 1000;
q.fill(0x77777777);


q.copyWithin(0, {valueOf : m}, {valueOf : n});

for(var i = 0; i < 20; i++){
	try{
	assert.sameValue(t[i], "0123456789012345678901234567890123456789", "hello " + i);
	}catch(e){}
}

assert.sameValue(q.length, 1, "hello");
