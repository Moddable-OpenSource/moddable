/*---
description:
flags: [module]
---*/

import test from "./url_FIXTURE.js";

const url = new URL("http://moddable.com/?x=0&y=1");
const searchParams = url.searchParams;

assert.sameValue(searchParams.get("x"), "0");
assert.sameValue(searchParams.get("y"), "1");

url.search = "?x=2&y=3";

assert.sameValue(searchParams.get("x"), "2");
assert.sameValue(searchParams.get("y"), "3");
