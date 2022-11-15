/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import Timer from "timer";
import {
	Profile,
} from "ProfilePane";

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
const mxStartProfilingCommand = 8;
const mxStepCommand = 9;
const mxStepInCommand = 10;
const mxStepOutCommand = 11;
const mxStopProfilingCommand = 12;
const mxToggleCommand = 13;
const mxImportCommand = 14;
const mxScriptCommand = 15;
const mxModuleCommand = 16;
const serialConnectStrings = ["Connect", "Disconnect", "Connecting...", "Installing..."];
const consoleColorCodes = [{ code: "<info>", color: 3 }, { code: "<warn>", color: 1 }, { code: "<error>", color: 2 }];

export class DebugBehavior @ "PiuDebugBehaviorDelete" {
	constructor(application) @ "PiuDebugBehaviorCreate"
	onCreate(application) {
		this.automaticInstruments = true;
		this.showExceptions = true;
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
		this.profileOnStart = 0;
		const platform = system.platform;
		if (platform == "lin")
			this.serialDevicePath = "/dev/ttyUSB0";
		else if (platform == "mac")
			this.serialDevicePath = "/dev/cu.SLAB_USBtoUART";
		else
			this.serialDevicePath = "com3";
		this.serialBaudRates = [ 460800, 921600, 1500000 ];
		this.serial = new DebugSerial(this);;
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
	
	canSerialConnect(application, item) {
		item.string = serialConnectStrings[this.serial.state];
		return this.serial.state < 2;
	}
	canSerialInstallApp() {
		return this.serial.state < 2;
	}
	canSerialInstallMod() {
		return this.serial.state == 1;
	}
	canSerialRestart() {
		return (this.serial.state == 1) && (this.serial.app.signature != "");
	}
	canSerialUninstallMod() {
		return (this.serial.state == 1) && (this.serial.mod.signature != "");
	}
	doSerialConnect() {
		if (this.serial.state == 0) {
			this.serial.doConnect();
		}
		else if (this.serial.state == 1) {
			this.serial.doDisconnect();
		}
	}
	doSerialInstallApp() {
		system.openFile({ prompt:"Install App" }, path => { if (path) application.defer("doSerialInstallAppCallback", new String(path)); });
	}
	doSerialInstallAppCallback(application, path) {
		this.serial.doInstallApp(path);
	}
	doSerialInstallMod() {
		system.openFile({ prompt:"Install Mod" }, path => { if (path) application.defer("doSerialInstallModCallback", new String(path)); });
	}
	doSerialInstallModCallback(application, path) {
		this.serial.doInstallMod(path);
	}
	doSerialRestart() {
		this.serial.doRestart();
	}
	doSerialUninstallMod() {
		this.serial.doUninstallMod();
	}

	onConnectError(application, e) {
		system.alert({ 
			type:"stop",
			prompt:"xsbug",
			info:`Serial connection error.\n\n${e.message}\n\nCheck the Serial Preferences...`,
			buttons:["OK", "Cancel"]
		}, ok => {
			if (ok)
				application.defer("doPreferences");
		});
	}
	onConnectTimeout(application) {
		system.alert({ 
			type:"stop",
			prompt:"xsbug",
			info:"Serial connection timeout.\n\nCheck the Serial Preferences...",
			buttons:["OK", "Cancel"]
		}, ok => {
			if (ok)
				application.defer("doPreferences");
		});
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
			this.machines.forEach(machine => machine.doBreakpointCommand(mxClearBreakpointCommand, path, line));
			path = this.unmapPath(path);
			if (path)
				this.machines.forEach(machine => machine.doBreakpointCommand(mxClearBreakpointCommand, path, line));
		}
		else {
			if (toggleDisabled) {
				return;
			}
			breakpoints.push({ path, name:system.getPathName(path), line, enabled: true });
			breakpoints.sort(this.sortBreakpoints);
			this.machines.forEach(machine => machine.doBreakpointCommand(mxSetBreakpointCommand, path, line));
			path = this.unmapPath(path);
			if (path)
				this.machines.forEach(machine => machine.doBreakpointCommand(mxSetBreakpointCommand, path, line));
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
		let length = bubbles.length;
		if (length == 999) {
			bubbles.copyWithin(0, 1);
			bubbles[998] = { path, line, conversation, flags, message };
		}
		else
			bubbles.push({ path, line, conversation, flags, message });
		application.distribute("onBubblesChanged", bubbles);
	}
	onDevicesChanged() {
		this.serial.check();
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
			this.machines.forEach(machine => machine.doBreakpointCommand(mxSetBreakpointCommand, "exceptions", 0));
		else
			this.machines.forEach(machine => machine.doBreakpointCommand(mxClearBreakpointCommand, "exceptions", 0));
	}
	toggleBreakOnStart(it) {
		this.breakOnStart = it;
	}
	toggleProfileOnStart(it) {
		this.profileOnStart = it;
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

let temporary = 0;

export class DebugMachine @ "PiuDebugMachineDelete" {
	constructor(socket) @ "PiuDebugMachineCreate"
	onCreate(application, behavior) {
		this.behavior = behavior;
		this.broken = false;
		this.once = false;
		this.parsing = false;
		this.profile = new Profile(this);
		this.profiling = false;
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
		this.tag = "";
		this.title = "";
		
		this.tagToPath = new Map;
		this.pathToTag = new Map;
		this.path = "";
		this.line = 0;
		this.frame = "";
		
		this.consoleLines = [];
		this.consoleScroll = { x:0, y:0 };
		this.consoleText = "";
		
		this.debugScroll = { x:0, y:0 };
		
		this.instrumentsView.expanded = true;
		
		this.binaryCommandID = 0;
		this.binaryCommands = [];
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
	doBinaryCommand(command, payload) {
		return new Promise((resolve, reject) => {
			let id = this.binaryCommandID + 1;
			this.binaryCommands.push({ id, resolve, reject });
			this.binaryCommandID = id;
			this.doBinaryCommandAux(command, id, payload);
		})
	}
	doBinaryCommandAux(command, id, payload) @ "PiuDebugMachine_doBinaryCommandAux"
	doBreakpointCommand(command, path, line) {
		const pathTag = this.pathToTag.get(path);
		if (pathTag !== undefined)
			path = pathTag;
		this.doCommand(command, path, line);
	}
	doCommand(command) @ "PiuDebugMachine_doCommand"
	doGo() {
		this.doCommand(mxGoCommand);
	}
	doImport(path, wait) {
		this.doCommand(mxImportCommand, path, wait);
	}
	doModule(path, wait) {
		this.doCommand(mxModuleCommand, path, wait, system.readFileBuffer(path));
	}
	doStartProfiling() {
		this.doCommand(mxStartProfilingCommand);
	}
	doStopProfiling() {
		this.doCommand(mxStopProfilingCommand);
	}
	doScript(path, wait) {
		this.doCommand(mxScriptCommand, path, wait, system.readFileBuffer(path));
	}
	onBinaryResult(data) {
		const view = new DataView(data);
		const command = view.getUint8(0);
		const id = view.getUint16(1);
		let item = this.binaryCommands.shift();
		while (item.id != id) {
			item.reject(-1);
			item = this.binaryCommands.shift();
		}
		if (command != 5) {
			item.reject(-1);
			return;
		}
		const code = view.getInt16(3);
		if (code != 0) {
			item.reject(code);
			return;
		}
		item.resolve(data.slice(5));
	}
	onBroken(path, line, data) {
		this.broken = true;
		this.behavior.onBroken(this);
		if (path && line) {
			this.onFileChanged(path, line);
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
	onEval(tag, string) {
		try {
			const path = system.buildPath(model.evalDirectory, temporary, "js");
			temporary++;
			system.writeFileString(path, string);
			this.tagToPath.set(tag, path);
			this.pathToTag.set(path, tag);
		}
		catch {
		}
	}
	onFileChanged(path, line) {
		const tagPath = this.tagToPath.get(path);
		if (tagPath !== undefined)
			path = tagPath;
		this.path = path;
		this.line = line;
		this.behavior.onFileChanged(this, path, line);
	}
	onFrameChanged(name, value) {
		this.frame = value;
		this.behavior.onFrameChanged(this, value);
	}
	onImport(path) {
		this.behavior.test262Context.onImport(this, path);
	}
	onLogged(path, line, data) {
		let showLog = true;
		if (path && line) {
			const tagPath = this.tagToPath.get(path);
			if (tagPath !== undefined)
				path = tagPath;
			let color;
			if (data.indexOf("breakpoint") >= 0)
				color = 3;
			else if ((data.indexOf("debugger") >= 0) || (data.indexOf("step") >= 0))
				color = 1;
			else {
				color = 2;
				showLog = this.behavior.showExceptions;
			}
			if (showLog) {
				this.consoleLines.push({ path, line, offset:this.consoleText.length, color }); 
				this.behavior.consoleLines.push({ path, line, offset:this.behavior.consoleText.length, color }); 
				data = path + " (" + line + ") " + data;
			} 
			if (showLog) {
				this.onLoggedAux(this, data);
				this.onLoggedAux(this.behavior, data);
				this.behavior.onLogged(this);
			}
		} else {
			let lines = data.split(/(?<=[\n])/);
			lines.forEach((line) => {
				if (this.consoleText.endsWith('\n')) {
					let colorCode = consoleColorCodes.find((colorCode) => line.startsWith(colorCode.code));
					if (colorCode) {
						line = line.slice(colorCode.code.length);
						this.consoleLines.push({ path: undefined, line: undefined, offset:this.consoleText.length, color: colorCode.color }); 
						this.behavior.consoleLines.push({ path: undefined, line: undefined, offset:this.behavior.consoleText.length, color: colorCode.color }); 
					}
				}
				this.onLoggedAux(this, line);
				this.onLoggedAux(this.behavior, line);
			});
			this.behavior.onLogged(this);
		}
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
					items.push({ path, line, enabled:item.enabled });
				}
			});
			this.doCommand(mxSetAllBreakpointsCommand, items.filter(
				item => item.enabled
			), behavior.breakOnStart, behavior.breakOnExceptions);
			if (behavior.profileOnStart)
				this.doCommand(mxStartProfilingCommand);
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
	onProfileRecord(name, value, path, line) {
// 		trace(`onProfileRecord ${name} ${value} ${path} ${line}\n`);
		this.profile.setRecord(parseInt(value), name, path, line);
	}
	onProfileSample(data) {
// 		trace(`onProfileSample ${data}\n`);
		const samples = data.split(".");
		const profile = this.profile;
		for (let i = 0, length = samples.length; i < length; i++) {
			const values = samples[i].split(",").map(item => parseInt(item, 36));
			const delta = values[0];
			let callee = profile.getRecord(values[1]);
			callee.hit(delta);
			let index = 2;
			let count = values.length;
			while (index < count) {
				let caller = profile.getRecord(values[index]);
				callee.insertCaller(caller);
				caller.insertCallee(callee);
				callee = caller;
				index++;
			}
		}
		profile.clear();
		profile.propagate();
	}
	onProfileTime(name, value) {
// 		trace(`onProfileTime ${name} ${value}\n`);
		if (name == "start") {
			this.profile.empty();
			this.profiling = true;
			application.distribute("onMachineChanged", this);
		}
		else if (name == "stop") {
			this.profiling = false;
			application.distribute("onMachineChanged", this);
		}
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
			return a.path.localeCompare(b.path);
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

import Serial from "io/serial";
import EspTool from "esptool";

globalThis.setTimeout = function (callback, delay) {
	Timer.set(function() {
		callback();
	}, delay);
}

globalThis.console = Object.freeze({
	log(msg) {
		trace(msg, "\n");
	}
}, true);

globalThis.TextDecoder = class {
	decode(data) {
		return String.fromArrayBuffer(data.subarray().buffer);
	}
}

const pocoPixelsFormats = [
	"",
	"",
	"",
	"Monochrome", // kCommodettoBitmapMonochrome (3)
	"4-bit Gray", //kCommodettoBitmapGray16 (4)
	"8-bit Gray", //kCommodettoBitmapGray256 (5)
	"8-bit RGB 332", //kCommodettoBitmapRGB332 (6)
	"16-bit RGB 565 Little Endian", //kCommodettoBitmapRGB565LE (7)
	"16-bit RGB 565 Big Endian", //kCommodettoBitmapRGB565BE (8)
	"24-bit RGB", //kCommodettoBitmap24RGB (9)
	"32-bit RGBA", //kCommodettoBitmap32RGBA (10)
	"4-bit Color Look-up Table", //kCommodettoBitmapCLUT16 (11)
	"12-bit RGB 444", //kCommodettoBitmapRGB444 (12)
];

function timeout(ms) {
    return new Promise(resolve => Timer.set(resolve, ms));
}
function twoDigits(number) {
	return (number < 16) ? "0" + number : number;
}

class DebugSerial @ "PiuDebugSerialDelete" {
	constructor(behavior) {
		this.behavior = behavior;
		this.device = {
			expanded: true,
			mcu: "",
			macAddress: "",
			pixelFormat: "",
		};
		this.app = {
			expanded: true,
			progress: -1,
			signature: "",
			xsVersion: "",
		};
		this.mod = {
			expanded: true,
			progress: -1,
			signature: "",
			spaceAvailable: -1,
		};
		this.machine = null;
		this.machines = [];
		this.serial = null;
		this.state = 0;
		this.create();
	}
	check() {
		this.serial?.check();
	}
	close() {
		this.machines.forEach(item => item.onDisconnected());
		this.machines = [];
		this.machine = null;
	}
	closeMachine(address) {
		let index = this.machines.findIndex(item => item.address == address);
		if (index < 0)
			return;
		let machine = this.machines[index];
		if (this.machine == machine)
			this.machine = null;
		machine.onDisconnected();	
		this.machines.splice(index, 1);
	}
	async connect() {
		try {
			const baudRates = this.behavior.serialBaudRates
			const devicePath = this.behavior.serialDevicePath
			let baudRatesIndex = 0
			while (baudRatesIndex < baudRates.length) {
				this.serial = new Serial({
					device: devicePath,
					baud: baudRates[baudRatesIndex],
					target:this,
					onReadable(count) {
						const buffer = this.read();
						if (buffer)
							this.target.parse(buffer);
					},
					onError() {
						this.target.doDisconnect();
					}
				});
				await this.doRestart();
				if (this.machine) {
					this.state = 1;
					application.distribute("onSerialChanged");
					return;
				}
				await timeout(50);
				baudRatesIndex++;
			}
			application.defer("onConnectTimeout");
		}
		catch(e) {
			application.defer("onConnectError", e);
		}
		this.state = 0;
		application.distribute("onSerialChanged");
	}
	create() @ "PiuDebugSerialCreate"
	async disconnect() {
		this.close();
		if (this.serial) {
			this.serial.close();
			this.serial = null;
		}
	}
	async doConnect(path) {
		this.state = 2;
		application.distribute("onSerialChanged");
		await this.connect();
	}
	async doDisconnect(path) {
		await this.disconnect();
		this.device.mcu = "";
		this.device.macAddress = "";
		this.device.pixelFormat = "";
		this.app.progress = -1;
		this.app.signature = "";
		this.app.xsVersion = "";
		this.mod.progress = -1;
		this.mod.signature = "";
		this.mod.spaceAvailable = -1;
		this.state = 0;
		application.distribute("onSerialChanged");
	}
	async doInstallApp(path) {
		const name = system.getPathName(path);
		const mcu = (name == "xs_esp32.bin") ? "esp32" : (name == "main.bin") ? "esp8266" : "";
		if (!mcu) {
			system.alert({ 
				type:"stop",
				prompt:"xsbug",
				info:`The file must be \"xs_esp32.bin\" for ESP32 or \"main.bin\" for the ESP8266.`,
				buttons:["Cancel"]
			}, ok => {
			});
			return;
		}
		try {
			if (this.state != 0)
				await this.disconnect();
			
			this.app.progress = 0;
			this.app.signature = "";
			this.app.xsVersion = "";
			this.mod.progress = -1;
			this.mod.signature = "";
			this.mod.spaceAvailable = -1;
			this.state = 3;
			application.distribute("onSerialChanged");
			
			const flash = {
				mode: ("esp32" === mcu) ? "dio" : "qio",	// qio is enabled in bootloader build for ESP32. must be dio here.
				frequency: "80m",
				size: "4MB",
			};
			const tool = new EspTool({
				device: this.behavior.serialDevicePath,
				mcu,
				flash,
			});
			try {
				await tool.beginProgramming();

				const info = await tool.getInfo();
				console.log(JSON.stringify(info, undefined, 3));

				const data = system.readFileBuffer(path);
				await tool.write({
					data,
					offset: "app",
				}, progress => {
					this.app.progress = progress;
					application.distribute("onSerialProgressChanged");
				});
			}
			catch(e) {
				throw e;
			}
			finally {
				tool.close();
			}
			this.app.progress = 1;
			application.distribute("onSerialProgressChanged");

			await this.connect();
		}
		catch(e) {
			this.doDisconnect();
			application.defer("onConnectError", e);
		}
	}
	async doInstallMod(path) {
		if (this.mod.spaceAvailable < 0) {
			system.alert({ 
				type:"stop",
				prompt:"xsbug",
				info:`The app does not support mods.`,
				buttons:["Cancel"]
			}, ok => {
			});
			return;
		}
		const data = system.readFileBuffer(path);
		const size = data.byteLength;
		if (this.mod.spaceAvailable < size) {
			system.alert({ 
				type:"stop",
				prompt:"xsbug",
				info:`Not enough memory. The mod requires ${size} bytes but only ${this.mod.spaceAvailable} bytes are available.`,
				buttons:["Cancel"]
			}, ok => {
			});
			return;
		}
		if (this.state == 0)
			await this.connect();
		if (this.state != 1)
			return;
		this.mod.progress = 0;
		this.mod.signature = "";
		this.state = 3;
		application.distribute("onSerialChanged");
		try {
			let offset = 0;
			let step = 16;
			while (offset < size) {
				const use = Math.min(step, size - offset);
				const payload = new Uint8Array(4 + use);
				payload[0] = (offset >> 24) & 0xff;
				payload[1] = (offset >> 16) & 0xff;
				payload[2] = (offset >>  8) & 0xff;
				payload[3] =  offset        & 0xff;
				payload.set(new Uint8Array(data, offset, use), 4);
				await this.machine.doBinaryCommand(3, payload.buffer);	
				this.mod.progress = offset / size;
				application.distribute("onSerialProgressChanged");
				offset += use;
				step = 512;
			}
			this.mod.progress = 1;
			application.distribute("onSerialProgressChanged");
			await this.doRestart();
		}
		catch(e) {
			this.doDisconnect();
			application.defer("onConnectError", e);
		}
	}
	async doRestart() {
		this.close();
		this.serial.set({DTR: false, RTS: true});
		await timeout(50);
		this.serial.set({DTR: false, RTS: false});
		const promise = new Promise((resolve, reject) => {
			this.resolveMachine = resolve;
		});
		await Promise.race([promise, timeout(1000)]);
		if (this.machine)
			return;
		this.disconnect();
	}
	async doSetTime() {
		const payload = new ArrayBuffer(12);
		const view = new DataView(payload);
		const date = new Date();
		view.setUint32(0, Math.round(date.valueOf() / 1000)); // big-endian
		view.setUint32(4, date.getTimezoneOffset() * -60);
		view.setUint32(8, 0);
		await this.machine.doBinaryCommand(9, payload);
		this.machine.doCommand(mxGoCommand);
	}
	async doUninstallMod() {
		try {
			await this.machine.doBinaryCommand(2);
			await this.doRestart();
		}
		catch(e) {
			this.doDisconnect();
			application.defer("onConnectError", e);
		}
	}
	async getInfos() {
		try {
			const machine = this.machine;
			let data, view;
			
			try {
				data = await machine.doBinaryCommand(17);
				this.device.mcu = String.fromArrayBuffer(data);
			}
			catch {
				this.device.mcu = "";
			}

			data = await machine.doBinaryCommand(14);
			view = new Uint8Array(data);
			this.device.macAddress = "MAC " + view.reduce((former, value) => { return former + (former ? ":" : "") + value.toString(16).padStart(2, "0") }, "");
			data = await machine.doBinaryCommand(12);
			view = new DataView(data);
			this.device.pixelFormat = `${pocoPixelsFormats[view.getUint8(0)]}`;

			this.app.progress = -1;
			data = await machine.doBinaryCommand(13);
			this.app.signature = String.fromArrayBuffer(data);
			data = await machine.doBinaryCommand(11);
			view = new DataView(data);
			this.app.xsVersion = `XS ${view.getUint8(0)}.${view.getUint8(1)}.${view.getUint8(2)}`;
			
			this.mod.progress = -1;
			try {
				data = await machine.doBinaryCommand(16, ArrayBuffer.fromString("NAME"));
				this.mod.signature = String.fromArrayBuffer(data);
				data = await machine.doBinaryCommand(15);
				view = new DataView(data);
				this.mod.spaceAvailable = view.getUint32(0);
			}
			catch {
				this.mod.signature = "";
				this.mod.spaceAvailable = -1;
			}
			
			await this.doSetTime();
			
			this.state = 1;
			application.distribute("onSerialChanged");
			
			if (this.resolveMachine) {
				this.resolveMachine(machine);
				this.resolveMachine = null;
			}
		}
		catch(e) {
			application.defer("onConnectError", e);
		}
	}
	onBinaryResult(buffer) {
		this.machine?.onBinaryResult(buffer);
	}
	onBroken(path, line, data) {
		this.machine?.onBroken(path, line, data);
	}
	onBubbled(path, line, name, value, data) {
		this.machine?.onBubbled(path, line, name, value, data);
	}
	onEval(tag, string) {
		this.machine?.onEval(tag, string);
	}
	onFileChanged(path, line) {
		this.machine?.onFileChanged(path, line);
	}
	onFrameChanged(name, value) {
		this.machine?.onFrameChanged(name, value);
	}
	onImport(path) {
		this.machine?.onImport(path);
	}
	onLogged(path, line, data) {
		this.machine?.onLogged(path, line, data);
	}
	onParsed() {
		const once = this.machine.once;
		this.machine?.onParsed();
		if (once) {
			this.getInfos();
		}
	}
	onParsing() {
		this.machine?.onParsing();
	}
	onProfileRecord(name, value, path, line) {
		this.machine?.onProfileRecord(name, value, path, line);
	}
	onProfileSample(data) {
		this.machine?.onProfileSample(data);
	}
	onProfileTime(name, value) {
		this.machine?.onProfileTime(name, value);
	}
	onSampled(samples) {
		this.machine?.onSampled(samples);
	}
	onTitleChanged(name, value) {
		this.machine?.onTitleChanged(name, value);
	}
	onViewChanged(index, list) {
		this.machine?.onViewChanged(index, list);
	}
	async getMachine() {
		return new Promise((resolve, reject) => {
			this.machineCallbacks = {resolve, reject};
		})
	}
	openMachine(address) {
		let machine = this.machines.find(item => item.address == address);
		if (!machine) {
			this.machine = machine = new DebugMachine(this, address);
			machine.onCreate(application, this.behavior);
			this.machines.push(machine);
			this.behavior.machines.push(machine);
			application.distribute("onMachinesChanged", this.behavior.machines);
		}
		else
			this.machine = machine;
	}
	parse(buffer) @ "PiuDebugSerialParse"
	write(buffer) {
		this.serial.write(buffer);
	}
}
