/*---
flags: [module]
---*/

// Copyright (c) 2023-2024 Mark Wharton
// https://opensource.org/license/mit/

import { JSONParser, Pattern } from "jsonparser";
import { fragment1, fragment2, fragment3, timeZoneOffset } from "./jsonparser_FIXTURE.js";

const patterns = [
    new Pattern({
        setup(vpt) {
            vpt.data = {
                error: false,
                schedule: {
                    count: 0,
                    meetings: []
                }
            };
        },
        value: "/object/field:value"
    }),
    new Pattern({
        match(vpt) {
            if (vpt.cache.status === "busy") {
                const schedule = vpt.data.schedule;
                if (schedule.count < 3) {
                    schedule.meetings.push({
                        start: new Date(vpt.cache.startDateTime + timeZoneOffset),
                        end: new Date(vpt.cache.endDateTime + timeZoneOffset)
                    });
                }
                schedule.count++;
            }
            delete vpt.cache;
        },
        setup(vpt) {
            vpt.cache = {};
        },
        value: "field:scheduleItems/array/object"
    }),
    new Pattern({
        match(vpt, node) {
            vpt.cache.status = node.text;
        },
        value: "field:status/string"
    }),
    new Pattern({
        match(vpt, node) {
            vpt.cache.startDateTime = node.text;
        },
        value: "field:start/object/field:dateTime/string"
    }),
    new Pattern({
        match(vpt, node) {
            vpt.cache.endDateTime = node.text;
        },
        value: "field:end/object/field:dateTime/string"
    })
];

const keys = patterns.map(pattern => pattern.names).flat();
const parser = new JSONParser({ keys, patterns });

parser.receive(fragment1);
parser.receive(fragment2);
parser.receive(fragment3);

assert.sameValue(parser.status, JSONParser.success);

const error = parser.data.error;
assert(error !== undefined, "error is undefined");

assert.sameValue(error, false);

const schedule = parser.data.schedule;
assert(schedule !== undefined, "schedule is undefined");

assert.sameValue(schedule.count, 4);

const meetings = schedule.meetings;
assert(meetings !== undefined, "meetings is undefined");

assert.sameValue(meetings.length, 3);
assert.sameValue(meetings[0].start.valueOf(), 1701745200000);
assert.sameValue(meetings[0].end.valueOf(), 1701746700000);
assert.sameValue(meetings[1].start.valueOf(), 1701748800000);
assert.sameValue(meetings[1].end.valueOf(), 1701750600000);
assert.sameValue(meetings[2].start.valueOf(), 1701752400000);
assert.sameValue(meetings[2].end.valueOf(), 1701754200000);

parser.close();
