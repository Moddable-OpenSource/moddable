/*---
description: clearing search empties searchParams without creating a "null" pair
flags: [module]
---*/

import test from "./url_FIXTURE.js";

const url = new URL("http://moddable.com/?x=0&y=1");
const searchParams = url.searchParams;

url.search = "";
assert.sameValue(url.search, "");
assert.sameValue(url.href, "http://moddable.com/");
assert.sameValue(searchParams.length, 0);
assert.sameValue(searchParams.has("null"), false);
assert.sameValue(searchParams.get("x"), null);

searchParams.set("z", "2");
assert.sameValue(url.search, "?z=2");
assert.sameValue(url.href, "http://moddable.com/?z=2");
