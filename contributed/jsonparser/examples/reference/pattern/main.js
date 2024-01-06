// Copyright (c) 2023-2024 Mark Wharton
// https://opensource.org/license/mit/

import { JSONParser, Pattern } from "jsonparser";

const fragment1 = '{"@odata.context":"https://graph.microsoft.com/v1.0/$metadata#Collection(microsoft.graph.scheduleInformation)","value":[{"scheduleId":"rm03_boardroom@example.com","availabilityView":"022222200000000000000200","scheduleItems":[{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \\u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":';
const fragment2 = '"2023-12-05T13:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T13:25:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Mark Wharton","location":"RM-03 \\u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T14:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T14:30:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \\u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T15:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T15:30:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \\u2013 Boardroom (AU)","isMeeting":true,"isRecurring":fa';
const fragment3 = 'lse,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T23:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T23:01:00.0000000","timeZone":"E. Australia Standard Time"}}],"workingHours":{"daysOfWeek":["monday","tuesday","wednesday","thursday","friday"],"startTime":"08:00:00.0000000","endTime":"17:00:00.0000000","timeZone":{"name":"Pacific Standard Time"}}}]}';

const valuePattern = new Pattern({
    setup(vpt) {
        vpt.data = {
            error: false,
            schedule: {
                count: 0,
                meetings: []
            }
        };
    },
    value: "field:value"
});

const scheduleItemsPattern = new Pattern({
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
});

const statusPattern = new Pattern({
    match(vpt, node) {
        vpt.cache.status = node.next.text;
    },
    value: "field:status"
});

const startDateTimePattern = new Pattern({
    match(vpt, node) {
        vpt.cache.startDateTime = node.next.text;
    },
    value: "field:start/object/field:dateTime"
});

const endDateTimePattern = new Pattern({
    match(vpt, node) {
        vpt.cache.endDateTime = node.next.text;
    },
    value: "field:end/object/field:dateTime"
});

const errorPattern = new Pattern({
    setup(vpt) {
        vpt.data = {
            error: true
        };
    },
    value: "field:error"
});

const patterns = [
    valuePattern,
    scheduleItemsPattern,
    statusPattern,
    startDateTimePattern,
    endDateTimePattern,
    errorPattern
];
const keys = patterns.map(pattern => pattern.names).flat();
const parser = new JSONParser({ keys, patterns });

parser.receive(fragment1);
parser.receive(fragment2);
parser.receive(fragment3);

let result = parser.status;
if (result === JSONParser.success) {
    parser.data.schedule.meetings.forEach(meeting => {
        trace(`startDateTime: ${meeting.start}, endDateTime: ${meeting.end}\n`);
    });
    trace(JSON.stringify(parser.data) + "\n");
    trace("success!\n");
}
else
    trace(`result: ${result}\n`);

parser.close();

debugger;
