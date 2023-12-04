// Copyright (c) 2023 Mark Wharton
// https://opensource.org/license/mit/
"use strict";

import { JSONParser, Matcher, NodeType } from "jsonparser";

const fragment1 = '{"@odata.context":"https://graph.microsoft.com/v1.0/$metadata#Collection(microsoft.graph.scheduleInformation)","value":[{"scheduleId":"rm03_boardroom@example.com","availabilityView":"022222200000000000000200","scheduleItems":[{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \\u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":';
const fragment2 = '"2023-12-05T13:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T13:25:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Mark Wharton","location":"RM-03 \\u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T14:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T14:30:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \\u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T15:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T15:30:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \\u2013 Boardroom (AU)","isMeeting":true,"isRecurring":fa';
const fragment3 = 'lse,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T23:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T23:01:00.0000000","timeZone":"E. Australia Standard Time"}}],"workingHours":{"daysOfWeek":["monday","tuesday","wednesday","thursday","friday"],"startTime":"08:00:00.0000000","endTime":"17:00:00.0000000","timeZone":{"name":"Pacific Standard Time"}}}]}';

const matcher = new Matcher({
    match(vpt, node) {
        let arrow = " ← ";
        let check = node;
        while (node.prev)
            node = node.prev;
        while (node) {
            if (node.prev)
                trace(arrow);
            switch (node.type) {
                case NodeType.null:
                    trace("null");
                    break;
                case NodeType.false:
                    trace("false");
                    break;
                case NodeType.true:
                    trace("true");
                    break;
                case NodeType.number:
                    trace(`number ${node.text}`);
                    break;
                case NodeType.string:
                    trace(`string "${node.text}"`);
                    break;
                case NodeType.array:
                    trace("array");
                    break;
                case NodeType.object:
                    trace("object");
                    break;
                case NodeType.field:
                    if (node.text !== undefined)
                        trace(`field [${node.text}]`);
                    else
                        trace("field");
                    break;
                case NodeType.root:
                    trace("root");
                    break;
            }
            if (node === check)
                arrow = " → ";
            node = node.next;
        }
        trace("\n");
    }
});

const parser = new JSONParser({ matcher });

parser.receive(fragment1);
parser.receive(fragment2);
parser.receive(fragment3);

let result = parser.status;
if (result === JSONParser.success)
    trace("success!\n");
else
    trace(`result: ${result}\n`);

parser.close();

debugger;
