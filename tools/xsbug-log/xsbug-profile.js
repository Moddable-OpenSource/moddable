/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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

class ProfileRecord {
	constructor(id, name, path, line) {
		this.id = id;
		this.name = name || "(anonymous)";
		this.path = path;
		this.line = line;
		this.hitCount = 0;
		this.duration = 0;
		this.propagated = false;
		this.propagation = 0;
		this.callees = [];
		this.callers = [];
	}
	hit(delta) {
		this.hitCount++;
		this.duration += delta;
	}
	insertCallee(callee) {
		const callees = this.callees;
		const index = callees.indexOf(callee);
		if (index < 0)
			callees.push(callee);
	}
	insertCaller(caller) {
		const callers = this.callers;
		const index = callers.indexOf(caller);
		if (index < 0)
			this.callers.push(caller);
	}
	propagate(delta, callee) {
		if (this.propagated) {
			this.removeCallee(callee);
			callee.removeCaller(this);
			return;
		}
		this.propagation += delta;
		const callersCount = this.callers.length;
		if (callersCount) {
			this.propagated = true;
			delta /= callersCount;
			if (delta >= 1)
				this.callers.forEach(record => {
					return record.propagate(delta, this)
				});
			this.propagated = false;
		}
	}
	removeCallee(callee) {
		const callees = this.callees;
		const index = callees.indexOf(callee);
		if (index >= 0)
			callees.splice(index, 1);
	}
	removeCaller(caller) {
		const callers = this.callers;
		const index = callers.indexOf(caller);
		if (index >= 0)
			callers.splice(index, 1);
	}
}

class Profile {
	constructor() {
	}
	empty() {
		this.records = new Map;
		this.startTime = 0;
		this.endTime = 0;
		this.samples = [];
		this.timeDeltas = [];
	}
	getRecord(id) {
		return this.records.get(id);
	}
	hit(values) {
		const delta = values[0];
		const sample = values[1];
		this.samples.push(sample);
		this.timeDeltas.push(delta);
		let callee = this.getRecord(sample);
		let index = 2;
		let count = values.length;
		while (index < count) {
			let caller = this.getRecord(values[index]);
			callee.insertCaller(caller);
			caller.insertCallee(callee);
			callee = caller;
			index++;
		}
	}
	propagate() {
		const { records, startTime, endTime, samples, timeDeltas } = this;
		const length = Math.min(samples.length, timeDeltas.length);
		let time = startTime + timeDeltas[0];
		for (let index = 1; index < length; index++) {
			const delta = timeDeltas[index]
			const sample = samples[index - 1];
			const record = records.get(sample);
			record.hit(delta);
			time += delta;
		}
		const sample = samples[length - 1];
		const record = records.get(sample);
		record.hit(endTime - time);
		let hits = [];
		this.records.forEach(record => {
			if (record.hitCount) {
				hits.push(record);
				record.propagate(record.duration, null);
			}
		});
		return hits;
	}
	reportHits() {
		const records = this.propagate();
		records.sort((a, b) => {
			let result = b.duration - a.duration;
			if (!result) 
				result = a.name.localeCompare(b.name);
			if (!result) 
				result = a.id - b.id;
			return result;
		});
		for (let record of records) {
			let { name, duration, propagation, path, line } = record;
			duration = Math.round((duration / 1000)).toString();
			duration = " ".repeat(6 - duration.length) + duration;
			propagation = Math.round((propagation / 1000)).toString();
			propagation = " ".repeat(6 - propagation.length) + propagation;
			let string = `${duration} ms ${propagation} ms ${name}`;
			if (path)
				string += ` (${path}:${line})`;
			console.log(string);
		}
	}
	setRecord(id, name, path, line) {
		this.records.set(id, new ProfileRecord(id, name, path, line));
	}
	start(when) {
		this.empty();
		this.startTime = when;
	}
	stop(when) {
		this.endTime = when;
	}
	toString() {
		const json = {
			nodes:[],
			startTime: this.startTime,
			endTime: this.endTime,
			samples: this.samples,
			timeDeltas: this.timeDeltas,
		}
		this.records.forEach(record => {
			const node = {
				id: record.id,
				callFrame: {
					functionName: record.name,
					scriptId: 0,
					url: record.path,
					lineNumber: record.line - 1,
					columnNumber: -1,
				},
				hitCount: record.hitCount,
				children: record.callees.map(callee => callee.id),
			}
			json.nodes.push(node);
		});	
		return JSON.stringify(json);
	}
};

exports.Profile = Profile;
