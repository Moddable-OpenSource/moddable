import {File} from "file";
import config from "mc/config";
import ModArchive from "ModArchive";

const root = "/Users/ps/Projects/moddable/build/bin/mac/mc/debug/"; //config.file.root;

let file = new File(root + "test/mc.xsa", true);
let buffer = file.read(SharedArrayBuffer);

let archive = new ModArchive(buffer);
let modulePaths = archive.modulePaths;
let resourcePaths = archive.resourcePaths;
trace(`### modulePaths\n${ modulePaths.join("\n") }\n`);
trace(`### resourcePaths\n${ resourcePaths.join("\n") }\n`);
trace(`### \n`);

const c = new Compartment({}, {
	test: { archive:archive.buffer, path:"mod", meta: { archive:archive.buffer } }
});
const ns = c.importNow("test");

trace(`${ ns.default(1, 2) }\n`);
