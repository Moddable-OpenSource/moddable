/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/665
flags: [onlyStrict]
---*/

const ws = ["0009", "000A", "000B", "000C", "000D", "0020", "00A0",
  "1680", "2000", "2001", "2002", "2003", "2004", "2005", "2006", "2007", "2008", "2009", "200A",
  "2028", "2029", "202F", "205F", "3000", "FEFF"
]
const rejected = ws.filter(hex => {
  const cp = String.fromCharCode(parseInt(hex, 16));
  try {
    eval("/x/" + cp);
    // No parsing error, suppress code point.
    return false;
  } catch ( ex ) {
    // Parsing error, keep code point.
    return true;
  }
});

assert.sameValue(rejected.length, 0);
