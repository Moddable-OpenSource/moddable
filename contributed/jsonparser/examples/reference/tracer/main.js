// Copyright (c) 2023-2024 Mark Wharton
// https://opensource.org/license/mit/

import { JSONParser, NodeType, VPT } from "jsonparser";

const fragment1 = '{"@odata.context":"https://graph.microsoft.com/v1.0/$metadata#Collection(microsoft.graph.scheduleInformation)","value":[{"scheduleId":"rm03_boardroom@example.com","availabilityView":"022222200000000000000200","scheduleItems":[{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \\u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":';
const fragment2 = '"2023-12-05T13:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T13:25:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Mark Wharton","location":"RM-03 \\u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T14:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T14:30:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \\u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T15:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T15:30:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \\u2013 Boardroom (AU)","isMeeting":true,"isRecurring":fa';
const fragment3 = 'lse,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T23:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T23:01:00.0000000","timeZone":"E. Australia Standard Time"}}],"workingHours":{"daysOfWeek":["monday","tuesday","wednesday","thursday","friday"],"startTime":"08:00:00.0000000","endTime":"17:00:00.0000000","timeZone":{"name":"Pacific Standard Time"}}}]}';

const keys = ["value", "scheduleItems", "status", "start", "end", "dateTime", "error"];

class TracerVPT extends VPT {
    depth = 0;
    space = 4;

    pop(nodeType) {
        if (nodeType === NodeType.object)
            this.depth--;
        delete this.node;
        return true;
    }

    push(nodeType) {
        if (nodeType === NodeType.object)
            this.depth++;
        this.node = this.makeNode(nodeType);
    }

    setText(text) {
        if (this.node.type === NodeType.field)
            trace(`${" ".repeat((this.depth - 1) * this.space)}${text}\n`);
    }
}

const vpt = new TracerVPT({});
const parser = new JSONParser({ keys, vpt });

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
