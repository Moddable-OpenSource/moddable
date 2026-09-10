/*---
description: deleting the last search parameter removes the query from href
flags: [module]
---*/

import test from "./url_FIXTURE.js";

// https://url.spec.whatwg.org/#concept-urlsearchparams-update
// an empty serialization sets the URL query to null, so href has no "?"

let url = new URL("http://moddable.com/?x=0");
let searchParams = url.searchParams;

searchParams.delete("x");
assert.sameValue(searchParams.length, 0);
assert.sameValue(url.search, "");
assert.sameValue(url.href, "http://moddable.com/");

url = new URL("http://moddable.com/?x=0&y=1");
searchParams = url.searchParams;
searchParams.delete("x");
assert.sameValue(url.href, "http://moddable.com/?y=1");
searchParams.delete("y");
assert.sameValue(url.search, "");
assert.sameValue(url.href, "http://moddable.com/");
