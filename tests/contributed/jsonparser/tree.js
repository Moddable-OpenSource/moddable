/*---
flags: [module]
---*/

// Copyright (c) 2023-2024 Mark Wharton
// https://opensource.org/license/mit/

import { JSONParser } from "jsonparser";
import { fragment1, fragment2, fragment3, keys } from "./jsonparser_FIXTURE.js";

const parser = new JSONParser({ keys });

parser.receive(fragment1);
parser.receive(fragment2);
parser.receive(fragment3);

assert.sameValue(parser.status, JSONParser.success);
assert(parser.root.value !== undefined, "root value is undefined");

const value = parser.root.value.value;
assert(value !== undefined, "value is undefined");

assert.sameValue(value.length, 1);

const scheduleItems = value[0].scheduleItems;
assert(scheduleItems !== undefined, "scheduleItems is undefined");

assert.sameValue(scheduleItems.length, 4);

const meeting1 = scheduleItems[0];
assert.sameValue(meeting1.status, "busy");
assert.sameValue(meeting1.start.dateTime, "2023-12-05T13:00:00.0000000");
assert.sameValue(meeting1.end.dateTime, "2023-12-05T13:25:00.0000000");

const meeting2 = scheduleItems[1];
assert.sameValue(meeting2.status, "busy");
assert.sameValue(meeting2.start.dateTime, "2023-12-05T14:00:00.0000000");
assert.sameValue(meeting2.end.dateTime, "2023-12-05T14:30:00.0000000");

const meeting3 = scheduleItems[2];
assert.sameValue(meeting3.status, "busy");
assert.sameValue(meeting3.start.dateTime, "2023-12-05T15:00:00.0000000");
assert.sameValue(meeting3.end.dateTime, "2023-12-05T15:30:00.0000000");

const meeting4 = scheduleItems[3];
assert.sameValue(meeting4.status, "busy");
assert.sameValue(meeting4.start.dateTime, "2023-12-05T23:00:00.0000000");

assert.sameValue(meeting4.end.dateTime, "2023-12-05T23:01:00.0000000");

parser.close();
