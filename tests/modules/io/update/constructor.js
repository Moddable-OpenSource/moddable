/*---
description: 
flags: [module]
---*/

import update from "embedded:update";
import flash from "embedded:storage/flash";


const nextota = flash.open({path: "nextota"});
const ota = update.open({partition: nextota});
ota.close();

assert.throws(TypeError, () => update.open());
assert.throws(Error, () => update.open({}));
assert.throws(SyntaxError, () => update.open({partition: "nextota"}));
assert.throws(RangeError, () => update.open({partition: nextota, mode: "xyzzy"}));
assert.throws(RangeError, () => update.open({partition: nextota, mode: "w+"}));
assert.throws(RangeError, () => update.open({partition: nextota, mode: "r"}));
assert.throws(RangeError, () => update.open({partition: nextota, mode: 12}));
assert.throws(TypeError, () => update.open({partition: nextota, mode: Symbol()}));
assert.throws(RangeError, () => update.open({partition: nextota, mode: {}}));
assert.throws(Error, () => update.open({mode: "a"}));

update.open({partition: nextota, mode: "a"}).close();
update.open({partition: nextota, mode: "w"}).close();
update.open({partition: nextota, byteLength: 0}).close();

assert.throws(RangeError, () => update.open({partition: nextota, byteLength: NaN}));
assert.throws(RangeError, () => update.open({partition: nextota, byteLength: "abcd"}));
assert.throws(RangeError, () => update.open({partition: nextota, byteLength: -1}));
assert.throws(RangeError, () => update.open({partition: nextota, byteLength: 4_294_967_296}));
assert.throws(TypeError, () => update.open({partition: nextota, byteLength: Symbol()}));
assert.throws(RangeError, () => update.open({partition: nextota, byteLength: nextota.status().size + 1}));
update.open({partition: nextota, byteLength: nextota.status().size}).close();

nextota.close();
assert.throws(SyntaxError, () => update.open({partition: nextota}));

const readonly = flash.open({path: "nextota", mode: "r"});
assert.throws(Error, () => update.open({partition: readonly}));
readonly.close();
