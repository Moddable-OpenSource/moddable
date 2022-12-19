/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";

assert.throws(TypeError, () => new Poco, "pixelsOut required");
assert.throws(TypeError, () => Poco(), "called as function");
let render = new Poco(screen);		// options parameteer optional

assert.sameValue(screen, render.pixelsOut, "pixelsOut is on return instance");
assert.sameValue(screen.width, render.width, "Poco width matches pixelsOut");
assert.sameValue(screen.height, render.height, "Poco height matches pixelsOut");
