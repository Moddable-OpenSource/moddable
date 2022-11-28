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

// MODEL

class ProfileRecord @ "PiuProfileDelete" {
	constructor(id, name, path, line) {
		this.id = id;
		this.name = name || "(anonymous)";
		this.path = path;
		this.line = line;
		if (path)
			this.where = system.getPathName(path) + " (" + line +")";
		else
			this.where = "";
		this.hitCount = 0;
		this.duration = 0;
		this.propagated = false;
		this.propagation = 0;
		this.callees = [];
		this.callees.expanded = id == 0;
		this.callees.sorted = false;
		this.callers = [];
		this.callers.expanded = false;
		this.callers.sorted = false;
	}
	hit(delta) {
		this.hitCount++;
		this.duration += delta;
	}
// 	propagate(delta, callee) {
// 		if (this.propagated) {
// 			this.removeCallee(callee);
// 			callee.removeCaller(this);
// 			return;
// 		}
// 		this.propagation += delta;
// 		const callersCount = this.callers.length;
// 		if (callersCount) {
// 			this.propagated = true;
// 			delta /= callersCount;
// 			if (delta >= 1)
// 				this.callers.forEach(record => {
// 					return record.propagate(delta, this)
// 				});
// 			this.propagated = false;
// 		}
// 	}

	createCache(length)  @ "PiuProfile_createCache"
	deleteCache()  @ "PiuProfile_deleteCache"
	fillCache(callers, index)  @ "PiuProfile_fillCache"
	propagate(duration)  @ "PiuProfile_propagate"
	
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

const calleesOrder = "callees";
const callersOrder = "callers";

export class Profile {
	constructor(machine, path) {
		this.machine = machine;
		this.path = path;
		this.records = new Map;
		
		this.expanded = true;
		this.order = callersOrder;

		this.hits = [];
		this.host = null;
		this.hostID = 0;
		this.sorted = false;
		this.total = 0;
		
		if (path) {
			const string = system.readFileString(path);
			const json = JSON.parse(string);
			const { nodes, startTime, endTime, samples, timeDeltas } = json;
			const records = this.records;
			nodes.forEach(node => {
				const id = node.id;
				const { functionName, url, lineNumber } = node.callFrame;
				const record = new ProfileRecord(id, functionName, url, lineNumber + 1);
				records.set(id, record);
			});
			nodes.forEach(node => {
				const children = node.children;
				if (children && children.length) {
					const caller = records.get(node.id);
					children.forEach(id => {
						const callee = records.get(id);
						if (callee != caller) {
							caller.insertCallee(callee);
							callee.insertCaller(caller);
						}
					});
				}
			});
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
			
			this.hostID = nodes[0].id;
			this.name = system.getPathName(path);
			this.propagate();
		}
		else {
			this.name = "PROFILE";
		}
	}
	clear() {
		this.hits = [];
		this.host = null;
		this.sorted = false;
		this.total = 0;
		this.records.forEach(record => {
			record.callees.sorted = false;
			record.callers.sorted = false;
			record.propagation = 0;
		});
	}
	empty() {
		this.records = new Map;
		this.hits = [];
		this.host = null;
		this.expanded = true;
		this.sorted = false;
		this.total = 0;
		application.distribute("onProfileChanged", this);
	}
	getRecord(id) {
		return this.records.get(id);
	}
	propagate() {
		this.records.forEach(record => {
			record.createCache(record.callers.length);
		});
		this.records.forEach(record => {
			record.callers.forEach((caller, index) => record.fillCache(caller, index));
		});

		let hits = this.hits;
		let total = 0;
		this.records.forEach(record => {
			if (record.hitCount) {
				hits.push(record);
				record.propagate(record.duration);
				total += record.duration;
			}
			if (record.id == this.hostID) {
				this.host = record;
			}
		});
		this.total = total;
		
		this.records.forEach(record => {
			record.deleteCache();
		});
		
		application.distribute("onProfileChanged", this);
	}
	setRecord(id, name, path, line) {
		this.records.set(id, new ProfileRecord(id, name, path, line));
	}
	
};

// ASSETS

import {
	headerHeight,
	rowHeight,
	rowIndent,
} from "assets";

// BEHAVIORS

import { 
	ButtonBehavior, 
	ScrollerBehavior,
	HolderColumnBehavior,
	HolderContainerBehavior,
	RowBehavior,
	HeaderBehavior,
	TableBehavior,
	SpinnerBehavior,
} from "behaviors";

class ProfilePaneBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		this.onProfilesChanged(container);
	}
	onProfilesChanged(container, profile) {
		let scroller = container.first;
		let column = scroller.first;
		let data = this.data;
		let items = data.profiles;
		let target = null;
		column.empty(0);
		if (items.length) {
			items.forEach(item => { 
				let table = new ProfileTable(item);
				column.add(table);
				if (item == profile)
					target = table
			});
			if (target) {
				let bounds = target.bounds;
				bounds.x -= column.x;
				bounds.y -= column.y;
				scroller.reveal(bounds);
			}
		}
	}
};

class ProfileTableBehavior extends TableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		if (expandIt) {
			header.behavior.expand(header, true);
			this.onExpanded(column);
		}
		else {
			header.behavior.expand(header, false);
			column.empty(1);
		}
	}
	hold(column) {
		return ProfileHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
	onCreate(column, data) {
		this.data = data;
		if (data.expanded)
			this.onExpanded(column);
		this.selectingFile = false;
	}
	onExpanded(column) {
		const data = this.data;
		const depth = 1;
		const order = data.order;
		const total = data.total;
		if (order == calleesOrder) {
			const record = data.host;
			if (record)
				column.add(new ProfileRecordTable({ depth, order, record, total }));
		}
		else {
			const records = data.hits;
			if (!data.sorted) {
				records.sort((a, b) => {
					let result = b.duration - a.duration;
					if (!result) 
						result = a.name.localeCompare(b.name);
					if (!result) 
						result = a.id - b.id;
					return result;
				});
				data.sorted = true;
			}
			records.forEach(record => {
				if (record.callers.length)
					column.add(new ProfileRecordTable({ depth, order, record, total }));
				else
					column.add(new ProfileRecordRow({ depth, order, record, total }));
			});
		}
		
		column.add(new ProfileFooter(data));
	}
	onOrderChanged(column) {
		column.empty(1);
		this.onExpanded(column);
	}
	onProfileChanged(column, profile) {
		if (this.selectingFile)
			return;
		if (this.data == profile)
			this.onOrderChanged(column);
	}
	onSelectFileEntered(container) {
		this.selectingFile = true;
	}
	onSelectFileExited(container) {
		this.selectingFile = false;
	}
};

class ProfileHeaderBehavior extends HeaderBehavior {
	reveal(row, revealIt) {
		row.content("ORDER").visible = revealIt;
		if (this.data.path)
			row.content("CLOSE").visible = revealIt;
	}
};

class ProfileOrderButtonBehavior extends ButtonBehavior {
	onTap(button) {
		var data = this.data;
		data.order = (data.order == calleesOrder) ? callersOrder : calleesOrder;
		button.last.variant = (data.order == calleesOrder) ? 14 : 15;
		button.bubble("onOrderChanged");
	}
};

class ProfileCloseButtonBehavior extends ButtonBehavior {
	onTap(button) {
		model.doCloseProfile(this.data);
	}
};

class ProfileButtonBehavior extends ButtonBehavior {
	onCreate(container, data) {
		super.onCreate(container, data);
		container.interval = 125;
		if (data.machine.profiling) {
			container.last.variant = 16;
			container.start();
		}
		else {
			container.stop();
			container.last.variant = 24;
		}
	}
	onTimeChanged(container) {
		const icon = container.last;
		let variant = icon.variant + 1;
		if (variant == 24)
			variant = 16;
		icon.variant = variant;
	}
	onTap(container) {
		const machine = this.data.machine;
		container.active = false;
		if (machine.profiling)
			machine.doStopProfiling();
		else
			machine.doStartProfiling();
	}
};

class ProfileRecordTableBehavior extends TableBehavior {
	onCreate(column, data) {
		this.data = data;
		if (data.record[data.order].expanded) {
			this.onExpanded(column);
		}
	}
	onExpanded(column) {
		const data = this.data;
		const depth = data.depth + 1;
		const order = data.order;
		const total = data.total;
		const records = data.record[order];
		if (!records.sorted) {
			records.sort((a, b) => {
				let result = b.propagation - a.propagation;
				if (!result) 
					result = a.name.localeCompare(b.name);
				if (!result) 
					result = a.id - b.id;
				return result;
			});
			records.sorted = true;
		}
		records.forEach(record => {
			if (record[order].length)
				column.add(new ProfileRecordTable({ depth, order, record, total }));
			else
				column.add(new ProfileRecordRow({ depth, order, record, total }));
		});
	}
	toggle(column) {
		const data = this.data;
		const records = data.record[data.order];
		records.expanded = !records.expanded;
		column.bubble("onOrderChanged");
	}
};

class ProfileRecordHeaderBehavior extends HeaderBehavior {
	onMouseEntered(row, x, y) {
		super.onMouseEntered(row, x, y);
		row.distribute("onRowEntered");
	}
	onMouseExited(row, x, y) {
		super.onMouseExited(row, x, y);
		row.distribute("onRowExited");
	}
};

class ProfileRecordRowBehavior extends RowBehavior {
	onMouseEntered(row, x, y) {
		super.onMouseEntered(row, x, y);
		row.distribute("onRowEntered");
	}
	onMouseExited(row, x, y) {
		super.onMouseExited(row, x, y);
		row.distribute("onRowExited");
	}
	onTap(row) {
	}
};

class ProfileRecordButtonBehavior extends ButtonBehavior {
	onMouseEntered(button, x, y) {
		button.bubble("onSelectFileEntered");
		super.onMouseEntered(button, x, y);
	}
	onMouseExited(button, x, y) {
		super.onMouseExited(button, x, y);
		button.bubble("onSelectFileExited");
	}
	onRowEntered(button) {
		button.visible = true;
	}
	onRowExited(button) {
		button.visible = false;
	}
	onTap(row) {
		const { path, line } = this.data.record;
		if (path)
			model.selectFile(path, { line });
	}
};

// TEMPLATES

import {
	VerticalScrollbar,
} from "piu/Scrollbars";

export var ProfilePane = Container.template(function($) { return {
	left:0, right:0, top:0, bottom:0, skin:skins.paneBackground,
	Behavior: ProfilePaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:0, bottom:0, active:true, clip:true, Behavior:ScrollerBehavior, 
			contents: [
				Column($, {
					left:0, right:0, top:0, Behavior:HolderColumnBehavior, 
					contents: [
					]
				}),
				Container($, {
					left:0, right:0, top:0, height:26, clip:true, Behavior:HolderContainerBehavior,
				}),
				VerticalScrollbar($, {}),
			]
		}),
	]
}});

var ProfileTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: ProfileTableBehavior,
	contents: [
		ProfileHeader($, {}),
	],
}});
export { ProfileTable };

var ProfileHeader = Row.template(function($) { return {
	left:0, right:0, height:27, skin:skins.tableHeader, active:true,
	Behavior: ProfileHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs, variant:$.expanded ? 3 : 1 }),
		Label($, { name:"TITLE", left:0, right:0, style:styles.tableHeader, string:$.name }),
		IconButton($, { name:"ORDER", top:0, variant:($.order == calleesOrder) ? 14 : 15, state:1, active:true, visible:false, Behavior:ProfileOrderButtonBehavior }),
		$.machine
			? IconButton($, { top:0, variant:16, active:true, Behavior: ProfileButtonBehavior })
			: IconButton($, { name:"CLOSE", top:0, variant:6, state:1, active:true, visible:false, Behavior:ProfileCloseButtonBehavior })

	],
}});

var ProfileFooter = Row.template(function($) { return {
	left:0, right:0, height:3, skin:skins.tableFooter,
	contents: [
	],
}});

var ProfileRecordTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: ProfileRecordTableBehavior,
	contents: [
		ProfileRecordHeader($, {}),
	],
}});

var ProfileRecordHeader = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow, active:true,
	Behavior: ProfileRecordHeaderBehavior,
	contents: [
		Content($, { width:rowIndent + (($.depth - 1) * 20) }),
		Content($, { width:20, skin:skins.glyphs, variant:$.record[$.order].expanded ? 3 : 1 }),
		Container($, { 
			left:0, right:0, top:0, bottom:0, clip:true,
			contents: [
				Label($, { left:0, style:styles.tableRow, string:$.record.name }),
				$.record.where ?
					Container($, {
						right:0, visible:false, active:true, Behavior:ProfileRecordButtonBehavior,
						contents: [
							RoundContent($, { left:0, right:0, top:1, bottom:1, border:1, radius:4, skin:skins.profileWhere, state:1 }),
							Label($, { top:1, bottom:1, style:styles.profileWhere, string:$.record.where }),
						],
					}) : null,
			],
		}),
		Content($, { width:10 }),
		ProfileDurationContainers[$.order].call(undefined, $, {}),
		Content($, { width:10 }),
	],
}});

var ProfileRecordRow = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow, active:true,
	Behavior: ProfileRecordRowBehavior,
	contents: [
		Content($, { width:rowIndent + (($.depth - 1) * 20) }),
        Content($, { width:20 }),
		Container($, { 
			left:0, right:0, top:0, bottom:0, clip:true,
			contents: [
				Label($, { left:0, style:styles.tableRow, string:$.record.name }),
				$.record.where ?
					Container($, {
						right:0, visible:false, active:true, Behavior:ProfileRecordButtonBehavior,
						contents: [
							RoundContent($, { left:0, right:0, top:1, bottom:1, border:1, radius:4, skin:skins.profileWhere, state:1 }),
							Label($, { top:1, bottom:1, style:styles.profileWhere, string:$.record.where }),
						],
					}) : null,
			],
		}),
		Content($, { width:10 }),
		ProfileDurationContainers[$.order].call(undefined, $, {}),
		Content($, { width:10 }),
	],
}});

function toDurationString(duration) {
	duration = Math.round((duration / 1000));
	if (duration < 60000)
		return duration + " ms";
	duration = Math.round((duration / 1000));
	return duration + " s";
}

var ProfileDurationContainers = {
	callees: Container.template(function($) { return {
		width:100, height:rowHeight,
		contents: [
			Content($, { left:0, right:0, height:2, bottom:1, skin:skins.profilePercent }),
			Content($, { right:0, width:Math.round((100 * $.record.propagation) / $.total), height:2, bottom:1, skin:skins.profilePercent, state:1 }),
			Label($, { left:0, width:50, top:0, bottom:3, style:styles.profileLight, string:toDurationString($.record.duration) }),
			Label($, { left:50, width:50, top:0, bottom:4, style:styles.profileNormal, string:toDurationString($.record.propagation) }),
		],
	}}),
	callers: Container.template(function($) { return {
		width:100, height:rowHeight,
		contents: [
			Content($, { left:0, right:0, height:2, bottom:1, skin:skins.profilePercent }),
			Content($, { left:0, width:Math.round((100 * $.record.duration) / $.total), height:2, bottom:1, skin:skins.profilePercent, state:1 }),
			Label($, { left:0, width:50, top:0, bottom:4, style:styles.profileNormal, string:toDurationString($.record.duration) }),
			Label($, { left:50, width:50, top:0, bottom:3, style:styles.profileLight, string:toDurationString($.record.propagation) }),
		],
	}})
};




