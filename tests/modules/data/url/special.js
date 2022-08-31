/*---
description:
flags: [module]
---*/

import URL from "url";

function test(url, base, success, expected) {
	let actual;
	try {
		let it = base ? new URL(url, new URL(base)) : new URL(url);
		actual = it.toString();
		if (!success)
  			throw new Test262Error("Expected failure but got «" + actual + "»");
		assert.sameValue(actual, expected);
  }
  catch(e) {
    if (success)
  		throw new Test262Error("Expected «" + expected + "» but got ", e.name);
  }
}

test("https:example.org", "", true, "https://example.org/");
test("https:/example.org", "", true, "https://example.org/");
test("https://example.org", "", true, "https://example.org/");
test("https:///example.org", "", true, "https://example.org/");
test("https:\\example.org", "", true, "https://example.org/");
test("https:\\\\example.org", "", true, "https://example.org/");
test("https:\\\\\\example.org", "", true, "https://example.org/");
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
