/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/807
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
const v1 = [1024];
v1[2147483647] = 1024;
const v2 = /\w\SMj\S+/gi;
const v5 = new Proxy(Object,v2);
const v7 = Reflect.construct(v5,v1);
}
main();
