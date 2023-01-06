/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

const { Profile } = require('./xsbug-profile.js');
const Saxophone = require('saxophone');

class Machine {
	constructor(input, output) {
		const parser = new Saxophone()
		parser.on('tagopen', ({ name, attrs, isSelfClosing }) => {
			this.onStartElement(name, Saxophone.parseAttrs(attrs));
			if (isSelfClosing)
				this.onEndElement(name);
		});
		parser.on('tagclose', ({ name }) => {
			this.onEndElement(name);
		});
		parser.on('text', ({ contents }) => {
			this.onCharacterData(Saxophone.parseEntities(contents));
		});
		parser.on('cdata', ({ contents }) => {
			this.onCharacterData(contents);
		});
		parser.on('error', err => {
			this.onError(err);
		});
		input.pipe(parser);
		this.encoder = new TextEncoder();
		this.instruments = [];
		this.output = output;
		this.profile = new Profile();
		this.profiling = false;
	}
	
	onStartElement(name, attributes) {
// 		console.log(`onStartElement ${name} ${JSON.stringify(attributes)}`);
		const parent = this.current;
		const current = attributes;
		switch (name) {
		case 'xsbug':
			break;
		case 'breakpoint':
		case 'instrument':
		case 'file':
		case 'frame':
			parent.children.push(current);
			current.parent = parent;
			break;
		case 'node':
		case 'property':
			parent.children.push(current);
			current.parent = parent;
			current.children = [];
			break;
			
		case 'break':
		case 'bubble':
		case 'eval':
		case 'log':
		case 'ps':
		case 'samples':
			current.data = "";
			break;

		case 'breakpoints':
		case 'instruments':
		case 'files':
		case 'frames':
		case 'global':
		case 'grammar':
		case 'local':
			current.children = [];
			break;
			
		}
		this.current = current;
	}
	onEndElement(name) {
// 		console.log(`onEndElement ${name}`);
		const current = this.current;
		this.current = null;
		switch (name) {
		case 'xsbug':
			break;
		case 'breakpoint':
		case 'instrument':
		case 'file':
		case 'frame':
			this.current = current.parent;
			delete current.parent;
			break;
		case 'node':
		case 'property':
			this.current = current.parent;
			delete current.parent;
			if (current.children.length == 0)
				delete current.children;
			break;
			
		case 'break':
			this.onLogged(current.path, current.line, current.data);
			this.onBroken(current.path, current.line, current.data);
			break;
		case 'breakpoints':
			this.onViewChanged(name, current.children);
			break;
		case 'bubble':
			this.onBubbled(current.name, current.value, current.path, current.line, current.data);
			break;
		case 'eval':
			this.onEval(current.path, current.data);
			break;
		case 'files':
			this.onViewChanged(name, current.children);
			break;
		case 'frames':
			this.onViewChanged(name, current.children);
			break;
		case 'global':
			this.onViewChanged(name, current.children);
			break;
		case 'grammar':
			this.onViewChanged("modules", current.children); // historical
			break;
		case 'import':
			this.onImport(current.path);
			break;
		case 'instruments':
			this.onInstrumentDescriptions(current.children);
			break;
		case 'local':
			if (current.path && current.line)
				this.onFileChanged(current.path, current.line);
			this.onFrameChanged(current.name, current.value);
			this.onViewChanged(name, current.children);
			break;
		case 'log':
			this.onLogged(current.path, current.line, current.data);
			break;
		case 'login':
			this.onTitleChanged(current.name, current.value);
			break;
		case 'pr':
			this.onProfileRecord(current.name, current.value, current.path, current.line);
			break;
		case 'ps':
			this.onProfileSamples(current.data);
			break;
		case 'pt':
			this.onProfileTime(current.name, current.value);
			break;
		case 'samples':
			this.onInstrumentValues(current.data.split(","));
			break;
		}
	}
	onCharacterData(data) {
// 		console.log(`onCharacterData ${data}`);
		if (this.current)
			this.current.data += data;
	}
	onError(error) {
	}
	
	// COMMANDS
	doCommand(name) {
		this.output.write(this.encoder.encode(`\r\n<${name}/>\r\n`));
	}
	doAbort() {
		this.doCommand(`abort`);
	}
	doClearAllBreakpoints() {
		this.doCommand(`clear-all-breakpoints`);
	}
	doClearBreakpoint(path, line) {
		this.doCommand(`clear-breakpoint path="${path}" line="${line}"`);
	}
	doGo() {
		this.doCommand(`go`);
	}
	doImport(path, wait) {
		this.doCommand(`import path="${path}" line="${wait ? 1 : 0}"`);
	}
	doLogout() {
		this.doCommand(`logout`);
	}
	doModule(path, wait, source) {
		let string = `\r\n<module path="${path}" line="${wait ? 1 : 0}"><![CDATA[`;
		string += source;
		string += `]]><module/>\r\n`;
		this.output.write(this.encoder.encode(string));
	}
	doScript(path, wait, source) {
		let string = `\r\n<script path="${path}" line="${wait ? 1 : 0}"><![CDATA[`;
		string += source;
		string += `]]><script/>\r\n`;
		this.output.write(this.encoder.encode(string));
	}
	doSetAllBreakpoint(breakpoints, atStart, atExceptions) {
		let string = `\r\n<set-all-breakpoints>`;
		if (atStart)
			string += `<breakpoint path="start" line="0"/>`;
		if (atExceptions)
			string += `<breakpoint path="exceptions" line="0"/>`;
		for (let breakpoint of breakpoints) {
			string += `<breakpoint path="${breakpoint.path}" line="${breakpoint.line}"/>`;
		}
		string += `<set-all-breakpoints/>\r\n`;
		this.output.write(this.encoder.encode(string));
	}
	doSetBreakpoint(path, line) {
		this.doCommand(`set-breakpoint path="${path}" line="${line}"`);
	}
	doSelect(value) {
		this.doCommand(`select id="${value}"`);
	}
	doStep() {
		this.doCommand(`step`);
	}
	doStepIn() {
		this.doCommand(`step-inside`);
	}
	doStepOut() {
		this.doCommand(`step-outside`);
	}
	doStartProfiling() {
		this.doCommand(`start-profiling`);
	}
	doStopProfiling() {
		this.doCommand(`stop-profiling`);
	}
	doToggle(value) {
		this.doCommand(`toggle id="${value}"`);
	}
	
	// INTERNAL EVENTS
	
	onBroken(path, line, text) {
		this.doGo();
	}
	onBubbled(path, line, id, flags, message) {
	}
	onEval(path, source) {
	}
	onFileChanged(path, line) {
	}
	onFrameChanged(name, value) {
	}
	onImport(path) {
	}
	onInstrumentDescriptions(descriptions) {
		this.instruments = descriptions.map(description => ({ name: description.name, unit: description.value }));
	}
	onInstrumentValues(values) {
		this.instruments.forEach((instrument, index) => { instrument.value = values[index]; });
    	this.#eventListeners.instruments.forEach(listener => listener.call(null, this.instruments));
	}
	onLogged(path, line, text) {
	}
	onProfileRecord(name, value, path, line) {
		this.profile.setRecord(parseInt(value), name, path, line);
	}
	onProfileSamples(data) {
		const samples = data.split(".");
		const profile = this.profile;
		for (let i = 0, length = samples.length; i < length; i++) {
			const values = samples[i].split(",").map(item => parseInt(item, 36));
			profile.hit(values);
		}
	}
	onProfileTime(name, value) {
		if (name == "start") {
			this.profile.start(parseInt(value.slice(1), 16));
			this.profiling = true;
		}
		else if (name == "stop") {
			this.profile.stop(parseInt(value.slice(1), 16));
			this.profiling = false;
    		this.#eventListeners.profile.forEach(listener => listener.call(null, this.profile));
		}
	}
	onTitleChanged(title, tag) {
		this.tag = tag;
		this.title = title;
		this.doGo();
	}
	onViewChanged(view, items) {
	}
	
	// EXTERNAL EVENTS
	
	#eventListeners = {
		instruments:[],
		profile:[],
	};
	addEventListener(event, listener) {
		let listeners = this.#eventListeners[event];
		if (!listeners)
			throw new Error("no such event");
		listeners.push(listener);
	}
	on(event, listener) {
		this.addEventListener(event, listener);
	}
	removeEventListener(event, listener) {
		let listeners = this.#eventListeners[event];
		if (!listeners)
			throw new Error("no such event");
		let index = listeners.find(item => item === listener);
		if (index >= 0)
			listeners.splice(index, 1);
	}
	reportInstruments(instruments) {
		let length = this.instruments.length;
		let index = 0;
		while (index < length) {
			let instrument = this.instruments[index];
			let result = instrument.name;
			result += ": ";
			result += instrument.value;
			result += instrument.unit;
			if (instrument.unit == " / ") {
				index++;
				instrument = this.instruments[index];
				result += instrument.value;
				result += instrument.unit;
			}
			console.log(result);
			index++;
		}
	}
}

exports.Machine = Machine;
