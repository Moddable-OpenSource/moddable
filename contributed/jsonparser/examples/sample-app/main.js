// Copyright (c) 2023 Mark Wharton
// https://opensource.org/license/mit/
"use strict";

import { JSONParser, Pattern } from "jsonparser";
import { Request } from "http";

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
        value: "root/object/field:value"
    }),
    new Pattern({
        match(vpt) {
            if (vpt.cache.status === "busy") {
                const schedule = vpt.data.schedule;
                if (schedule.count < 3) {
                    schedule.meetings.push({
                        start: new Date(vpt.cache.startDateTime),
                        end: new Date(vpt.cache.endDateTime)
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
    }),
    new Pattern({
        match(vpt, node) {
            vpt.data = {
                error: true,
                errorCode: node.text
            };
        },
        value: "field:error/object/field:code/string"
    })
];
const keys = patterns.map(pattern => pattern.names).flat();

let parser;
let request = new Request({ host: "localhost", path: "sample-schedule-info.json", port: 8000 });
request.callback = function(message, value, etc)
{
    if (Request.status === message) {
        parser = new JSONParser({ keys, patterns });
    }
    else if (Request.responseFragment === message) {
        let size = 64, start = 0, text = this.read(String);
        while (parser.receive(text, start, start + size) > 0) {
            start += size;
        }
    }
    else if (Request.responseComplete === message) {
        let result = parser.status;
        if (result === JSONParser.success) {
            if (parser.data.error) {
                trace(`An error occurred: ${parser.data.errorCode}\n`);
            }
            else {
                trace(`Logged ${parser.data.schedule.count} meetings.\n`);
                trace(`Here are the top ${parser.data.schedule.meetings.length} for further processing:\n`);
                parser.data.schedule.meetings.forEach((meeting, index) => {
                    trace(`${index + 1}. From ${meeting.start.toLocaleDateString()} ${meeting.start.toLocaleTimeString()} to ${meeting.end.toLocaleTimeString()}\n`);
                });
                trace("success!\n");
            }
        }
        else {
            trace(`result: ${result}\n`);
        }
        parser.close();
    }
}
