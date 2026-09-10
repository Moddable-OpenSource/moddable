/*---
description: setting href keeps the existing searchParams object and synchronizes it with the new URL
flags: [module]
---*/

import test from "./url_FIXTURE.js";

const url = new URL("http://moddable.com/?x=0");
const searchParams = url.searchParams;

url.href = "http://example.com/?y=1";
assert.sameValue(url.searchParams, searchParams);
assert.sameValue(searchParams.get("x"), null);
assert.sameValue(searchParams.get("y"), "1");

searchParams.set("z", "2");
assert.sameValue(url.search, "?y=1&z=2");
assert.sameValue(url.href, "http://example.com/?y=1&z=2");

url.href = "http://example.com/";
assert.sameValue(searchParams.length, 0);
assert.sameValue(searchParams.has("null"), false);

searchParams.set("w", "3");
assert.sameValue(url.href, "http://example.com/?w=3");
