/*---
description: 
flags: [module]
---*/

import * as ns0 from "./bar0_FIXTURE.js"
import * as ns1 from "./bar1_FIXTURE.js"
import * as ns2 from "./bar2_FIXTURE.js"
import * as ns3 from "./bar3_FIXTURE.js"
assert.sameValue(ns0.x, undefined, "reexport");
assert.sameValue(ns1.x, 1, "reexport");
assert.sameValue(ns2.x, 2, "reexport");
assert.sameValue(ns3.x, 3, "reexport");



