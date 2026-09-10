/*---
description: searchParams stays connected to a URL constructed without a query
flags: [module]
---*/

import test from "./url_FIXTURE.js";

let url = new URL("http://moddable.com/");
let searchParams = url.searchParams;

assert.sameValue(searchParams.length, 0);
assert.sameValue(searchParams.has("null"), false);
assert.sameValue(searchParams.toString(), "");

// searchParams -> search
searchParams.set("x", "0");
assert.sameValue(url.search, "?x=0");
assert.sameValue(url.href, "http://moddable.com/?x=0");

// search -> searchParams
url = new URL("http://moddable.com/");
searchParams = url.searchParams;
url.search = "?x=1&y=2";
assert.sameValue(searchParams.get("x"), "1");
assert.sameValue(searchParams.get("y"), "2");
assert.sameValue(url.searchParams, searchParams);
