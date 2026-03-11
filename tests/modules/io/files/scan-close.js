/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const s = files.scan();
while (!s.next().done)
	;
assert(s.next().done);
assert(s.return().done);
