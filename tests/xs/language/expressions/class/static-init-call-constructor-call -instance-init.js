/*---
description: 
flags: [onlyStrict]
---*/

class Foo {
    #length;
    constructor(length) {
      this.#length = length;
    }
    static empty = new Foo(0);
}

assert.sameValue(Foo.empty.constructor, Foo, "static init call constructor call instance init");
