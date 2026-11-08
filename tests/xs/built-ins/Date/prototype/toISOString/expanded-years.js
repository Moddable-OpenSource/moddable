/*---
features: [Date]
---*/

assert.sameValue(new Date(Date.UTC(-1, 1, 1)).toISOString(), "-000001-02-01T00:00:00.000Z", "year -1");
assert.sameValue(new Date(Date.UTC(10000, 0, 1)).toISOString(), "+010000-01-01T00:00:00.000Z", "year 10000");
assert.sameValue(new Date(-62198755200000).toISOString(), "-000001-01-01T00:00:00.000Z", "year -1 from time value");
assert.sameValue(new Date(-62167219200000).toISOString(), "0000-01-01T00:00:00.000Z", "year 0");
