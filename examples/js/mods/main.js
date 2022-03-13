import config from "mc/config";
import ArchiveFileMapping from "ArchiveFileMapping";

const oops = 0;
const wow = 1;
const root = "/Users/ps/Projects/moddable/build/bin/mac/mc/debug/"; //config.file.root;

const archiveFileMapping = new ArchiveFileMapping(root + "test/mc.xsa");
const archive = archiveFileMapping.archive;
// const modulePaths = archiveFileMapping.modulePaths;
// const resourcePaths = archiveFileMapping.resourcePaths;
// trace(`### modulePaths\n${ modulePaths.join("\n") }\n`);
// trace(`### resourcePaths\n${ resourcePaths.join("\n") }\n`);
// trace(`### \n`);

const c = new Compartment({}, {
	test: { archive, path:"mod", meta: { archive } }
});
const ns = c.importNow("test");

trace(`${ ns.default(1, 2) }\n`);
