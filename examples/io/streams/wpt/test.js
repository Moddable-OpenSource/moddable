import * as streams from "../modules/streams.js";
for (let key in streams)
	globalThis[key] = streams[key];

globalThis.console = {
	debug: print,
	log: print,
	warn: print,
};
globalThis.createBuffer = function(type, length, opts) {
    if (type === "ArrayBuffer")
		return new ArrayBuffer(length, opts);
    if (type === "SharedArrayBuffer")
		return new SharedArrayBuffer(length, opts);
	throw new Error("type has to be ArrayBuffer or SharedArrayBuffer");
}
globalThis.garbageCollect = async () => {
	return $262.gc();
}
globalThis.self = globalThis;
globalThis.structuredClone = function(buffer) {
	if (buffer instanceof ArrayBuffer)
		return buffer.transfer();
	debugger
}
globalThis.MessageChannel = class {
	constructor() {
		this.port1 = {
			postMessage(buffer, array) {
				if (buffer instanceof ArrayBuffer)
					return buffer.transfer();
				debugger
			}
		}
	} 
}

runScript("./resources/testharness.js");
runScript("./streams/resources/test-utils.js");
runScript("./streams/resources/rs-utils.js");
runScript("./streams/resources/recording-streams.js");
runScript("./streams/resources/rs-test-templates.js");
runScript("./encoding/streams/resources/readable-stream-from-array.js");
runScript("./encoding/streams/resources/readable-stream-to-array.js");

let testCount = 0;
let passedCount = 0;
add_completion_callback((tests, status) => {
	for (let test of tests) {
		testCount++;
		if (test.status)
			print(`${test.name}: ${test.message}`);
		else
			passedCount++;
	}
	print(`# ${passedCount}/${testCount}`);
});
setup({ debug:false });
