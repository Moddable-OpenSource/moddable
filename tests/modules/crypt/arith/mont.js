/*---
description:
flags: [module]
---*/

import Modular from "modular";
import Mont from "mont";

const p = 0xffffffff00000001000000000000000000000000ffffffffffffffffffffffffn;	// a 256-bit prime
const e = 0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdefn;
const x = 0xfedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321n % p;

const expected = new Modular(p).exp(x, e);
assert.sameValue(new Mont({m: p}).exp(x, e), expected, "LR matches plain exponentiation");
