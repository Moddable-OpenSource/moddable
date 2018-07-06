export default class Test262 {
	constructor() {
		this.base = "";
		this.filter = "";
		this.harness = "";
		this.machine = null;
		this.metadata = null;
		this.test = "";
		this.tests = [];
	}
	doTest() {
		let machine = this.machine;
		let tests = this.tests;
		while (tests.length) {
			let path = this.tests.shift();
			this.metadata = this.getData(path);
			if (this.metadata.strict) {
				this.metadata.paths = this.metadata.paths.map(name => system.buildPath(this.harness, name));
				this.metadata.paths.push(path);
				this.metadata.test = path;
				machine.behavior.onBubbled(this, path, 0, "test262", 1, path.slice(this.test.length + 1));
				path = this.metadata.paths.shift();
				machine.doScript(path);
				break;
			}
		}
	}
	getData(path) @ "test262_getData"
	getTests(info, tests) {
		if (info.directory) {
			let iterator = new system.DirectoryIterator(info.path);
			while (info = iterator.next()) {
				this.getTests(info, tests);
			}
		}
		else {
			let name = info.name;
			if (name.endsWith(".js") && !name.endsWith("_FIXTURE.js"))
				tests.push(info.path);
		}
	}
	onMessage(machine, message) {
		if (message == ">") {
			this.machine = machine;
			this.doTest();
		}
		else if (message == "<") {
			let metadata = this.metadata;
			if (metadata.paths.length) {
				let path = metadata.paths.shift();
				machine.doScript(path);
			}
			else {
				if (metadata.negative) {
					machine.behavior.onBubbled(this, metadata.test, 0, "test262", 2, `Expected ${metadata.negative} but got no errors`);
				}
				else {
					machine.behavior.onBubbled(this, metadata.test, 0, "test262", 2, "OK");
				}
				machine.doAbort();
				this.machine = null;
			}
		}
		else {
			let metadata = this.metadata;
			if (metadata.paths.length) {
				machine.behavior.onBubbled(this, metadata.test, 0, "test262", 2, message);
			}
			else {
				if (message.indexOf(metadata.negative) == 0) {
					machine.behavior.onBubbled(this, metadata.test, 0, "test262", 2, "OK");
				}
				else {
					machine.behavior.onBubbled(this, metadata.test, 0, "test262", 2, `Expected ${metadata.negative} but got ${message}`);
				}
			}
			machine.doAbort();
			this.machine = null;
		}
	}
	onPreferencesChanged() {
		let tests = this.tests = [];
		if (this.base && this.filter) {
			this.harness = system.buildPath(this.base, "harness");
			this.test = system.buildPath(this.base, "test");
			let info = system.getFileInfo(this.harness);
			if (info && info.directory) {
				info = system.getFileInfo(this.test);
				if (info && info.directory) {
					info = system.getFileInfo(this.test + "/" + this.filter);
					if (info) {
						this.getTests(info, tests);
						tests.sort();
					}
				}
			}
		}
		if (this.machine)
			this.doTest();
	}
}
