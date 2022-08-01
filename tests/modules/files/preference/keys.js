/*---
description: 
flags: [module]
---*/

import Preference from "preference";

let keys = Preference.keys("keys");
for (let key in keys)
	Preference.delete("keys", key);

const test = {
	1: "one",
	"two": 2,
	"étc": true,
	true: false
};

for (let key in test)
	Preference.set("keys", key, test[key]);

keys = Preference.keys("keys");
assert.sameValue(keys.length, 4, "wrong number of keys");
assert(keys.includes("1") && keys.includes("two") && keys.includes("étc") && keys.includes("true"), "incorrect key names")

keys.forEach(key => Preference.delete("keys", key));

keys = Preference.keys("keys");
assert.sameValue(keys.length, 0, "non-zero key length after delete all");
