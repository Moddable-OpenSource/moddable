/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";

const render = new Poco(screen);
render.close();
render.close();		// close twice is safe

assert.throws(SyntaxError, () => render.begin(0, 0, screen.width, screen.height), "unavailable after close");
assert.throws(SyntaxError, () => render.width, "unavailable after close");
