/*---
flags: [module]
---*/

// Copyright (c) 2023-2024 Mark Wharton
// https://opensource.org/license/mit/

import { JSONParser, Matcher, NodeType } from "jsonparser";
import { fragment1, fragment2, fragment3, keys } from "./jsonparser_FIXTURE.js";

const matcher = new Matcher({
    begin(vpt) {
        vpt.data = {
            error: false,
            schedule: {
                count: 0,
                meetings: []
            }
        };
    },
    match(vpt, node) {
        switch (node.type) {
            case NodeType.field:
                switch (node.text) {
                    case "status":
                        node.up(1, NodeType.object).data.status = node.next.text;
                        break;
                    case "dateTime":
                        node.up(3, NodeType.object).data[node.up(2, NodeType.field).text] = node.next.text;
                        break;
                    case "error":
                        vpt.data.error = true;
                        break;
                }
                break;
            case NodeType.object:
                if (node.up(2, NodeType.field, "scheduleItems")) {
                    let data = node.data;
                    if (data.status === "busy") {
                        let schedule = vpt.data.schedule;
                        if (schedule.count < 3) {
                            schedule.meetings.push({
                                start: new Date(data.start),
                                end: new Date(data.end)
                            });
                        }
                        schedule.count++;
                    }
                }
                break;
        }
    }
});

const parser = new JSONParser({ keys, matcher });

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
