/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

export const mxFramesView = 0;
export const mxLocalsView = 1;
export const mxGlobalsView = 2;
export const mxFilesView = 3;
export const mxBreakpointsView = 4;
export const mxModulesView = 5;
export const mxInstrumentsView = 6;

const mxAbortCommand = 0;
const mxClearAllBreakpointsCommand = 1;
const mxClearBreakpointCommand = 2;
const mxGoCommand = 3;
const mxLogoutCommand = 4;
const mxSelectCommand = 5;
const mxSetAllBreakpointsCommand = 6;
const mxSetBreakpointCommand = 7;
const mxStepCommand = 8;
const mxStepInCommand = 9;
const mxStepOutCommand = 10;
const mxToggleCommand = 11;
const mxScriptCommand = 12;

export class DebugBehavior @ "PiuDebugBehaviorDelete" {
	constructor(application) @ "PiuDebugBehaviorCreate"
	onCreate(application) {
		this.automaticInstruments = true;
		this.breakpoints = {
			expanded: true,
			items: [],
		};
		this.breakOnStart = 0;
		this.breakOnExceptions = 1;
		this.bubbles = {
			expanded: true,
			items: [],
		};
		this.consoleLines = [];
		this.consoleScroll = { x:0, y:0 };
		this.consoleText = "";
		this.conversations = {
			expanded: false,
			items: [],
		};
		this.currentMachine = null;
		this.currentTab = 0;
		this.machines = [];
		this.mappings = [];
		if (system.platform == "win") {
			this.separator = "\\";
			this.separatorRegexp = /\\/g;
			this.alienSeparator = "/";
			this.alienSeparatorRegexp = /\//g;
		}
		else {
			this.separator = "/";
			this.separatorRegexp = /\//g;
			this.alienSeparator = "\\";
			this.alienSeparatorRegexp = /\\/g;
		}
		this.port = 5002;
		this.sortingExceptions = {
			"(return)":"0",
			"new.target":"1",
			"(function)":"2",
			"this":"3",
		};
		this.sortingRegexps = [
			/(\[)([0-9]+)(\])/,
			/(\(\.)([0-9]+)(\))/,
			/(\(\.\.)([0-9]+)(\))/,
			/(arg\()([0-9]+)(\))/,
			/(var\()([0-9]+)(\))/,
		];
		this.sortingZeros = "0000000000";
	}
	
	canAbort() {
		let machine = this.currentMachine;
		return machine ? true : false;
	}
	canBreak() {
		let machine = this.currentMachine;
		return machine && !machine.broken;
	}
	canClearAllBreakpoints() {
		let breakpoints = this.breakpoints.items;
		return breakpoints.length > 0;
	}
	canClearAllBubbles() {
		let bubbles = this.bubbles.items;
		return bubbles.length > 0;
	}
	canDisableBreakpoint(path, line) {
		let breakpoints = this.breakpoints.items;
		let index = breakpoints.findIndex(breakpoint => (breakpoint.path == path) && (breakpoint.line == line));
		return index >= 0;
	}
	canGo() {
		let machine = this.currentMachine;
		return machine && machine.broken;
	}
	canStep() {
		let machine = this.currentMachine;
		return machine && machine.broken;
	}
	canStepIn() {
		let machine = this.currentMachine;
		return machine && machine.broken;
	}
	canStepOut() {
		let machine = this.currentMachine;
		return machine && machine.broken;
	}

	doAbort() {
		let machine = this.currentMachine;
		machine.doCommand(mxAbortCommand);
	}
	doBreak() {
		let machine = this.currentMachine;
		machine.doCommand(mxStepCommand);
	}
	doClearAllBreakpoints() {
		this.breakpoints.items = [];
		this.machines.forEach(machine => machine.doCommand(mxClearAllBreakpointsCommand));
		application.distribute("onBreakpointsChanged");
	}
	doClearAllBubbles() {
		this.conversations.items = [];
		this.bubbles.items = [];
		application.distribute("onConversationsChanged", this.conversations.items);
		application.distribute("onBubblesChanged", this.bubbles.items);
	}
	doClearConsole() {
		let machine = this.currentMachine;
		if (machine) {
			machine.consoleText = "";
			machine.consoleLines = [];
			application.distribute("onMachineLogged", machine.consoleText, machine.consoleLines);
		}
		else {
			this.consoleText = "";
			this.consoleLines = [];
			application.distribute("onMachineLogged", this.consoleText, this.consoleLines);
		}
	}
	doDisableBreakpoint(path, line) {
		this.doToggleBreakpoint(path, line, true);
	}
	doGo() {
		let machine = this.currentMachine;
		machine.broken = false;
		machine.running = false;
		machine.timeout = Date.now() + 500;
		machine.doCommand(mxGoCommand);
	}
	doSelectItem(application, value) {
		this.currentMachine.doCommand(mxSelectCommand, value);
	}
	doStep() {
		let machine = this.currentMachine;
		machine.broken = false;
		machine.running = false;
		machine.timeout = Date.now() + 500;
		machine.doCommand(mxStepCommand);
	}
	doStepIn() {
		let machine = this.currentMachine;
		machine.broken = false;
		machine.running = false;
		machine.timeout = Date.now() + 500;
		machine.doCommand(mxStepInCommand);
	}
	doStepOut() {
		let machine = this.currentMachine;
		machine.broken = false;
		machine.running = false;
		machine.timeout = Date.now() + 500;
		machine.doCommand(mxStepOutCommand);
	}
	doToggleBreakpoint(path, line, toggleDisabled) {
		let breakpoints = this.breakpoints.items;
		let index = breakpoints.findIndex(breakpoint => (breakpoint.path == path) && (breakpoint.line == line));
		if (index >= 0) {
			if (toggleDisabled) {
				breakpoints[index].enabled = !breakpoints[index].enabled;
			}
			else {
				breakpoints.splice(index, 1);
			}
			this.machines.forEach(machine => machine.doCommand(mxClearBreakpointCommand, path, line));
			path = this.unmapPath(path);
			if (path)
				this.machines.forEach(machine => machine.doCommand(mxClearBreakpointCommand, path, line));
		}
		else {
			if (toggleDisabled) {
				return;
			}
			breakpoints.push({ path, name:system.getPathName(path), line, enabled: true });
			breakpoints.sort(this.sortBreakpoints);
			this.machines.forEach(machine => machine.doCommand(mxSetBreakpointCommand, path, line));
			path = this.unmapPath(path);
			if (path)
				this.machines.forEach(machine => machine.doCommand(mxSetBreakpointCommand, path, line));
		}
		application.distribute("onBreakpointsChanged");
	}
	doToggleItem(application, value) {
		this.currentMachine.doCommand(mxToggleCommand, value);
	}
	doToggleConversation(application, conversation) {
		conversation.visible = !conversation.visible;
		application.distribute("onConversationsChanged", this.conversations.items);
		application.distribute("onBubblesChanged", this.bubbles.items);
	}
	formatBinaryMessage(message) @ "PiuDebugBehavior_formatBinaryMessage"
	mapPath(path, flag) {
		for (let mapping of this.mappings) {
			if (path.startsWith(mapping.remote)) {
				path = path.slice(mapping.remote.length);
				if (mapping.alien)
					path = path.replace(this.alienSeparatorRegexp, this.separator);
				path = mapping.locale.concat(path);
				return path;
			}
		}
		if (flag)
			return path;
	}
	onAccepted(socket) {
		let machine = new DebugMachine(socket);
		machine.onCreate(application, this);
		this.machines.push(machine);
		application.distribute("onMachinesChanged", this.machines);
	}
	onBroken(machine) {
		if (!this.currentMachine || !this.currentMachine.broken)
			this.selectMachine(machine);
		if (this.currentMachine == machine)
			application.distribute("onMachineBroken", machine);
		application.gotoFront();
	}
	onBubbled(machine, path, line, id, flags, message) {
		let conversations = this.conversations.items;
		let conversation = conversations.find(item => item.id == id);
		if (!conversation) {
			conversation = { visible:true, id, tint:conversations.length % 7 };
			conversations.push(conversation);
			application.distribute("onConversationsChanged", conversations);
		}
		let bubbles = this.bubbles.items;
		if (flags & 4) {
			message = this.formatBinaryMessage(message);
		}
		else {
			try {
				let json = JSON.parse(message);
				if (json)
					message = JSON.stringify(json, null, 2);
				else
					flags |= 4;
			}
			catch {
				flags |= 4;
			}
		}
		bubbles.push({ path, line, conversation, flags, message });
		application.distribute("onBubblesChanged", bubbles);
	}
	onDisconnected(machine) {
		let machines = this.machines;
		let index = machines.indexOf(machine);
		if (index >= 0) {
			machines.splice(index, 1);
			application.distribute("onMachinesChanged", machines);
		}
		if (this.currentMachine == machine) {
			this.selectMachine(null);
		}
	}
	onFileChanged(machine, path, line) {
		if (this.currentMachine == machine)
			this.selectFile(path, { line });
	}
	onFrameChanged(machine, value) {
		if (this.currentMachine == machine)
			application.distribute("onMachineFrameChanged", value);
	}
	onLogged(machine) {
		var console = this.CONSOLE;
		if (this.currentMachine == machine)
			application.distribute("onMachineLogged", machine.consoleText, machine.consoleLines);
		else if (this.currentMachine == null)
			application.distribute("onMachineLogged", this.consoleText, this.consoleLines);
	}
	onSampled(machine) {
		if (this.currentMachine == machine)
			application.distribute("onMachineSampled", machine.tracks);
	}
	onPortChanged(application) {
		this.stop();
		this.start();
	}
	onTimeChanged() {
		let now = Date.now();
		let machines = this.machines;
		let c = machines.length;
		for (let i = 0; i < c; i++) {
			let machine = machines[i];
			if ((!machine.broken) && (!machine.parsing) && (!machine.running) && (machine.timeout <= now)) {
				this.runMachine(machine);
			}
		}
	}
	onTitleChanged(machine, title, tag) {
		application.distribute("onMachinesChanged", this.machines);
		this.runMachine(machine);
	}
	onViewChanged(machine, viewIndex, lines) {
		if (this.currentMachine == machine)
			application.distribute("onMachineViewChanged", viewIndex);
	}
	
	runMachine(machine) {
		machine.framesView.empty();
		machine.localsView.empty();
		machine.modulesView.empty();
		machine.globalsView.empty();
		if (this.automaticInstruments)
			machine.instrumentsView.expanded = true;
		machine.running = true;
		application.distribute("onMachineChanged", machine);
		if (machine == this.currentMachine) {
			application.distribute("onMachineViewChanged", mxFramesView);
			application.distribute("onMachineViewChanged", mxLocalsView);
			application.distribute("onMachineViewChanged", mxModulesView);
			application.distribute("onMachineViewChanged", mxGlobalsView);
		}
	}
	start() @ "PiuDebugBehavior_start"
	stop() {
		this.machines.forEach(machine => machine.doCommand(mxAbortCommand));
		this.stopAux();
	}
	stopAux() @ "PiuDebugBehavior_stop"
	toggleBreakOnExceptions(it) {
		this.breakOnExceptions = it;
		if (it)
			this.machines.forEach(machine => machine.setBreakpoint("exceptions", 0));
		else
			this.machines.forEach(machine => machine.clearBreakpoint("exceptions", 0));
	}
	toggleBreakOnStart(it) {
		this.breakOnStart = it;
	}
	unmapPath(path, flag) {
		for (let mapping of this.mappings) {
			if (path.startsWith(mapping.locale)) {
				path = path.slice(mapping.locale.length);
				if (mapping.alien)
					path = path.replace(this.separatorRegexp, this.alienSeparator);
				path = mapping.remote.concat(path);
				return path;
			}
		}
		if (flag)
			return path;
	}
}

export class DebugMachine @ "PiuDebugMachineDelete" {
	constructor(socket) @ "PiuDebugMachineCreate"
	onCreate(application, behavior) {
		this.behavior = behavior;
		this.broken = false;
		this.once = false;
		this.parsing = false;
		this.running = true;
		this.timeout = 0;
		
		this.samplesCount = 1024;
		this.samplesIndex = -1;
		this.samplesLoop = false;

		this.views = [
			new MachineView("CALLS"),
			new MachineView("LOCALS"),
			new MachineView("GLOBALS"),
			new MachineView("FILES"),
			new MachineView("BREAKPOINTS"),
			new MachineView("MODULES"),
			new MachineView("INSTRUMENTS"),
		];
		this.ip = this.address.slice(0, this.address.lastIndexOf(":"));
		this.tag = "";
		this.title = "";
		
		this.path = "";
		this.line = 0;
		this.frame = "";
		
		this.consoleLines = [];
		this.consoleScroll = { x:0, y:0 };
		this.consoleText = "";
		
		this.debugScroll = { x:0, y:0 };
		
		this.instrumentsView.expanded = true;
	}
	get framesView() {
		return this.views[0];
	}
	get localsView() {
		return this.views[1];
	}
	get globalsView() {
		return this.views[2];
	}
	get filesView() {
		return this.views[3];
	}
	get breakpointsView() {
		return this.views[4];
	}
	get modulesView() {
		return this.views[5];
	}
	get instrumentsView() {
		return this.views[6];
	}
	close() {
		this.doCommand(mxLogoutCommand);
	}
	doAbort() {
		this.doCommand(mxAbortCommand);
	}
	doCommand(command) @ "PiuDebugMachine_doCommand"
	doScript(path) {
		this.doCommand(mxScriptCommand, path, system.readFileString(path));
	}
	onBroken(path, line, data) {
		this.broken = true;
		this.behavior.onBroken(this);
		if (path && line) {
			this.path = path;
			this.line = line;
			this.behavior.onFileChanged(this, path, line);
		}
	}
	onBubbled(path, line, id, flags, message) {
		if (id == "test262")
			this.behavior.test262Context.onMessage(this, message);
		else
			this.behavior.onBubbled(this, path, line, id, flags, message);
	}
	onDisconnected() {
		this.behavior.test262Context.onDisconnected(this);
		this.behavior.onDisconnected(this);
	}
	onFileChanged(path, line) {
		this.path = path;
		this.line = line;
		this.behavior.onFileChanged(this, path, line);
	}
	onFrameChanged(name, value) {
		this.frame = value;
		this.behavior.onFrameChanged(this, value);
	}
	onLogged(path, line, data) {
		if (path && line) {
			let color;
			if (data.indexOf("breakpoint") >= 0)
				color = 3;
			else if ((data.indexOf("debugger") >= 0) || (data.indexOf("step") >= 0))
				color = 1;
			else
				color = 2;
			this.consoleLines.push({ path, line, offset:this.consoleText.length, color }); 
			this.behavior.consoleLines.push({ path, line, offset:this.behavior.consoleText.length, color }); 
			data = path + " (" + line + ") " + data;
		}
		this.onLoggedAux(this, data);
		this.onLoggedAux(this.behavior, data);
		this.behavior.onLogged(this);
	}
	onLoggedAux(target, data) {
		let text = target.consoleText + data;
		let length = text.length - 0x7FFF;
		if (length > 0) {
			let offset = text.indexOf("\n") + 1;
			while (offset && (offset < length))
				offset = text.indexOf("\n", offset) + 1;
			if (offset)
				length = offset;
			text = text.slice(length);
			let count = -1;
			target.consoleLines.forEach((consoleLine, index) => {
				let offset = consoleLine.offset - length;
				if (offset >= 0)
					consoleLine.offset = offset;
				else
					count = index;
			});
			if (count >= 0)
				target.consoleLines.splice(0, count + 1);
		}
		target.consoleText = text;
	}
	onParsed() {
		this.parsing = false;
		var behavior = this.behavior;
		if (this.once) {
			this.once = false;
			var items = behavior.breakpoints.items.concat();
			behavior.breakpoints.items.forEach(item => {
				let path = behavior.unmapPath(item.path);
				if (path) {
					let line = item.line;
					items.push({ path, line });
				}
			});
			this.doCommand(mxSetAllBreakpointsCommand, items.filter(
				item => item.enabled
			), behavior.breakOnStart, behavior.breakOnExceptions);
		}
		else if (this.broken) {
			this.framesView.expanded = true;
			this.localsView.expanded = true;
			if (this.behavior.automaticInstruments)
				this.instrumentsView.expanded = false;
			application.distribute("onMachineChanged", this);
		}
// 		else
// 			this.doCommand(mxGoCommand);
	}
	onParsing() {
		this.parsing = true;
	}
	onSampled(data) {
		var samples = data.split(",");
		var samplesIndex = this.samplesIndex + 1;
		if (samplesIndex >= this.samplesCount) {
			this.samplesLoop = true;
			samplesIndex = 0;
		}
		this.samplesIndex = samplesIndex;
		this.instrumentsView.lines.forEach((line, index) => {
			let value = parseInt(samples[index]);
			if (line.min > value)
				line.min = value;
			if (line.max < value)
				line.max = value;
			if (line.slash)
				line.value = value + line.unit + parseInt(samples[index + 1]) + line.slash.unit;
			else
				line.value = value + line.unit;
			line.values[samplesIndex] = value;
		});
		this.behavior.onSampled(this);
	}
	onTitleChanged(title, tag) {
		this.tag = tag;
		this.title = title;
		this.once = true;
		this.behavior.onTitleChanged(this, title, tag);
	}
	onViewChanged(viewIndex, lines) {
		if ((viewIndex == mxLocalsView) || (viewIndex == mxModulesView) || (viewIndex == mxGlobalsView))
			lines = this.sortLines(viewIndex, lines);
		else if (viewIndex == mxInstrumentsView)
			lines = this.trackLines(lines);
		this.views[viewIndex].lines = lines;
		this.behavior.onViewChanged(this, viewIndex, lines);
	}
	sortLines(viewIndex, lines) {
		let behavior = this.behavior;
		let exceptions = (viewIndex == mxLocalsView) ? behavior.sortingExceptions : null;
		let regexps = behavior.sortingRegexps;
		let zeros = behavior.sortingZeros;
		let c = regexps.length;
		let former = { column:-1, parent:null, path:null };
		let name;
		lines.forEach(line => {
			while (line.column <= former.column)
				former = former.parent;
			line.parent = former;
			name = line.name;
			for (let i = 0; i < c; i++) {
				let results = regexps[i].exec(name);
				if (results) {
					let result = results[2];
					name = results[1] + zeros.slice(0, -result.length) + result + results[3];
					break;
				}
			}
			let path = former.path;
			if (path)
				line.path = path + "." + name;
			else if (exceptions && exceptions.hasOwnProperty(name))
				line.path = exceptions[name];
			else
				line.path = name;
			former = line;
		});
		lines.sort((a, b) => {
			return a.path.compare(b.path);
		});
		return lines;
	}
	trackLines(lines) {
		this.samplesCount = 1024;
		this.samplesIndex = -1;
		this.samplesLoop = false;
		lines.forEach((line, index) => {
			line.machine = this;
			line.max = -2147483647;
			line.min = 0;
			if (line.value == " / ") {
				line.slash = lines[index + 1];
				line.slash.skip = true;
			}
			line.unit = line.value;
			line.value = "";
			line.values = new Int32Array(this.samplesCount);
		});
		return lines;
	}
}

class MachineView {
	constructor(title) {
		this.expanded = false;
		this.lines = [];
		this.path = null;
		this.title = title;
	}
	empty() {
		this.expanded = false;
		this.lines = [];
	}
};

