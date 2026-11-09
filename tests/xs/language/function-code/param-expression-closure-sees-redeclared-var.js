/*---
description: A closure in a parameter expression sees the parameter, not a body var that redeclares it
---*/

function f(a = 1, b = () => a) { var a = 2; return [a, b()]; }
assert.sameValue(f().join(), "2,1", "body var assignment does not reach the parameter closure");

function g(x, h = () => x) { var x = "body"; return h(); }
assert.sameValue(g("param"), "param", "closure sees the parameter after body var assignment");

function h(x = 1, c = () => x) { var x; return [x, c()]; }
assert.sameValue(h().join(), "1,1", "body var without initializer starts with the parameter value");

function k(x = 3, c = () => x) { { var x = 5; } return [x, c()]; }
assert.sameValue(k().join(), "5,3", "block-level body var redeclaring a parameter");
