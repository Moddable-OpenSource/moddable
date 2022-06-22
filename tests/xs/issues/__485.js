/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/485
flags: [onlyStrict]
---*/

// Copyright (C) 2017 Josh Wolfe. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.
/*---
esid: pending
description: BigInt.asUintN order of parameter type coercion
info: |
  BigInt.asUintN ( bits, bigint )

  1. Let bits be ? ToIndex(bits).
  2. Let bigint ? ToBigInt(bigint).

features: [BigInt]
---*/

var i = 0;
var bits = {
  valueOf() {
    i++;
    return 1065353216;
  }
};
var bigint = {
  valueOf() {
    i++;
    return 0n;
  }
};

BigInt.asUintN(bits, bigint);
