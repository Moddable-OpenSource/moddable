// Copyright (c) 2023-2024 Mark Wharton
// https://opensource.org/license/mit/

import { JSONParser } from "jsonparser";

const data = '{"Hello, Japanese!":"\\u3053\\u3093\\u306b\\u3061\\u306f\\u4e16\\u754c\\uff01","G clef character":"\\uD834\\uDD1E"}';

const parser = new JSONParser();

parser.receive(data);

let result = parser.status;
if (result === JSONParser.success) {
    trace(parser.root.value["Hello, Japanese!"] + "\n");
    trace(parser.root.value["G clef character"] + "\n");
    trace("success!\n");
}
else
    trace(`result: ${result}\n`);

parser.close();

debugger;
