// Copyright (c) 2023-2024 Mark Wharton
// https://opensource.org/license/mit/

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
        value: "/object/field:value"
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

let request = new Request({ host: "localhost",
                            path: "sample-schedule-info.json",
                            port: 8000 });

request.callback = function(message, value, etc)
{
    if (Request.status === message) {
        this.parser = new JSONParser({ keys, patterns });
    }
    else if (Request.responseFragment === message) {
        let size = 64, start = 0, string = this.read(String);
        while (this.parser.receive(string, start, start + size) > 0) {
            start += size;
        }
    }
    else if (Request.responseComplete === message) {
        let result = this.parser.status;
        if (result === JSONParser.success) {
            const data = this.parser.data;
            if (data.error) {
                trace(`An error occurred: ${data.errorCode}\n`);
            }
            else {
                const schedule = data.schedule;
                trace(`Logged ${schedule.count} meetings.\n`);
                trace(`Here are the top ${schedule.meetings.length} for further processing:\n`);
                schedule.meetings.forEach((meeting, index) => {
                    trace(`${index + 1}. From ${meeting.start.toLocaleDateString()} ${meeting.start.toLocaleTimeString()} to ${meeting.end.toLocaleTimeString()}\n`);
                });
            }
        }
        else {
            trace(`result: ${result}\n`);
        }
        this.parser.close();
    }
}
