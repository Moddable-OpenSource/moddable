/*---
flags: [module]
---*/

// Copyright (c) 2023-2024 Mark Wharton
// https://opensource.org/license/mit/

import { JSONParser } from "jsonparser";

const parser1 = new JSONParser();

const fullSurrogateEscapeSequence = '"\\uD834\\uDD1E"';
parser1.receive(fullSurrogateEscapeSequence);

assert.sameValue(parser1.status, JSONParser.success);
assert.sameValue(parser1.root.value, "𝄞");

const parser2 = new JSONParser();

const halfSurrogateEscapeSequence = '"\\uD834"';
parser2.receive(halfSurrogateEscapeSequence);

assert.sameValue(parser2.status, JSONParser.failure); // failure!

parser2.close();
parser1.close();
