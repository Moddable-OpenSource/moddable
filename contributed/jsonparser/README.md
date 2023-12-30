# JSONParser

Mark Wharton, December 20, 2023\
Revised: December 31, 2023

## JSONParser Module

### Overview

The Moddable SDK JSON Parser module is designed for parsing JSON data streams in embedded systems. It supports character-by-character extraction in C, enabling efficient processing of large files in low-memory environments. JavaScript interfaces offer flexibility for diverse parsing needs, such as using matching functions to extract essential data or constructing a complete JSON tree from the input stream.

### License and Credits

The JSON Parser module and examples are released under the [MIT License](https://opensource.org/license/mit/). Please refer to [LICENSE](./LICENSE) in the source for details. They are contributed to Moddable under the [Contributor License Agreement](../../licenses/readme.md#contributor-license-agreement).

#### Ragel

The JSON Parser module uses Ragel to generate robust parser code. Ragel was designed and developed by [Adrian Thurston](https://github.com/adrian-thurston), and it is known for its efficiency in generating state machine-based code for lexical analyzers and parsers. For more information about using Ragel, refer to the [Ragel State Machine Compiler](http://www.colm.net/open-source/ragel/) documentation.

#### HeliMods

Special thanks to [HeliMods](https://www.helimods.com) and [Marc Treble](https://github.com/mtreble) for initiating and supporting the Meeting Room Display project. It turned out that the project required parsing JSON data streams greater than the available memory on a [Moddable Three](https://www.moddable.com/moddable-three) device. Thanks to [Jethro Wharton](https://github.com/jethrowharton) for developing the Meeting Room Display app and testing this code.

### Classes

#### JSONParser Class

The JSONParser class serves as a JSON parsing engine with customizable parsing options.

##### Constructor Options
- `initialBufferSize`: Initial size of the buffer for storing text, set to 64 by default. The buffer size is doubled each time more memory is needed. If there is not enough memory for the new buffer, an exception is thrown.
- `initialStackDepth`: Initial depth of the stack used for parsing, set to 8 by default. The stack size is doubled each time more memory is needed. If there is not enough memory for the new stack, an exception is thrown.
- `keys`: An optional property representing a list of keys used during parsing. This feature is equivalent to the Moddable SDK's JSON.parse() keys.
- `matcher`: An optional property representing a custom matcher for parsing.
- `patterns`: An optional property representing a list of patterns for parsing.

##### Public Properties and Methods
- `data`: Retrieves data from the Virtual Parse Tree (VPT) associated with a matcher or patterns.
- `root`: Retrieves the root from the Virtual Parse Tree (VPT) using the default tree behavior.
- `status`: Retrieves the parsing result status. This can be called at any time before closing the parsing process.
- `close()`: Closes the parsing process and deallocates the associated memory. Subsequent calls to parser methods will throw an exception.
- `receive(string, start, end)`: Receives data into the parse tree. Optional start and end values act like string slice(), extracting a portion of the string. Returns the parsed character count.

#### Matcher Class

The Matcher class is used for matching nodes. It is configured with options to begin and match functions.

##### Constructor Options
- `begin`: A function called at the beginning of parsing.
- `match`: A function called when nodes are popped during parsing.

#### Node Class

The Node class represents a basic node in the parse tree. It is part of the Virtual Parse Tree (VPT) structure and can hold data associated with the node.

##### Public Properties and Methods
- `data`: Retrieves or initializes user data associated with the node.
- `next`: Limited to field nodes with primitive values, retrieves the next node in the parse tree; returns undefined otherwise.
- `text`: Limited to field, number, and string nodes, retrieves the text associated with the node; returns undefined otherwise.
- `up(count, nodeType, nodeText)`: Moves up the parse tree by a specified count of nodes, filtering by type and text if provided.

#### Pattern Class

The Pattern class is used for matching nodes with a pattern. It is configured with options to match and setup functions and a pattern value string.

##### Constructor Options
- `match`: A function called when a node matching the pattern is popped during parsing.
- `setup`: A function called when a node matching the pattern is pushed or text is set during parsing.
- `value`: The pattern value string follows the format `type(:text)?(/type(:text)?)*`.

> **Note**: The characters `/` and `:` in the pattern value act as delimiters, and the behavior for field names containing these characters is undefined.

### Constants

Constants for parsing result status:
- `JSONParser.failure`: Represents parsing failure.
- `JSONParser.receive`: Represents an ongoing parsing process.
- `JSONParser.success`: Represents successful parsing.

### Node Types

The module defines an enumeration for different node types in the parse tree:
- `NodeType.null`
- `NodeType.false`
- `NodeType.true`
- `NodeType.number`
- `NodeType.string`
- `NodeType.array`
- `NodeType.object`
- `NodeType.field`
- `NodeType.root`

### Basic Usage

1. Import the necessary classes: `JSONParser`.
2. Create an instance of `JSONParser` to initiate the parsing process.
3. Use the provided methods to receive data into the parse tree and, on success, extract information. 

Example Script
```javascript
import { JSONParser } from "jsonparser";

const parser = new JSONParser();

const data = '"Hello, Moddable SDK!"';

parser.receive(data);

let result = parser.status;
if (result === JSONParser.success)
    trace(`${parser.root.value}\nsuccess!\n`);
else
    trace(`result: ${result}\n`);

parser.close();
```

Example Output
```text
Hello, Moddable SDK!
success!
```

### Matcher Usage

1. Import the necessary classes: `JSONParser`, `Matcher`, and `NodeType`.
2. Customize the parsing behavior by implementing a `Matcher` with appropriate functions.
3. Create an instance of `JSONParser`, passing the `matcher` instance, to initiate the parsing process.
4. Use provided methods to receive data into the parse tree and handle 'begin' and 'match' events for extraction and construction.

Example Script
```javascript
import { JSONParser, Matcher, NodeType } from "jsonparser";

const matcher = new Matcher({
    begin(vpt) {
        vpt.data = {};
    },
    match(vpt, node) {
        switch (node.type) {
            case NodeType.string:
                vpt.data.value = node.text;
                break;
        }
    }
});

const parser = new JSONParser({ matcher });

const data = '"Hello, Moddable SDK!"';

parser.receive(data);

let result = parser.status;
if (result === JSONParser.success)
    trace(`${parser.data.value}\nsuccess!\n`);
else
    trace(`result: ${result}\n`);

parser.close();
```

Example Output
```text
Hello, Moddable SDK!
success!
```

Explore advanced functionality with the following JSON example:
```json
 1 {
 2   "workingHours": {
 3     "daysOfWeek": [
 4       "monday",
 5       "tuesday",
 6       "wednesday",
 7       "thursday",
 8       "friday"
 9     ],
10     "startTime": "08:00:00.0000000",
11     "endTime": "17:00:00.0000000",
12     "timeZone": {
13       "name": "Pacific Standard Time"
14     }
15   }
16 }
```

> **Note**: Line numbers for explanation only.

The Virtual Parse Tree (VPT) process:

1. VPT constructor creates a root node.
2. Parser reads `{` _(line 1)_ and pushes an object node.
3. Parser reads `workingHours` _(line 2)_ and pushes a field node.
4. Parser reads `{` _(line 2)_ and pushes an object node.
5. Parser reads `daysOfWeek` _(line 3)_ and pushes a field node.
6. Parser reads `[` _(line 3)_ and pushes an array node.
7. Parser reads `monday` _(line 4)_, pushes a string node, sets the node text, and then pops it _(#1)_.
8. Parser reads `tuesday` _(line 5)_, pushes a string node, sets the node text, and then pops it _(#2)_.
9. Parser reads `wednesday` _(line 6)_, pushes a string node, sets the node text, and then pops it _(#3)_.
10. Parser reads `thursday` _(line 7)_, pushes a string node, sets the node text, and then pops it _(#4)_.
11. Parser reads `friday` _(line 8)_, pushes a string node, sets the node text, and then pops it _(#5)_.
12. Parser reads `]` _(line 9)_ and pops the array node _(#6)_.
13. Parser reads `,` _(line 9)_ and pops the field node _(`daysOfWeek`)_ _(#7)_.
14. Parser reads `startTime` _(line 10)_ and pushes a field node.
15. Parser reads `08:00:00.0000000` _(line 10)_, pushes a string node, sets the node text, and then pops it _(#8)_.
16. Parser reads `,` _(line 10)_ and pops the field node _(`startTime`)_ _(#9)_.
17. Parser reads `endTime` _(line 11)_ and pushes a field node.
18. Parser reads `17:00:00.0000000` _(line 11)_, pushes a string node, sets the node text, and then pops it _(#10)_.
19. Parser reads `,` _(line 11)_ and pops the field node _(`endTime`)_ _(#11)_.
20. Parser reads `timeZone` _(line 12)_ and pushes a field node.
21. Parser reads `{` _(line 12)_ and pushes an object node.
22. Parser reads `name` _(line 13)_ and pushes a field node.
23. Parser reads `Pacific Standard Time` _(line 13)_, pushes a string node, sets the node text, and then pops it _(#12)_.
24. Parser reads `}` _(line 14)_, pops the field node _(`name`)_ _(#13)_, and then pops the object node _(#14)_.
25. Parser reads `}` _(line 15)_, pops the field node _(`timeZone`)_ _(#15)_, and then pops the object node _(#16)_.
26. Parser reads `}` _(line 16)_, pops the field node _(`workingHours`)_ _(#17)_, and then pops the object node _(#18)_.
27. The VPT node is now the root node, and parsing is complete.

Accompanied by an explanation of the Type, Text, and Active List for each call to the Matcher's match function:

| #  | Type     | Text                  | Active List                                                                                                             | → |
|----|----------|-----------------------|-------------------------------------------------------------------------------------------------------------------------|---|
| 1  | `string` | monday                | root ← object ← field workingHours ← object ← field daysOfWeek ← array ← **`string` monday**                            |   |
| 2  | `string` | tuesday               | root ← object ← field workingHours ← object ← field daysOfWeek ← array ← **`string` tuesday**                           |   |
| 3  | `string` | wednesday             | root ← object ← field workingHours ← object ← field daysOfWeek ← array ← **`string` wednesday**                         |   |
| 4  | `string` | thursday              | root ← object ← field workingHours ← object ← field daysOfWeek ← array ← **`string` thursday**                          |   |
| 5  | `string` | friday                | root ← object ← field workingHours ← object ← field daysOfWeek ← array ← **`string` friday**                            |   |
| 6  | `array`  |                       | root ← object ← field workingHours ← object ← field daysOfWeek ← **`array`**                                            |   |
| 7  | `field`  | daysOfWeek            | root ← object ← field workingHours ← object ← **`field` daysOfWeek**                                                    |   |
| 8  | `string` | 08:00:00.0000000      | root ← object ← field workingHours ← object ← field startTime ← **`string` 08:00:00.0000000**                           |   |
| 9  | `field`  | startTime             | root ← object ← field workingHours ← object ← **`field` startTime** → string 08:00:00.0000000                           | * |
| 10 | `string` | 17:00:00.0000000      | root ← object ← field workingHours ← object ← field endTime ← **`string` 17:00:00.0000000**                             |   |
| 11 | `field`  | endTime               | root ← object ← field workingHours ← object ← **`field` endTime** → string 17:00:00.0000000                             | * |
| 12 | `string` | Pacific Standard Time | root ← object ← field workingHours ← object ← field timeZone ← object ← field name ← **`string` Pacific Standard Time** |   |
| 13 | `field`  | name                  | root ← object ← field workingHours ← object ← field timeZone ← object ← **`field` name** → string Pacific Standard Time | * |
| 14 | `object` |                       | root ← object ← field workingHours ← object ← field timeZone ← **`object`**                                             |   |
| 15 | `field`  | timeZone              | root ← object ← field workingHours ← object ← **`field` timeZone**                                                      |   |
| 16 | `object` |                       | root ← object ← field workingHours ← **`object`**                                                                       |   |
| 17 | `field`  | workingHours          | root ← object ← **`field` workingHours**                                                                                |   |
| 18 | `object` |                       | root ← **`object`**                                                                                                     |   |

\* Special _lookahead_ feature (node.next) for added convenience, as illustrated in the [schedule](./examples/schedule/main.js) example.

Example to extract start and end times using the JSON example:

Script
```javascript
const matcher = new Matcher({
    match(vpt, node) {
        switch (node.type) {
            case NodeType.field:
                switch (node.text) {
                    case "startTime":
                        trace(`${node.next.text}\n`);
                        break;
                    case "endTime":
                        trace(`${node.next.text}\n`);
                        break;
                }
                break;
        }
    }
});
```

This example demonstrates the use of _lookahead_ to access the next node in the Active List. When performing the pop() operation, the implementation keeps the tail node (node.next) for fields with primitive values and discards it for non-primitive fields, as it doesn't offer any value.

Output
```text
08:00:00.0000000
17:00:00.0000000
```

Example to extract days of the week:

Script
```javascript
const matcher = new Matcher({
    match(vpt, node) {
        switch (node.type) {
            case NodeType.string:
                if (node.up(2, NodeType.field, "daysOfWeek")) {
                    trace(`${node.text}\n`);
                }
                break;
        }
    }
});
```

This example demonstrates using the `up()` method to lookup previous nodes in the Active List.

Output
```text
monday
tuesday
wednesday
thursday
friday
```

Example to extract the time zone name:

Script
```javascript
const matcher = new Matcher({
    match(vpt, node) {
        switch (node.type) {
            case NodeType.field:
                switch (node.text) {
                    case "name":
                        if (node.up(2, NodeType.field, "timeZone")) {
                            trace(`${node.next.text}\n`);
                        }
                        break;
                }
                break;
        }
    }
});
```

This example demonstrates checking that the `name` field is found within the context of a `timeZone` field.

Output
```text
Pacific Standard Time
```

Complete example to extract and dynamically transform data on the fly:

Script
```javascript
const matcher = new Matcher({
    begin(vpt) {
        vpt.data = {
            workingWeek: {
                days: []
            }
        };
    },
    match(vpt, node) {
        let workingWeek = vpt.data.workingWeek;
        switch (node.type) {
            case NodeType.string:
                if (node.up(2, NodeType.field, "daysOfWeek"))
                    workingWeek.days.push(node.text);
                break;
            case NodeType.field:
                switch (node.text) {
                    case "startTime":
                        workingWeek.start = node.next.text.slice(0, 5);
                        break;
                    case "endTime":
                        workingWeek.stop = node.next.text.slice(0, 5);
                        break;
                    case "name":
                        if (node.up(2, NodeType.field, "timeZone"))
                            workingWeek.tz = node.next.text;
                        break;
                }
                break;
        }
    }
});
```

Parser Data
```json
{
  "workingWeek": {
    "days": [
      "monday",
      "tuesday",
      "wednesday",
      "thursday",
      "friday"
    ],
    "start": "08:00",
    "end": "17:00",
    "tz": "Pacific Standard Time"
  }
}
```

> **Note**: Certain transformations within the tree may require aggregating information in a parent node prior to updating the data. Refer to [schedule](./examples/schedule/main.js) for a concrete example of how this approach can be implemented.

### Pattern Usage

1. Import the necessary classes: `JSONParser` and `Pattern`.
2. Customize parsing by implementing `Patterns` with relevant functions and values.
3. Create an instance of `JSONParser`, passing a list of `patterns`, to initiate the parsing process.
4. Use provided methods to receive data into the parse tree and handle 'setup' and 'match' events for extraction and construction.

Example Script
```javascript
import { JSONParser, Pattern } from "jsonparser";

const patterns = [
    new Pattern({
        match(vpt, node) {
            vpt.data.value = node.text;
        },
        setup(vpt, node) {
            vpt.data = {};
        },
        value: "root/string"
    })
];

const parser = new JSONParser({ patterns });

const data = '"Hello, Moddable SDK!"';

parser.receive(data);

let result = parser.status;
if (result === JSONParser.success)
    trace(`${parser.data.value}\nsuccess!\n`);
else
    trace(`result: ${result}\n`);

parser.close();
```

Example Output
```text
Hello, Moddable SDK!
success!
```

Patterns for the JSON example from 'Matcher Usage' section (see above):

Script
```javascript
const patterns = [
    new Pattern({
        setup(vpt) {
            vpt.data = {
                workingWeek: {
                    days: []
                }
            };
        },
        value: "field:workingHours"
    }),
    new Pattern({
        match(vpt, node) {
            vpt.data.workingWeek.days.push(node.text);
        },
        value: "field:daysOfWeek/array/string"
    }),
    new Pattern({
        match(vpt, node) {
            vpt.data.workingWeek.start = node.text.slice(0, 5);
        },
        value: "field:startTime/string"
    }),
    new Pattern({
        match(vpt, node) {
            vpt.data.workingWeek.end = node.text.slice(0, 5);
        },
        value: "field:endTime/string"
    }),
    new Pattern({
        match(vpt, node) {
            vpt.data.workingWeek.tz = node.text;
        },
        value: "field:timeZone/object/field:name/string"
    })
];
```

Parser Data
```json
{
  "workingWeek": {
    "days": [
      "monday",
      "tuesday",
      "wednesday",
      "thursday",
      "friday"
    ],
    "start": "08:00",
    "end": "17:00",
    "tz": "Pacific Standard Time"
  }
}
```

> **Note**: Certain transformations within the tree may require caching information prior to updating the data. Refer to [pattern](./examples/pattern/main.js) for a concrete example of how this approach can be implemented.

### Code Generation

Regenerate from [source](./jsonparser.rl); avoid direct edits to generated code (except for the recommended update below).

Confirm Ragel version 6.10 (Stable):
```bash
ragel --version
```

Example Output
```text
Ragel State Machine Compiler version 6.10 March 2017
Copyright (c) 2001-2009 by Adrian Thurston
```

Generate `-T1` output style code:
```bash
ragel -T1 jsonparser.rl > jsonparser.c
```

Update `jsonparser.c` by adding `ICACHE_XS6RO_ATTR` and integrating read macros for accessing static const data in ROM. Using the regular expression search and replace feature of your IDE:

Step 1
- Search: `\[\] = \{`
- Replace: `[] ICACHE_XS6RO_ATTR = {`

Step 2
- Search: `(?:\*(_mid) )`
- Replace: `c_read8( $1 )`

Step 3
- Search: `(_JSON_(?:index|key)_offsets\[[^\]]+\])`
- Replace: `c_read16( &$1 )`

Step 4
- Search: `((_JSON_(?:indicies|(?:range|single)_lengths|trans_(?:actions|targs))|_mid)\[[^\]]+\])`
- Replace: `c_read8( &$1 )`

Compile the code and test the changes to confirm that everything works as expected.

### Unit Tests

Unit tests for JavaScript are provided in the [contributed tests](../../tests/contributed/jsonparser) section. To run these tests in the Moddable SDK, please consult the [Moddable SDK Tests](../../documentation/tools/testing.md) documentation.

### Limitations and Known Issues

- ~~The CESU-8 compatibility encoding scheme is not supported.~~
- ~~`\u` hex code value substitution is not yet implemented (value becomes ?).~~
- JSON field names consist of plain text, and special characters are not unescaped.
- ~~Needs ICACHE and read macros to store and access static const data in ROM.~~
- There are no [jsonparser.d.ts](../../typings/jsonparser.d.ts) type definitions ~~and no [jsonparser](../../tests/contributed/jsonparser) tests~~.
- JSON.parse() like reviver functionality is not supported.
- ~~xsUnknownError with and without xsTry/xsCatch.~~
- Some `TODO: review` items to address.

### References

- ECMA-404 The JSON data interchange syntax
  - https://ecma-international.org/publications-and-standards/standards/ecma-404/
  - https://ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf
- ECMA-419 ECMAScript® embedded systems API specification
  - https://ecma-international.org/publications-and-standards/standards/ecma-419/
  - https://ecma-international.org/wp-content/uploads/ECMA-419_2nd_edition_june_2023.pdf
- Ragel State Machine Compiler
  - https://www.colm.net/open-source/ragel/
  - https://www.colm.net/files/ragel/ragel-guide-6.10.pdf

