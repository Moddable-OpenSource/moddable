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

class ProfileRecord {
	constructor(id, name, path, line) {
		this.id = id;
		this.name = name || "(anonymous)";
		this.path = path;
		this.line = line;
		this.hitCount = 0;
		this.duration = 0;
		this.propagation = 0;
		this.callees = [];
		this.callees.expanded = id == 0;
		this.callers = [];
		this.callers.expanded = false;
	}
	hit(delta) {
		this.hitCount++;
		this.duration += delta;
	}
	propagate(delta) {
		if (this.propagated)
			return;
		this.propagated = true;
		this.propagation += delta;
		const callersCount = this.callers.length;
		if (callersCount) {
			delta /= callersCount;
			this.callers.forEach(record => record.propagate(delta));
		}
	}
	insertCallee(callee) {
		if (this.callees.indexOf(callee) < 0)
			this.callees.push(callee);
	}
	insertCaller(caller) {
		if (this.callers.indexOf(caller) < 0)
			this.callers.push(caller);
	}
}

const calleesOrder = "callees";
const callersOrder = "callers";

export class Profile {
	constructor(machine, path) {
		this.expanded = true;
		this.machine = machine;
		this.order = callersOrder;
		this.path = path;
		this.records = [];
		if (path) {
			const string = system.readFileString(path);
			const json = JSON.parse(string);
			const { nodes, samples, timeDeltas } = json;
			const records = this.records;
			nodes.forEach(node => {
				const id = node.id;
				const { functionName, url, lineNumber } = node.callFrame;
				const record = new ProfileRecord(id, functionName, url, lineNumber + 1);
				records[id] = record;
			});
			nodes.forEach(node => {
				const caller = records[node.id];
				node.children.forEach(id => {
					const callee = records[id];
					caller.insertCallee(callee);
					callee.insertCaller(caller);
				});
			});
			const length = Math.min(samples.length, timeDeltas.length);
			for (let index = 0; index < length; index++) {
				const sample = samples[index];
				const record = records[sample];
				record.hit(timeDeltas[index]);
			}
			this.name = system.getPathName(path);
			this.propagate();
		}
		else {
			this.name = "PROFILE";
		}
	}
	clear() {
		this.records.forEach(record => {
			record.propagation = 0;
		});
		this.total = 0;
	}
	empty() {
		this.records = [];
		this.total = 0;
		application.distribute("onProfileChanged", this);
	}
	getRecord(id) {
		return this.records[id];
	}
	propagate() {
		this.records.forEach(record => {
			record.propagated = false;
		});
		let total = 0;
		this.records.forEach(record => { 
			if (record.hitCount) {
				record.propagate(record.duration);
				total += record.duration;
			}
		});
		this.total = total;
		application.distribute("onProfileChanged", this);
	}
	setRecord(id, name, path, line) {
		this.records[id] = new ProfileRecord(id, name, path, line);
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
	}
	onExpanded(column) {
		const data = this.data;
		const depth = 1;
		const order = data.order;
		const total = data.total;
		if (order == calleesOrder) {
			const records = data.records;
			if (records.length) {
				const record = data.records[0];
				column.add(new ProfileRecordTable({ depth, order, total, ...record }));
			}
		}
		else {
			const records = data.records.filter(record => record.hitCount > 0);
			records.sort((a, b) => {
				let result = b.hitCount - a.hitCount;
				if (!result) 
					result = a.name.localeCompare(b.name);
				return result;
			});
			records.forEach(record => {
				if (record.callers.length)
					column.add(new ProfileRecordTable({ depth, order, total, ...record }));
				else
					column.add(new ProfileRecordRow({ depth, order, total, ...record }));
			});
		}
		
		column.add(new ProfileFooter(data));
	}
	onOrderChanged(column) {
		column.empty(1);
		this.onExpanded(column);
	}
	onProfileChanged(column, profile) {
		if (this.data == profile)
			this.onOrderChanged(column);
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
		if (machine.profiling)
			machine.doStopProfiling();
		else
			machine.doStartProfiling();
	}
};

class ProfileRecordTableBehavior extends TableBehavior {
	onCreate(column, data) {
		this.data = data;
		if (data[data.order].expanded)
			this.onExpanded(column);
	}
	onExpanded(column) {
		const data = this.data;
		const depth = data.depth + 1;
		const order = data.order;
		const total = data.total;
		if (order == calleesOrder) {
			const items = data.callees.map(record => { return { depth, order, total, ...record }});
			items.sort((a, b) => {
				let result = b.propagation - a.propagation;
				if (!result) 
					result = a.name.localeCompare(b.name);
				return result;
			});
			for (let item of items) {
				if ((item.callees.length) && (depth < 8))
					column.add(new ProfileRecordTable(item));
				else
					column.add(new ProfileRecordRow(item));
			}
		}
		else {
			const items = data.callers.map(record => { return { depth, order, total, ...record }});
			items.sort((a, b) => {
				let result = b.propagation - a.propagation;
				if (!result) 
					result = a.name.localeCompare(b.name);
				return result;
			});
			for (let item of items) {
				if ((item.callers.length) && (depth < 8))
					column.add(new ProfileRecordTable(item));
				else
					column.add(new ProfileRecordRow(item));
			}
		}
	}
	toggle(column) {
		const data = this.data;
		const records = data[data.order];
		records.expanded = !records.expanded;
		column.bubble("onOrderChanged");
	}
};

class ProfileRecordHeaderBehavior extends HeaderBehavior {
	onTap(row) {
		super.onTap(row);
		const { path, line } = this.data;
		if (path)
			model.selectFile(path, { line });
	}
};

class ProfileRecordRowBehavior extends HeaderBehavior {
	onTap(row) {
		const { path, line } = this.data;
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
		Content($, { width:20, skin:skins.glyphs, variant:$[$.order].expanded ? 3 : 1 }),
		Label($, { left:0, right:0, style:styles.tableRow, string:$.name }),
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
		Label($, { left:0, right:0, style:styles.tableRow, string:$.name }),
		ProfileDurationContainers[$.order].call(undefined, $, {}),
		Content($, { width:10 }),
	],
}});

var ProfileDurationContainers = {
	callees: Container.template(function($) { return {
		width:100, height:rowHeight,
		contents: [
			Content($, { left:0, right:0, height:2, bottom:1, skin:skins.profilePercent }),
			Content($, { right:0, width:Math.round((100 * $.propagation) / $.total), height:2, bottom:1, skin:skins.profilePercent, state:1 }),
			Label($, { left:0, width:50, top:0, bottom:3, style:styles.profileLight, string:Math.round(($.duration / 1000)) + " ms" }),
			Label($, { left:50, width:50, top:0, bottom:4, style:styles.profileNormal, string:Math.round(($.propagation / 1000)) + " ms" }),
		],
	}}),
	callers: Container.template(function($) { return {
		width:100, height:rowHeight,
		contents: [
			Content($, { left:0, right:0, height:2, bottom:1, skin:skins.profilePercent }),
			Content($, { left:0, width:Math.round((100 * $.duration) / $.total), height:2, bottom:1, skin:skins.profilePercent, state:1 }),
			Label($, { left:0, width:50, top:0, bottom:4, style:styles.profileNormal, string:Math.round(($.duration / 1000)) + " ms" }),
			Label($, { left:50, width:50, top:0, bottom:3, style:styles.profileLight, string:Math.round(($.propagation / 1000)) + " ms" }),
		],
	}})
};




