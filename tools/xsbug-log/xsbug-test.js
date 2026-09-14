/*
 * Copyright (c) 2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 *
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
	Test runner for xsdb. A port of the Test262Context in xsbug's Test262Pane.js.

	Drives the test262 and testmc apps (tools/test262, tools/testmc) over the xsbug channel:

	- the app bubbles ">" on conversation "test262" when it is ready for a test
	- the runner sends harness includes and the test as <script>, or the test as <import> for modules
	- the engine asks for imported fixtures with <import path>, answered with <module> (or <go> if missing)
	- the app bubbles "<" when a step completed or $DONE() was called, anything else is an error
	- the runner aborts the app after each test, the app restarts and bubbles ">" again

	Tests run strict or as modules only. Sloppy-only tests (noStrict, raw) are skipped.
*/

const fs = require('node:fs');
const path = require('node:path');

// Features not implemented on device. Matches tools/xsbug/Test262Pane.c.
const skippedFeatures = [
	"Atomics.pause",
	"FinalizationRegistry.prototype.cleanupSome",
	"ShadowRealm",
	"Temporal",
	"arbitrary-module-namespace-names",
	"canonical-tz",
	"decorators",
	"import-assertions",
	"import-defer",
	"joint-iteration",
	"nonextensible-applies-to-private",
	"source-phase-imports",
	"source-phase-imports-module-source",
	"regexp-unicode-property-escapes",
];

// Harness files whose definitions the test apps already provide (tools/test262/harness.js), so they are never sent.
const builtInIncludes = new Set([
	"assert.js",
	"sta.js",
	"doneprintHandle.js",
	"compareArray.js",
]);

// Minimal YAML reader for test262 front matter: scalars, flow sequences, block sequences,
// one level of nested mapping (negative: {phase, type}), and block scalars (description: |).
function parseFrontMatter(source) {
	const begin = source.indexOf("/*---");
	if (begin < 0)
		return null;
	const end = source.indexOf("---*/", begin + 5);
	if (end < 0)
		return null;
	const lines = source.slice(begin + 5, end).split(/\r?\n/);
	const scalar = value => {
		value = value.trim();
		if ((value.length >= 2) && ((value[0] == '"' && value.endsWith('"')) || (value[0] == "'" && value.endsWith("'"))))
			value = value.slice(1, -1);
		return value;
	};
	const flow = value => value.slice(1, -1).split(",").map(scalar).filter(item => item.length);
	const result = {};
	let index = 0;
	while (index < lines.length) {
		const line = lines[index++];
		if (!line.trim() || line.trim().startsWith("#"))
			continue;
		const match = line.match(/^(\S[^:]*):\s*(.*)$/);
		if (!match)
			continue;
		const key = match[1].trim();
		let value = match[2].trim();
		if (value.startsWith("#"))
			value = "";
		if (value.startsWith("[") && value.endsWith("]")) {
			result[key] = flow(value);
			continue;
		}
		if (value && (value != "|") && (value != ">") && (value != "|-") && (value != ">-")) {
			result[key] = scalar(value);
			continue;
		}
		// block: collect indented lines
		const block = [];
		while (index < lines.length) {
			const next = lines[index];
			if (next.trim() && !/^\s/.test(next))
				break;
			block.push(next);
			index++;
		}
		const items = block.map(item => item.trim()).filter(item => item.length && !item.startsWith("#"));
		if (!items.length)
			result[key] = "";
		else if (items.every(item => item.startsWith("- ")))
			result[key] = items.map(item => scalar(item.slice(2)));
		else if (!value && items.every(item => /^[A-Za-z_][\w-]*:(\s|$)/.test(item))) {
			const mapping = {};
			for (const item of items) {
				const colon = item.indexOf(":");
				mapping[item.slice(0, colon).trim()] = scalar(item.slice(colon + 1));
			}
			result[key] = mapping;
		}
		else
			result[key] = items.join("\n");
	}
	return result;
}

// Equivalent of Test262Context_getMetadata: decide how (and whether) to run a test.
function getMetadata(source) {
	const metadata = {
		includes: [],
		negative: null,
		async: false,
		strict: true,
		module: false,
		skip: null,
	};
	const yaml = parseFrontMatter(source);
	if (!yaml)
		return metadata;
	if (Array.isArray(yaml.includes))
		metadata.includes = yaml.includes.filter(include => !builtInIncludes.has(include));
	if (yaml.negative && (typeof yaml.negative == "object") && yaml.negative.type)
		metadata.negative = yaml.negative.type;
	let flags = yaml.flags;
	if (!Array.isArray(flags))
		flags = [];
	for (const flag of flags) {
		switch (flag) {
			case "onlyStrict":
				break;
			case "noStrict":
			case "raw":
				metadata.strict = false;
				metadata.skip ??= `flag: ${flag}`;
				break;
			case "module":
				metadata.strict = false;
				metadata.module = true;
				break;
			case "async":
				metadata.async = true;
				break;
			case "CanBlockIsTrue":
				metadata.strict = false;
				metadata.module = false;
				metadata.skip ??= `flag: ${flag}`;
				break;
			case "CanBlockIsFalse":
			case "generated":
			case "non-deterministic":
				break;
			default:
				metadata.skip ??= `unknown flag: ${flag}`;
				break;
		}
	}
	if (Array.isArray(yaml.features)) {
		for (const feature of yaml.features) {
			if (skippedFeatures.includes(feature)) {
				metadata.strict = false;
				metadata.module = false;
				metadata.skip ??= `feature: ${feature}`;
				break;
			}
		}
	}
	if (!metadata.strict && !metadata.module)
		metadata.skip ??= "not strict";
	return metadata;
}

// xsdb has no shell in front of it, so expand a leading ~ in paths typed by the user.
function expandHome(name) {
	if ((name === "~") || name.startsWith("~/") || name.startsWith("~\\"))
		return require("node:os").homedir() + name.slice(1);
	return name;
}

function isTestFile(name) {
	return name.endsWith(".js") && !name.endsWith("FIXTURE.js");
}

// Yields test file paths in sorted order, depth first. Same order as xsbug.
function* iterateTests(root) {
	const info = fs.statSync(root, { throwIfNoEntry: false });
	if (!info)
		return;
	if (!info.isDirectory()) {
		if (isTestFile(path.basename(root)))
			yield root;
		return;
	}
	const names = fs.readdirSync(root).sort();
	for (const name of names) {
		const child = path.join(root, name);
		let stat;
		try {
			stat = fs.statSync(child);
		}
		catch {
			continue;
		}
		if (stat.isDirectory())
			yield* iterateTests(child);
		else if (isTestFile(name))
			yield child;
	}
}

class TestRunner {
	// options.output(event, data, text): report to the user (text or JSON, depending on xsdb output mode)
	constructor(options = {}) {
		this.output = options.output ?? ((event, data, text) => console.log(text));
		this.moddable = options.moddable ?? process.env.MODDABLE;
		this.project = options.project ?? process.env.XSBUG_PROJECT;
		this.test262 = options.test262 ?? null;			// path to a test262 repository
		this.timeout = options.timeout ?? 60;			// seconds
		// Report file directory: the project directory by default (next to .xsdb.json); null disables the report file.
		this.logDirectory = (options.logDirectory === undefined) ? (this.project ?? process.cwd()) : options.logDirectory;
		this.machine = null;
		this.iterator = null;
		this.current = null;
		this.watchdog = null;
		this.reset();
	}
	reset() {
		this.report = {
			path: "",
			root: "",
			harness: "",
			logPath: "",
			failed: 0,
			passed: 0,
			skipped: 0,
			started: 0,
			items: [],
		};
	}
	get running() {
		return this.iterator != null;
	}

	// Where tests may live, in order: $MODDABLE/tests, the test262 repository, the project, the current directory.
	get roots() {
		const roots = [];
		if (this.moddable)
			roots.push(path.join(this.moddable, "tests"));
		if (this.test262)
			roots.push(path.join(this.test262, "test"));
		if (this.project)
			roots.push(this.project);
		roots.push(process.cwd());
		return roots;
	}
	// Directories that hold tests, for completing the argument of the test command.
	get testRoots() {
		return this.roots.slice(0, this.test262 ? 2 : 1);
	}
	// Tab completion for "test <path>": directories (with a trailing separator) and test files matching the prefix.
	complete(prefix) {
		const name = expandHome(prefix);
		const slash = Math.max(name.lastIndexOf("/"), name.lastIndexOf(path.sep));
		const directory = name.slice(0, slash + 1);
		const base = name.slice(slash + 1);
		const parents = path.isAbsolute(directory) ? [directory] : this.testRoots.map(root => path.join(root, directory));
		const results = new Set();
		for (const parent of parents) {
			let entries;
			try {
				entries = fs.readdirSync(parent, { withFileTypes: true });
			}
			catch {
				continue;
			}
			for (const entry of entries) {
				if (!entry.name.startsWith(base) || entry.name.startsWith("."))
					continue;
				if (entry.isDirectory())
					results.add(prefix.slice(0, prefix.length - base.length) + entry.name + "/");
				else if (isTestFile(entry.name))
					results.add(prefix.slice(0, prefix.length - base.length) + entry.name);
			}
		}
		return [...results].sort();
	}
	infoTest262() {
		const root = this.test262;
		const data = { test262: root, harness: root ? path.join(root, "harness") : this.resolveHarness(null) };
		const text = root ? `test262 repository: ${root}` : `test262 repository: not set (harness includes come from ${data.harness ?? "nowhere"})`;
		this.output("info_test262", data, text);
	}
	resolve(name) {
		name = expandHome(name);
		if (path.isAbsolute(name)) {
			if (!fs.existsSync(name))
				return null;
			const root = this.roots.find(root => name.startsWith(root + path.sep) || (name == root)) ?? path.dirname(name);
			return { path: name, root };
		}
		for (const root of this.roots) {
			const candidate = path.join(root, name);
			if (fs.existsSync(candidate))
				return { path: candidate, root };
		}
		return null;
	}
	// Harness includes come from the test262 repository when configured, else from the copies in tools/test262/harness.
	resolveHarness(testPath) {
		const candidates = [];
		if (this.test262)
			candidates.push(path.join(this.test262, "harness"));
		if (testPath) {
			// a test inside some test262 checkout: prefer its own harness
			const index = testPath.lastIndexOf(path.sep + "test" + path.sep);
			if (index > 0)
				candidates.push(path.join(testPath.slice(0, index), "harness"));
		}
		if (this.moddable)
			candidates.push(path.join(this.moddable, "tools", "test262", "harness"));
		return candidates.find(candidate => fs.existsSync(path.join(candidate, "propertyHelper.js"))) ?? null;
	}

	start(name) {
		if (this.running)
			this.stop("restarted");
		const resolved = this.resolve(name);
		if (!resolved) {
			this.output("test_error", { path: name, message: `Not found: ${name}` }, `Not found: ${name}. Looked in ${this.roots.join(", ")}`);
			return false;
		}
		this.reset();
		const report = this.report;
		report.path = resolved.path;
		report.root = resolved.root;
		report.harness = this.resolveHarness(resolved.path) ?? "";
		report.started = Date.now();
		this.openLog();
		this.iterator = iterateTests(resolved.path);
		this.output("test_start", { path: report.path, root: report.root, harness: report.harness, log: report.logPath }, `Testing ${this.nameOf(report.path) || "."} (harness: ${report.harness || "none"})`);
		if (this.machine)
			this.step();
		else
			this.output("test_waiting", { path: report.path }, "Waiting for the test app to connect...");
		return true;
	}
	stop(reason) {
		if (!this.running)
			return false;
		this.clearWatchdog();
		if (this.current) {
			this.fail(reason ?? "stopped");
			const machine = this.machine;
			this.current = null;
			this.machine = null;
			if (machine?.connected)
				machine.doAbort();
		}
		this.finish(reason);
		return true;
	}
	finish(reason) {
		this.iterator = null;
		this.current = null;
		const report = this.report;
		const seconds = (Date.now() - report.started) / 1000;
		const total = report.failed + report.passed + report.skipped;
		const name = this.nameOf(report.path) || ".";
		let text = `${name}: ${total} tests, FAIL ${report.failed}, PASS ${report.passed}, SKIP ${report.skipped} (${seconds.toFixed(1)}s)`;
		if (reason)
			text += ` [${reason}]`;
		this.closeLog(total);
		if (report.logPath)
			text += `\nReport: ${report.logPath}`;
		this.output("test_summary", { path: report.path, total, failed: report.failed, passed: report.passed, skipped: report.skipped, seconds, reason, log: report.logPath }, text);
	}

	nameOf(testPath) {
		const root = this.report.root;
		if (root && testPath.startsWith(root + path.sep))
			return testPath.slice(root.length + 1).split(path.sep).join("/");
		return testPath;
	}
	result(status, reason) {
		const current = this.current;
		const item = { path: current.path, name: current.name, status };
		if (reason)
			item.reason = reason;
		this.report.items.push(item);
		let text = `${status.toUpperCase()} ${current.name}`;
		if (reason)
			text += `: ${reason}`;
		this.output("test_result", item, text);
	}
	pass() {
		this.report.passed++;
		this.result("pass");
	}
	fail(reason) {
		this.report.failed++;
		this.result("fail", reason);
		this.appendLog(`${this.current.path}: ${reason}\n`);
	}

	// Report file, in the same format as xsbug: a summary line, a blank line, then one line per failure.
	openLog() {
		const report = this.report;
		report.logPath = "";
		if (!this.logDirectory)
			return;
		let name = this.nameOf(report.path).replace(/[\/\\]/g, "-");
		if (!name || (name == "."))
			name = path.basename(report.path) || "tests";
		const logPath = path.join(this.logDirectory, name + ".txt");
		try {
			fs.writeFileSync(logPath, "");
			report.logPath = logPath;
		}
		catch (e) {
			this.output("test_error", { message: `cannot write report ${logPath}: ${e.message}` }, `Cannot write report ${logPath}: ${e.message}`);
		}
	}
	appendLog(text) {
		if (!this.report.logPath)
			return;
		try {
			fs.appendFileSync(this.report.logPath, text);
		}
		catch {
		}
	}
	closeLog(total) {
		const report = this.report;
		if (!report.logPath)
			return;
		try {
			const failures = fs.readFileSync(report.logPath, "utf8");
			fs.writeFileSync(report.logPath, `${report.path}: ${total} FAIL ${report.failed} PASS ${report.passed} SKIP ${report.skipped}\n\n${failures}`);
		}
		catch {
		}
	}
	skip(reason) {
		this.report.skipped++;
		this.result("skip", reason);
	}

	// Find the next runnable test and send it to the app.
	step() {
		const machine = this.machine;
		let next;
		while (!(next = this.iterator.next()).done) {
			const testPath = next.value;
			let source;
			try {
				source = fs.readFileSync(testPath, "utf8");
			}
			catch (e) {
				this.current = { path: testPath, name: this.nameOf(testPath) };
				this.fail(`cannot read: ${e.message}`);
				this.current = null;
				continue;
			}
			const metadata = getMetadata(source);
			this.current = { path: testPath, name: this.nameOf(testPath), metadata, source };
			if (metadata.skip) {
				this.skip(metadata.skip);
				this.current = null;
				continue;
			}
			const harness = this.report.harness;
			const missing = metadata.includes.find(include => !harness || !fs.existsSync(path.join(harness, include)));
			if (missing) {
				this.fail(`harness include not found: ${missing}`);
				this.current = null;
				continue;
			}
			this.current.paths = metadata.includes.map(include => path.join(harness, include));
			this.current.paths.push(testPath);
			this.current.started = Date.now();
			this.send(machine);
			return;
		}
		this.finish();
	}
	// Send the next script of the current test. The last one is the test itself.
	send(machine) {
		const current = this.current;
		const metadata = current.metadata;
		const scriptPath = current.paths.shift();
		this.armWatchdog();
		if (current.paths.length == 0) {
			if (metadata.module)
				machine.doImport(TestRunner.toImportPath(scriptPath), metadata.async);
			else
				machine.doScript(scriptPath, metadata.async, current.source);
		}
		else
			machine.doScript(scriptPath, false, fs.readFileSync(scriptPath, "utf8"));
	}
	// The test is over: forget the machine and restart the app.
	end(machine) {
		this.clearWatchdog();
		this.current = null;
		this.machine = null;
		if (machine.connected)
			machine.doAbort();
		this.armWatchdog();		// waiting for the app to restart
	}

	// Bubble on conversation "test262" from the app.
	onMessage(machine, message) {
		if (message == ">") {
			this.clearWatchdog();
			this.machine = machine;
			if (this.running)
				this.step();
			return;
		}
		if (!this.running)
			return;
		const current = this.current;
		if (message == "<") {
			if (!current) {
				this.machine = null;
				machine.doAbort();
				return;
			}
			if (current.paths.length > 0) {
				this.send(machine);
				return;
			}
			if (current.metadata.negative)
				this.fail(`Expected ${current.metadata.negative} but got no errors`);
			else
				this.pass();
			this.end(machine);
			return;
		}
		if (current) {
			const negative = current.metadata.negative;
			if (current.paths.length || !negative)
				this.fail(message);
			else if (!message.startsWith(negative))
				this.fail(`Expected ${negative} but got ${message}`);
			else
				this.pass();
		}
		this.end(machine);
	}
	// The engine asks for the source of a module imported by the test (fixtures).
	onImport(machine, importPath) {
		TestRunner.answerImport(machine, importPath);
	}
	onDisconnected(machine) {
		if (this.machine != machine)
			return;
		this.machine = null;
		if (this.current) {
			this.fail("Unhandled exception");
			this.current = null;
			this.clearWatchdog();
			this.armWatchdog();		// waiting for the app to restart
		}
	}

	armWatchdog() {
		this.clearWatchdog();
		if (!(this.timeout > 0))
			return;
		this.watchdog = setTimeout(() => this.onWatchdog(), this.timeout * 1000);
	}
	clearWatchdog() {
		if (this.watchdog) {
			clearTimeout(this.watchdog);
			this.watchdog = null;
		}
	}
	onWatchdog() {
		this.watchdog = null;
		if (!this.running)
			return;
		const machine = this.machine;
		if (this.current) {
			if (machine?.broken) {		// stopped in the debugger: not a timeout
				this.armWatchdog();
				return;
			}
			this.fail(`timeout after ${this.timeout} seconds`);
			this.end(machine);
			return;
		}
		this.stop(`test app did not restart within ${this.timeout} seconds`);
	}

	info() {
		const report = this.report;
		const data = {
			running: this.running,
			path: report.path,
			harness: report.harness,
			test262: this.test262,
			timeout: this.timeout,
			log: report.logPath,
			logDirectory: this.logDirectory,
			failed: report.failed,
			passed: report.passed,
			skipped: report.skipped,
			failures: report.items.filter(item => item.status == "fail"),
		};
		if (this.current)
			data.current = this.current.name;
		const lines = [];
		if (report.path) {
			lines.push(`${this.running ? "Running" : "Tested"} ${this.nameOf(report.path) || "."}: FAIL ${report.failed}, PASS ${report.passed}, SKIP ${report.skipped}`);
			if (data.current)
				lines.push(`  current: ${data.current}`);
			for (const item of data.failures)
				lines.push(`  FAIL ${item.name}: ${item.reason}`);
		}
		else
			lines.push("No tests run.");
		lines.push(`Test roots: ${this.roots.join(", ")}`);
		lines.push(`test262 repository: ${this.test262 ?? "not set"}`);
		lines.push(`Test timeout: ${this.timeout} seconds`);
		lines.push(`Test reports: ${this.logDirectory ?? "off"}${report.logPath ? ` (last: ${report.logPath})` : ""}`);
		this.output("info_tests", data, lines.join("\n"));
	}

	static toImportPath(scriptPath) {
		return (process.platform == "win32") ? scriptPath.replaceAll("\\", "/") : scriptPath;
	}
	static answerImport(machine, importPath) {
		if (process.platform == "win32")
			importPath = importPath.replaceAll("/", "\\");
		let source;
		try {
			source = fs.readFileSync(importPath, "utf8");
		}
		catch {
			machine.doGo();
			return;
		}
		machine.doModule(importPath, false, source);
	}
}

exports.TestRunner = TestRunner;
exports.expandHome = expandHome;
exports.parseFrontMatter = parseFrontMatter;
exports.getMetadata = getMetadata;
exports.iterateTests = iterateTests;
