/*---
description:
flags: [module]
---*/

import files, {capabilities} from "./files_FIXTURE.js";

if (!capabilities.createLink) {
	trace("readLink unsupported - skipping tests\n");
}
else {
	const target = "readlink-target.bin";
	const link = "readlink-link.bin";

	files.delete(target);
	files.delete(link);

	files.openFile({path: target, mode: "w+"}).close();

	// round-trip: readLink returns the target passed to createLink
	files.createLink(link, target);
	assert.sameValue(files.readLink(link), target);

	// readLink on a regular file throws (not a symlink)
	assert.throws(Error, () => files.readLink(target));

	// readLink on a missing path throws
	files.delete(link);
	assert.throws(Error, () => files.readLink(link));

	// bad args
	assert.throws(SyntaxError, () => files.readLink.call(new $TESTMC.HostObject, target));

	files.delete(target);
}
