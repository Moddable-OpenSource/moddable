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

export class Test262Context {
	constructor() {
		this.home = {
			context:this,
			depth:0,
			expanded:true,
			items:[],
			filter:"",
			name:"",
			path:"",
		},
		this.machine = null;
		this.report = {
			context:this,
		};
		this.reset();
	}
	editFilter(name) {
		if (this.home.name != name) {
			this.home.filter = system.buildPath(this.home.path, name);
			this.home.name = name;
			this.reset();
			application.distribute("onTest262FilterChanged", this.home.filter);
		}
	}
	fail(reason) {
		let metadata = this.metadata;
		metadata.expanded = true;
		metadata.reason = reason;
		this.report.failed++;
		if (this.report.items.length < 100)
			this.report.items.push(metadata);
		
		let log = system.readFileString(this.report.logPath);
		log += metadata.path + ": " + metadata.reason + "\n";
		system.writeFileString(this.report.logPath, log);
		
		application.distribute("onTest262ReportChanged");
	}
	fromJSON(json) {
		this.locate(json.home.path);
		if (this.home.path) {
			this.home.expanded = json.home.expanded;
			this.home.items = json.home.items;
			this.home.name = json.home.name;
			this.home.filter = json.home.filter;
			this.report.expanded = json.report.expanded;
			this.report.items = json.report.items;
			this.report.logPath = json.report.logPath;
			this.report.failed = json.report.failed;
			this.report.passed = json.report.passed;
			this.report.skipped = json.report.skipped;
		}
	}
	getMetadata(path) @ "Test262Context_getMetadata"
	locate(path) {
		this.home.path = path;
		this.onDirectoryChanged();
		if (this.home.path) {
			let directory = system.getPathDirectory(this.home.path);
			this.directoryNotifier = new system.DirectoryNotifier(directory, path => {
				this.onDirectoryChanged(path);
			});
			application.distribute("onTest262PathChanged");
		}
	}
	onDirectoryChanged() {
		if (!system.fileExists(this.home.path)) {
			if (this.directoryNotifier)
				this.directoryNotifier.close();
			let home = this.home;
			home.expanded = false;
			home.items = [];
			home.name = "";
			home.path = "";
			home.filter = "";
			this.reset();
			application.distribute("onTest262PathChanged");
		}
	}
	onDisconnected(machine) {
		if (this.machine == machine) {
			this.machine = null;
			let metadata = this.metadata;
			if (metadata) {
				this.fail(`Unhandled exception`);
			}
		}
	}
	onImport(machine, path) {
		if (system.fileExists(path))
			machine.doModule(path);
		else
			machine.doGo();
	}
	onMessage(machine, message) {
		if (message == ">") {
			this.machine = machine;
			if (this.node)
				this.step();
			application.distribute("onTest262StatusChanged");
		}
		else if (message == "<") {
			let metadata = this.metadata;
			if (metadata) {
				if (metadata.paths.length > 1) {
					let path = metadata.paths.shift();
					machine.doScript(path);
				}
				else if (metadata.paths.length > 0) {
					let path = metadata.paths.shift();
					if (metadata.module)
						machine.doImport(path, metadata.async);
					else
						machine.doScript(path, metadata.async);
				}
				else {
					if (metadata.negative)
						this.fail(`Expected ${metadata.negative} but got no errors`);
					else
						this.report.passed++;
					this.machine = null;
					machine.doAbort();
				}
			}
			else {
				this.machine = null;
				machine.doAbort();
			}
		}
		else {
			let metadata = this.metadata;
			if (metadata) {
				if (metadata.paths.length || !metadata.negative)
					this.fail(message);
				else if (message.indexOf(metadata.negative) != 0)
					this.fail(`Expected ${metadata.negative} but got ${message}`);
				else
					this.report.passed++;
			}
			this.machine = null;
			machine.doAbort();
		}
	}
	openLog() {
		if (this.report.logPath)
			system.launchPath(this.report.logPath);
	}
	reset() {
		let report = this.report;
		report.expanded = false;
		report.items = [];
		report.logPath = "";
		report.failed = 0;
		report.passed = 0;
		report.skipped = 0;
		report.status = "REPORT";
		this.metadata = null;
		this.node = null;
	}
	selectFilter(path) {
		this.home.filter = path;
		this.home.name = path.slice(this.home.path.length + 1);
		this.reset();
		application.distribute("onTest262FilterChanged", this.home.filter);
	}
	start() {
		this.reset();
		let directory = system.localDirectory;
		let name = this.home.filter.slice(this.home.path.length + 1).replace(/\/|\\/g, "-");
		this.report.logPath = system.buildPath(directory, name, "txt");
		system.writeFileString(this.report.logPath, "");
		this.node = new Test262Node(null, this.home.filter);
		if (this.machine)
			this.step();
		else {
			system.alert({ 
				type:"stop",
				prompt:"xsbug",
				info:"Launch the test262 app to test...",
				buttons:["OK"]
			}, ok => {
			});
		}
		application.distribute("onTest262ReportChanged");
	}
	step() {
		let machine = this.machine;
		let node = this.node;
		while (node = node.next()) {
			let path = node.path;
			let metadata = this.metadata = this.getMetadata(path);
			if (metadata.strict || metadata.module) {
				this.node = node;
				let directory = system.getPathDirectory(this.home.path);
				let harness = system.buildPath(directory, "harness");
				metadata.paths = metadata.paths.map(name => system.buildPath(harness, name));
				metadata.paths.push(path);
				metadata.path = path;
				this.report.status = metadata.name = path.slice(this.home.path.length + 1);
				path = metadata.paths.shift();
				if (metadata.paths.length == 0) {
					if (metadata.module)
						machine.doImport(path, metadata.async);
					else
						machine.doScript(path, metadata.async);
				}
				else
					machine.doScript(path);
				return;
			}
			this.report.skipped++;
		}
		this.stop();
	}
	stop() {
		let log = system.readFileString(this.report.logPath);
		let report = this.report;
		log = this.home.filter + ": " + (report.failed + report.passed + report.skipped) + " FAIL " + report.failed + " PASS " + report.passed + " SKIP " + report.skipped + "\n\n" + log;
		system.writeFileString(this.report.logPath, log);
		this.metadata = null;
		this.node = null;
		this.report.status = "REPORT";
	}
	toJSON() {
		let home = this.home;
		let report = this.report;
		return {
			home: {
				expanded: home.expanded,
				items: home.items,
				name: home.name,
				path: home.path,
				filter: home.filter,
			},
			report: {
				expanded: report.expanded,
				items: report.items,
				logPath: report.logPath,
				failed: report.failed,
				passed: report.passed,
				skipped: report.skipped,
			},
		};
	}
}

class Test262Node {
	constructor(parent, path) {
		this.parent = parent;
		let children = [];
		if (parent) {
			let iterator = new system.DirectoryIterator(path), info;
			while (info = iterator.next()) {
				let name = info.name;
				if (info.directory || (name.endsWith(".js") && !name.endsWith("_FIXTURE.js")))
					children.push(info.path);
			}
			children.sort();
		}
		else {
			let info = system.getFileInfo(path);
			if (info) {
				let name = info.name;
				if (info.directory || (name.endsWith(".js") && !name.endsWith("_FIXTURE.js")))
					children.push(info.path);
			}
		}
		this.children = children;
		this.index = 0;
	}
	next() {
		let index = this.index;
		let children = this.children;
		if (this.index < children.length) {
			this.index = index + 1;
			let path = children[index];
			let info = system.getFileInfo(path);
			if (info.directory) {
				let child = new Test262Node(this, path);
				return child.next();
			}
			this.path = path;
			return this;
		}
		if (this.parent)
			return this.parent.next();
		return null;
	}
}

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

import { 
	FileRowBehavior,
	FolderRowBehavior,
	FolderTableBehavior,
} from "FilePane";

class Test262PaneBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		this.onTest262PathChanged(container);
	}
	onOpenLog(container) {
		this.data.test262Context.openLog();
	}
	onTest262PathChanged(container) {
		let scroller = container.first;
		let column = scroller.first;
		let test262Context = this.data.test262Context;
		column.empty(0);
		column.add(new Test262ReportTable(test262Context.report));
		if (test262Context.home.path)
			column.add(new Test262HomeTable(test262Context.home));
		else 
			column.add(new Test262LocateRow(test262Context));
	}
};

class Test262HeaderBehavior extends Behavior {
	onCreate(row, data) {
		this.data = data;
	}
	onTest262FilterChanged(row) {
		this.onTest262StatusChanged(row);
	}
	onTest262ReportChanged(row) {
		this.onTest262StatusChanged(row);
	}
	onTest262StatusChanged(row) {
		let data = this.data;
		let label = row.first.next;
		label.string = data.failed + data.passed + data.skipped;
		label = label.next.next;
		label.string = data.failed;
		label = label.next.next;
		label.string = data.passed;
		label = label.next.next;
		label.string = data.skipped;
	}
};

class Test262ButtonBehavior extends ButtonBehavior {
	onTap(button) {
		if (button.variant)
			this.data.context.start();
		else {
			this.data.context.stop();
			application.distribute("onTest262StatusChanged");
		}
	}
	onTest262FilterChanged(button, filter) {
		this.onTest262StatusChanged(button);
	}
	onTest262ReportChanged(button, filter) {
		this.onTest262StatusChanged(button);
	}
	onTest262StatusChanged(button) {
		button.variant = this.data.context.node ? 0 : 1;
	}
};

class Test262ReportTableBehavior extends TableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		column.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			for (let item of data.items)
				column.add(new Test262FailureTable(item));
			column.add(new Test262ReportFooter(data));
		}
		else {
			header.behavior.expand(header, false);
		}
	}
	hold(column) {
		return Test262ReportHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
	onCreate(column, data) {
		this.data = data;
		this.expand(column, data.expanded);
	}
	onTest262FilterChanged(column) {
		this.onTest262ReportChanged(column);
	}
	onTest262ReportChanged(column) {
		this.expand(column, this.data.expanded);
	}
}

class Test262ReportHeaderBehavior extends HeaderBehavior {
	onTest262FilterChanged(row, filter) {
		this.onTest262StatusChanged(row);
	}
	onTest262ReportChanged(row, filter) {
		this.onTest262StatusChanged(row);
	}
	onTest262StatusChanged(row) {
		row.last.previous.string = this.data.status;
	}
	reveal(row, revealIt) {
		row.last.visible = revealIt;
	}
};

class Test262FailureTableBehavior extends TableBehavior {
	expand(table, expandIt) {
		let data = this.data;
		let header = table.first;
		data.expanded = expandIt;
		table.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			table.add(new Test262FailureRow(data));
		}
		else {
			header.behavior.expand(header, false);
		}
	}
	onCreate(table, data) {
		this.data = data;
		this.expand(table, data.expanded);
	}
};

class Test262FailureHeaderBehavior extends HeaderBehavior {
};

class Test262FailureRowBehavior extends RowBehavior {
	onCreate(row, data) {
		this.data = data;
	}
	onTap(row) {
		let data = this.data;
		model.selectFile(data.path);
	}
};

class Test262LocateRowBehavior extends RowBehavior {
	doOpenDirectoryCallback(button, path) {
		this.data.locate(system.buildPath(path, "test"));
	}
	onTap(button) {
		var dictionary = { message:"Open test262 repository", prompt:"Open" };
		system.openDirectory(dictionary, path => { 
			if (path)
				button.defer("doOpenDirectoryCallback", new String(path)); 
		});
	}
};

class Test262FilterButtonBehavior extends Behavior {
	changeState(button, state) {
		button.state = state;
	}
	onCreate(button, data) {
		this.data = data;
		this.flags = 0;
		this.onTest262FilterChanged(button, model.test262Context.home.filter);
	}
	onMouseEntered(button, x, y) {
		button.variant |= 4;
		return true;
	}
	onMouseExited(button, x, y) {
		button.variant &= ~4;
		return true;
	}
	onRowEntered(button) {
		button.variant |= 2;
	}
	onRowExited(button) {
		button.variant &= ~2;
	}
	onRowSelected(button, select) {
		button.state = select ? 1 : 0;
	}
	onTap(button) {
		button.bubble("onTest262SelectFilter", this.data.path);
	}
	onTouchBegan(button, id, x, y, ticks) {
		this.variant = button.variant;
		button.variant = 5;
		button.captureTouch(id, x, y, ticks);
	}
	onTouchEnded(button, id, x, y, ticks) {
		button.variant = this.variant;
		if (button.hit(x, y)) {
			this.onTap(button);
		}
		else {
// 			this.changeState(button, 1);
		}
	}
	onTouchMoved(button, id, x, y, ticks) {
		button.variant = button.hit(x, y) ? 5 : 4;
	}
	onTest262FilterChanged(button, filter) {
		if (this.data.path == filter)
			button.variant |= 1;
		else
			button.variant &= ~1;
	}
};

class Test262FileRowBehavior extends FileRowBehavior {
	changeState(row) {
		super.changeState(row);
		let button = row.first;
		button.behavior.onRowSelected(button, this.flags & 4 ? true : false);
	}
	onMouseEntered(row, x, y) {
		super.onMouseEntered(row, x, y);
		let button = row.first;
		button.behavior.onRowEntered(button);
	}
	onMouseExited(row, x, y) {
		super.onMouseExited(row, x, y);
		let button = row.first;
		button.behavior.onRowExited(button);
	}
};

class Test262FolderRowBehavior extends FolderRowBehavior {
	changeArrowState(row, state) {
		row.first.next.next.variant = state;
	}
	onMouseEntered(row, x, y) {
		super.onMouseEntered(row, x, y);
		let button = row.first;
		button.behavior.onRowEntered(button);
	}
	onMouseExited(row, x, y) {
		super.onMouseExited(row, x, y);
		let button = row.first;
		button.behavior.onRowExited(button);
	}
};

class Test262FolderTableBehavior extends FolderTableBehavior {
	onDirectoryChanged(column) {
		let data = this.data;
		let depth = data.depth + 1;
		let iterator = new system.DirectoryIterator(data.path);
		let formers = data.items;
		let currents = [];
		let info = iterator.next();
		while (info) {
			let current;
			while (info && info.symbolicLink) {
				info = system.getSymbolicLinkInfo(info.path);
			}
			if (info) {
				let name = info.name;
				if (info.directory) {
					current = { depth, kind:"folder", name, path:info.path, expanded:false, currents:[] };
					currents.push(current);
				}
				else if (name.endsWith(".js") && !name.endsWith("FIXTURE.js")) {
					current = { depth, kind:"file", name, path:info.path };
					currents.push(current);
				}
			}
			info = iterator.next();
		}
		currents.sort((a, b) => a.name.localeCompare(b.name));
		if (formers) {
			let i = 0, j = 0;
			let c = formers.length, d = currents.length;
			while ((i < c) && (j < d)) {
				let former = formers[i];
				let current = currents[j];
				let order = former.name.localeCompare(current.name);
				if (order < 0)
					i++;
				else if (order > 0)
					j++;
				else {
					currents[j] = former;
					i++;
					j++;
				}
			}
		}
		data.items = currents;
		column.empty(this.emptyIndex);
		if (data.expanded) {
			for (let item of currents)
				column.add(new Test262FileKindTemplates[item.kind](item));
		}
	}
};

class Test262HomeTableBehavior extends Test262FolderTableBehavior {
	hold(column) {
		return Test262HomeHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
	onDirectoryChanged(column) {
		super.onDirectoryChanged(column);
		if (this.data.expanded) {
			column.add(new Test262HomeFooter(this.data));
		}
	}
	onFlow(table, holder) {
		let data = this.data;
		data.SELECT_FOCUS.delegate("onSave");;
		super.onFlow(table, holder);
		table.first.distribute("onRestore");
	}
	onHold(table, holder) {
		let data = this.data;
		table.first.distribute("onSave");
		super.onHold(table, holder);
		data.SELECT_FOCUS.delegate("onRestore");;
	}
	onTest262SelectFilter(container, path) {
		this.data.context.selectFilter(path);
	}
};

class Test262HomeHeaderBehavior extends HeaderBehavior {
};

class Test262HomeFieldBehavior extends Behavior {
	onCreate(field, data) {
		 this.data = data;
	}
	onStringChanged(field) {
		var data = this.data;
		this.data.context.editFilter(field.string);
	}
	onSave(field) {
		var data = this.data;
		field.placeholder = "";
		field.string = "";
		data.SELECT_FOCUS = null;
	}
	onRestore(field) {
		var data = this.data;
		field.placeholder = "SELECT";
		field.string = this.data.name;
		data.SELECT_FOCUS = field;
	}
	onTest262FilterChanged(field, filter) {
		field.string = this.data.name;
	}
};

// TEMPLATES

import {
	VerticalScrollbar,
} from "piu/Scrollbars";

export var Test262Pane = Container.template(function($) { return {
	left:0, right:0, top:0, bottom:0, skin:skins.paneBackground,
	Behavior: Test262PaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:27, bottom:0, active:true, clip:true, Behavior:ScrollerBehavior, 
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
		Content($, { left:0, right:0, top:26, height:1, skin:skins.paneSeparator, }),
		Test262Header($.test262Context.report, { }),
		Container($, { left:0, right:0, top:27, bottom:0 }),
	]
}});

var Test262Header = Row.template($ => ({
	left:0, top:0, height:headerHeight,
	Behavior: Test262HeaderBehavior,
	contents: [
		IconButton($, { width:26, variant:$.node ? 0 : 1, active:true, Behavior:Test262ButtonBehavior }),
		Label($, { width:40, style:styles.tableHeader, string:$.failed + $.passed + $.skipped }),
		Label($, { width:40, style:styles.test262Headers[0], string:"FAIL" }),
		Label($, { width:40, style:styles.test262Headers[1], string:$.failed }),
		Label($, { width:40, style:styles.test262Headers[2], string:"PASS" }),
		Label($, { width:40, style:styles.test262Headers[3], string:$.passed }),
		Label($, { width:40, style:styles.test262Headers[4], string:"SKIP" }),
		Label($, { width:40, style:styles.test262Headers[5], string:$.skipped }),
	],
}));


var Test262ReportTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: Test262ReportTableBehavior,
	contents: [
		Test262ReportHeader($, {}),
	],
}});

var Test262ReportHeader = Row.template(function($) { return {
	left:0, right:0, height:27, skin:skins.tableHeader, active:true,
	Behavior: Test262ReportHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs, variant:$.expanded ? 3 : 1 }),
		Label($, { left:0, right:0, style:styles.tableHeader, string:$.status }),
		IconButton($, { top:0, variant:10, active:true, visible:false, name:"onOpenLog", Behavior:ButtonBehavior }),
	],
}});

var Test262ReportFooter = Row.template(function($) { return {
	left:0, right:0, height:3, skin:skins.tableFooter,
}});

var Test262FailureTable = Column.template(function($) { return {
	left:0, right:0, active:true, clip:true,
	Behavior: Test262FailureTableBehavior,
	contents: [
		Test262FailureHeader($, {}),
	],
}});

var Test262FailureHeader = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow, active:true,
	Behavior: Test262FailureHeaderBehavior,
	contents: [
		Content($, { width:26 }),
		Content($, { width:20, skin:skins.glyphs, variant:$.expanded ? 3 : 1 }),
		Label($, { left:0, right:0, style:styles.tableRow, string:$.name }),
	],
}});

var Test262FailureRow = Row.template(function($) { return {
	left:0, right:0, skin:skins.resultRow, active:true,
	Behavior: Test262FailureRowBehavior,
	contents: [
		Content($, { width:46 }),
		Text($, { left:0, right:0, style:styles.resultLabel, string:$.reason, active:false }),
	],
}});


var Test262LocateRow = Row.template(function($) { return {
	left:0, right:0, height:27, skin:skins.tableHeader, active:true,
	Behavior: Test262LocateRowBehavior,
	contents: [
		Content($, { width:26 }),
		Label($, { left:0, right:0, style:styles.tableHeader, string:"REPOSITORY..." }),
	],
}});


var Test262FileRow = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.fileRow, active:true,
	Behavior: Test262FileRowBehavior,
	contents: [
		Content($, { width:26, skin:skins.filterTest262, active:true, Behavior:Test262FilterButtonBehavior }),
		Content($, { width:(($.depth - 1) * 20) }),
		Content($, { width:20 }),
		Label($, { left:0, right:0, style:styles.tableRow, string:$.name }),
	],
}});

var Test262FolderTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: Test262FolderTableBehavior,
	contents: [
		Test262FolderHeader($, {}),
	],
}});

var Test262FolderHeader = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow, active:true,
	Behavior: Test262FolderRowBehavior,
	contents: [
		Content($, { width:26, skin:skins.filterTest262, active:true, Behavior:Test262FilterButtonBehavior }),
		Content($, { width:(($.depth - 1) * 20) }),
		Content($, { width:20, skin:skins.glyphs, variant:$.expanded ? 3 : 1 }),
		Label($, { left:0, right:0, style:styles.tableRow, string:$.name }),
	],
}});

var Test262FileKindTemplates = {
	file: Test262FileRow,
	folder: Test262FolderTable,
};

var Test262HomeTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: Test262HomeTableBehavior,
	contents: [
		Test262HomeHeader($, {}),
	],
}});

var Test262HomeHeader = Row.template(function($) { return {
	left:0, right:0, height:27, skin:skins.tableHeader, active:true,
	Behavior: Test262HomeHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs, variant:$.expanded ? 3 : 1 }),
		Container($, { 
			left:0, right:0, top:2, bottom:3, skin:skins.fieldScroller,
			contents: [
				Field($, {
					anchor:"SELECT_FOCUS",
					left:1, right:1, top:1, bottom:1,
					clip:true,
					active:true,
					skin:skins.field,
					style:styles.field,
					placeholder:"SELECT",
					string:$.name,
					Behavior: Test262HomeFieldBehavior,
				}),
			],
		}),
		Content($, { width:26 }),
	],
}});

var Test262HomeFooter = Row.template(function($) { return {
	left:0, right:0, height:3, skin:skins.tableFooter,
}});
