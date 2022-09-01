/*---
description:
flags: [module]
---*/

import test from "./url_FIXTURE.js";

test("https:example.org", undefined, true, "https://example.org/");
test("https:/example.org", undefined, true, "https://example.org/");
test("https://example.org", undefined, true, "https://example.org/");
test("https:///example.org", undefined, true, "https://example.org/");
test("https:////example.org", undefined, true, "https://example.org/");

test("https:\\example.org", undefined, true, "https://example.org/");
test("https:\\\\example.org", undefined, true, "https://example.org/");
test("https:\\\\\\example.org", undefined, true, "https://example.org/");






test("https:example.org", "https://example.com/", true, "https://example.com/example.org");
test("https:/example.org", "https://example.com/", true, "https://example.com/example.org");
test("https://example.org", "https://example.com/", true, "https://example.org/");
test("https:///example.org", "https://example.com/", true, "https://example.org/");
test("https:\\example.org", "https://example.com/", true, "https://example.com/example.org");
test("https:\\\\example.org", "https://example.com/", true, "https://example.org/");
test("https:\\\\\\example.org", "https://example.com/", true, "https://example.org/");

test("https:?example.org", "", false, "Failure");
test("https:#example.org", "", false, "Failure");
test("https:?example.org", "https://example.com/", true, "https://example.com/?example.org");
test("https:#example.org", "https://example.com/", true, "https://example.com/#example.org");
