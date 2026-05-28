/*---
description: scan options must include onFound
flags: [module]
---*/

import WiFi from "embedded:network/interface/wifi";

const wifi = new WiFi({});
// onComplete and channel are both optional; pass them to isolate that the
// failure is specifically because onFound is missing.
assert.throws(Error, () => wifi.scan({onComplete() {}, channel: 1}), "scan without onFound should throw");
wifi.close();
