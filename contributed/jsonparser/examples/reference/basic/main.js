// Copyright (c) 2023-2024 Mark Wharton
// https://opensource.org/license/mit/

import { JSONParser } from "jsonparser";

const parser = new JSONParser();

// Test JSON data.
const testData = '{"name": "Jethro", "age": 17, "city": "Caloundra"}';

// Expected parsed result.
const expectedResult = {
    name: "Jethro",
    age: 17,
    city: "Caloundra"
};

// Parse the JSON data using your parser.
parser.receive(testData);

// Compare the actual result with the expected result.
const actualResult = parser.root.value;
if (JSON.stringify(actualResult) === JSON.stringify(expectedResult))
    trace("JSON parsing test passed!\n");
else {
    trace("JSON parsing test failed!\n");
    trace(`Expected: ${JSON.stringify(expectedResult)}\n`);
    trace(`Actual: ${JSON.stringify(actualResult)}\n`);
}

// Close the parsing process and deallocate associated memory.
parser.close();

debugger;
