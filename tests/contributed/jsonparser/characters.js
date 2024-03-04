/*---
flags: [module]
---*/

// Copyright (c) 2023-2024 Mark Wharton
// https://opensource.org/license/mit/

import { JSONParser } from "jsonparser";

const parser = new JSONParser();

const escapeSequences = '"\\u3053\\u3093\\u306b\\u3061\\u306f\\u4e16\\u754c\\uff01"';
Array.from(escapeSequences).forEach(character => parser.receive(character));

assert.sameValue(parser.status, JSONParser.success);
assert.sameValue(parser.root.value, "こんにちは世界！");

parser.close();
