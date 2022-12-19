/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/895
---*/

assert(Object.is(120 % 2, 0));
assert(Object.is(-120 % 2, -0));
assert(Object.is(120 % -2, 0));
assert(Object.is(-120 % -2, -0));
