/*---
description: 
flags: [module]
---*/

import Preference from "preference";

const long = "012345678901234";
Preference.delete(long, long);
Preference.set(long, long, 123);
Preference.delete(long, long);
assert.sameValue(Preference.get(long, long), undefined, "preference delete failed with long domain/key");

Preference.set("foo", 1, 1);
Preference.set("foo", 2, 2);
assert.sameValue(Preference.get("foo", 1), 1);
assert.sameValue(Preference.get("foo", 2), 2);
Preference.delete("foo", 1);
Preference.delete("foo", "2");
assert.sameValue(Preference.get("foo", "1"), undefined, "preference not deleted");
assert.sameValue(Preference.get("foo", 2), undefined, "preference not deleted");

Preference.delete("foo", 2);		// double delete
Preference.delete("foo", 3);		// never created		
Preference.delete("bar", 4);		// empty domain		

assert.throws(SyntaxError, () => Preference.delete(), "preference.delete requires 2 arguments")
assert.throws(SyntaxError, () => Preference.delete("1"), "preference.delete requires 2 arguments")
assert.throws(TypeError, () => Preference.delete(Symbol()), "preference.delete requires string arguments")
assert.throws(TypeError, () => Preference.delete("1", Symbol()), "preference.delete requires string arguments")
