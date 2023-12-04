# JSONParser

Mark Wharton, December 20, 2023

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
- `keys`: An optional property representing a list of keys used during parsing. This feature is functionally equivalent to the Moddable SDK's JSON.parse() keys.
- `matcher`: An optional property representing a custom matcher for parsing.

##### Public Properties and Methods
- `data`: Retrieves data from the Virtual Parse Tree (VPT) associated with a matcher.
- `root`: Retrieves the root from the Virtual Parse Tree (VPT) using the default tree behavior.
- `status`: Retrieves the parsing result status. This can be called at any time before closing the parsing process.
- `close()`: Closes the parsing process and deallocates the associated memory. Subsequent calls to parser methods will throw an exception.
- `receive(string, start, end)`: Callback for receiving data strings during parsing. The optional start and end values are functionally equivalent to string slice(), used to extract a part of the string. Returns the number of characters parsed.

#### Matcher Class

The Matcher class is used for matching nodes. It is configured with options to begin and match functions.

##### Constructor Options
- `begin`: A function called at the beginning of parsing.
- `match`: A function called when nodes are popped during parsing.

#### Node Class

The Node class represents a basic node in the parse tree. It is part of the Virtual Parse Tree (VPT) structure and can hold data associated with the node.

##### Public Properties and Methods
- `data`: Retrieves or initializes the data associated with the node.
- `next`: Limited to field nodes with primitive values, retrieves the next node in the parse tree; returns undefined otherwise.
- `text`: Limited to field, number, and string nodes, retrieves the text associated with the node; returns undefined otherwise.
- `up(count, nodeType, nodeText)`: Moves up the parse tree by a specified count of nodes, filtering by type and text if provided.

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

Script
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

Output
```text
Hello, Moddable SDK!
success!
```

### Advanced Usage

1. Import the necessary classes: `JSONParser`, `Matcher`, `NodeType`.
2. Customize the parsing behavior by implementing a `Matcher` with appropriate functions.
3. Create an instance of `JSONParser`, passing the `matcher` instance, to initiate the parsing process.
4. Use the provided methods to manipulate the parse tree and extract information.

Explore advanced functionality with the following example:
```json
{
  "workingHours": {
    "daysOfWeek": [
      "monday",
      "tuesday",
      "wednesday",
      "thursday",
      "friday"
    ],
    "startTime": "08:00:00.0000000",
    "endTime": "17:00:00.0000000",
    "timeZone": {
      "name": "Pacific Standard Time"
    }
  }
}
```

The Virtual Parse Tree (VPT) process:

1. VPT constructor creates a root node.
1. Parser reads `{` (line 1) and pushes an object node.
1. Parser reads `workingHours` (line 2) and pushes a field node.
1. Parser reads `{` (line 2) and pushes an object node.
1. Parser reads `daysOfWeek` (line 3) and pushes a field node.
1. Parser reads `[` (line 3) and pushes an array node.
1. Parser reads `monday` (line 4), pushes a string node, sets the node text, and then pops it.
1. Parser reads `tuesday` (line 5), pushes a string node, sets the node text, and then pops it.
1. Parser reads `wednesday` (line 6), pushes a string node, sets the node text, and then pops it.
1. Parser reads `thursday` (line 7), pushes a string node, sets the node text, and then pops it.
1. Parser reads `friday` (line 8), pushes a string node, sets the node text, and then pops it.
1. Parser reads `]` (line 9) and pops the array node.
1. Parser reads `,` (line 9) and pops the field node (`daysOfWeek`).
1. Parser reads `startTime` (line 10) and pushes a field node.
1. Parser reads `08:00:00.0000000` (line 10), pushes a string node, sets the node text, and then pops it.
1. Parser reads `,` (line 10) and pops the field node (`startTime`).
1. Parser reads `endTime` (line 11) and pushes a field node.
1. Parser reads `17:00:00.0000000` (line 11), pushes a string node, sets the node text, and then pops it.
1. Parser reads `,` (line 11) and pops the field node (`endTime`).
1. Parser reads `timeZone` (line 12) and pushes a field node.
1. Parser reads `{` (line 12) and pushes an object node.
1. Parser reads `name` (line 13) and pushes a field node.
1. Parser reads `Pacific Standard Time` (line 13), pushes a string node, sets the node text, and then pops it.
1. Parser reads `}` (line 14), pops the field node (`name`), and then pops the object node.
1. Parser reads `}` (line 15), pops the field node (`timeZone`), and then pops the object node.
1. Parser reads `}` (line 16), pops the field node (`workingHours`), and then pops the object node.
1. The VPT node is now the root node, and parsing is complete.

Accompanied by an explanation of the Type, Text, and Active List for each call:

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

\* Special "lookahead" feature (node.next) for added convenience, as illustrated in the [schedule](./examples/schedule/main.js) example.

Matchers are configured with options to begin and match functions:

Script
```javascript
const matcher = new Matcher({
    begin(vpt) {
        ...
    },
    match(vpt, node) {
        ...
    }
});
```

Example to extract start and end times:

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

This example demonstrates the use of "lookahead" to access the next node in the Active List. When performing the pop() operation, the implementation keeps the tail node (node.next) for fields with primitive values and discards it for non-primitive fields, as it doesn't offer any value.

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

Output
```text
monday
tuesday
wednesday
thursday
friday
```

This example demonstrates using the `up()` method to lookup previous nodes in the Active List.

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

This example demonstrates confirming the object `timeZone` as there could be many `name` fields.

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

Transformed Data: `parser.data.workingWeek`
```json
{
  "days": [
    "monday",
    "tuesday",
    "wednesday",
    "thursday",
    "friday"
  ],
  "start": "08:00",
  "stop": "17:00",
  "tz": "Pacific Standard Time"
}
```

> **Note**: Certain transformations within the tree may require aggregating information in a parent node prior to updating the data. Refer to the [schedule](./examples/schedule/main.js) for a concrete example of how this approach can be implemented.

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

