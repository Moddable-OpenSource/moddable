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

var searchThread = new Thread("search");
var searchService = new Service(searchThread, "SearchService");

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

class FilePaneBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		container.first.first.first.distribute("onBreakpointsChanged");
		this.onHomesChanged(container);
	}
	onHomesChanged(container, home) {
		let scroller = container.first;
		let column = scroller.first;
		let data = this.data;
		let items = data.homes.items;
		let target = null;
		column.empty(1);
		if (items.length) {
			column.add(new SearchTable(data.search));
			items.forEach(item => { 
				let table = new HomeTable(item);
				column.add(table);
				if (item == home)
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

class FileButtonBehavior extends Behavior {
	changeState(button, state) {
		button.state = state;
	}
	onCreate(container, data) {
		this.data = data;
	}
	onMouseEntered(container, x, y) {
		this.changeState(container, 1);
	}
	onMouseExited(container, x, y) {
		this.changeState(container, 0);
	}
	onTap(container) {
	}
	onTouchBegan(container, id, x, y, ticks) {
		this.changeState(container, 2);
		container.captureTouch(id, x, y, ticks);
	}
	onTouchEnded(container, id, x, y, ticks) {
		if (container.hit(x, y)) {
			this.changeState(container, 1);
			this.onTap(container);
		}
	}
	onTouchMoved(container, id, x, y, ticks) {
		this.changeState(container, container.hit(x, y) ? 2 : 1);
	}
};

export class FileRowBehavior extends RowBehavior {
	onCreate(row, data) {
		super.onCreate(row, data);
		this.onPathChanged(row, model.path);
	}
	onPathChanged(row, path) {
		if (this.data.path == path) {
			row.skin = skins.fileRow;
			row.last.style = styles.fileRow; 
			this.flags |= 4;
		}
		else {
			row.skin = skins.tableRow;
			row.last.style = styles.tableRow;
			this.flags &= ~4;
		}
		this.changeState(row);
	}
	onTap(row) {
		model.selectFile(this.data.path);
	}
};

class BreakpointTableBehavior extends TableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		column.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			for (let item of data.items)
				column.add(new BreakpointRow(item));
			column.add(new BreakpointFooter(data));
		}
		else {
			header.behavior.expand(header, false);
		}
	}
	hold(column) {
		return BreakpointHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
	onCreate(column, data) {
		this.data = data;
		this.expand(column, data.expanded);
	}
	onBreakpointsChanged(column, data) {
		var data = this.data;
		this.expand(column, data.expanded);
	}
}

class BreakpointHeaderBehavior extends HeaderBehavior {
	reveal(row, revealIt) {
		row.last.visible = revealIt;
	}
};

class BreakpointRowBehavior extends RowBehavior {
	onTap(row) {
		let data = this.data;
		model.selectFile(data.path, { line:data.line });
	}
};

export class FolderTableBehavior extends TableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		if (expandIt) {
			header.behavior.expand(header, true);
			this.notifier = new system.DirectoryNotifier(data.path, path => {
				this.onDirectoryChanged(column, path);
			});
			this.onDirectoryChanged(column);
		}
		else {
			header.behavior.expand(header, false);
			this.notifier.close();
			this.notifier = null;
			column.empty(this.emptyIndex);
		}
	}
	onCreate(column, data) {
		this.data = data;
		this.emptyIndex = 1;
		if (data.expanded) {
			this.notifier = new system.DirectoryNotifier(data.path, path => {
				this.onDirectoryChanged(column, path);
			});
			this.onDirectoryChanged(column);
		}
	}
	onDirectoryChanged(column) {
		let data = this.data;
		let depth = data.depth + 1;
		let iterator = new system.DirectoryIterator(data.path);
		let formers = data.items;
		let currents = [];
		let info = iterator.next();
		while (info) {
			let name = info.name;
			let current;
			if (info.directory) {
				current = { depth, kind:"folder", name, path:info.path, expanded:false, currents:[] };
			}
			else {
				if (name.endsWith(".js") || name.endsWith(".json") || name.endsWith(".ts") || name.endsWith(".xml") || name.endsWith(".xs"))
					current = { depth, kind:"file", name, path:info.path };
				else
					current = { depth, kind:"info", name, path:info.path };
			}
			currents.push(current);
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
				column.add(new FileKindTemplates[item.kind](item));
		}
	}
	onUndisplayed(column) {
		if (this.notifier) {
			this.notifier.close();
			this.notifier = null;
		}
	}
};

export class FolderRowBehavior extends HeaderBehavior {
};

class HomeTableBehavior extends FolderTableBehavior {
	hold(column) {
		return HomeHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
	onDirectoryChanged(column) {
		super.onDirectoryChanged(column);
		if (this.data.expanded) {
			column.add(new HomeFooter(this.data));
		}
	}
};

class HomeHeaderBehavior extends FolderRowBehavior {
	reveal(row, revealIt) {
		row.content("CLOSE").visible = revealIt;
	}
};

class HomeCloseButtonBehavior extends ButtonBehavior {
	onTap(button) {
		model.doCloseDirectory(this.data.path);
	}
};

class SearchTableBehavior extends TableBehavior {
	expand(table, expandIt) {
		var data = this.data;
		var header = table.first;
		data.expanded = expandIt;
		table.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			if (data.items) {
				if (data.items.length) {
					for (let item of data.items)
						table.add(new ResultTable(item));
				}
				else
					table.add(new SearchEmpty(data));
			}
			else
				table.add(new SearchSpinner(data));
			table.add(new SearchFooter(data));
		}
		else {
			header.behavior.expand(header, false);
		}
	}
	hold(table) {
		return SearchHeader(this.data, {left:0, right:0, top:0, height:table.first.height});
	}
	onCreate(table, data) {
		table.duration = 500;
		this.data = data;
		this.onFindEdited(table);
	}
	onFindEdited(table) {
		table.stop();
		let data = this.data;
		if (data.findString) {
			searchService.search(
				findModeToPattern(data.findMode, data.findString), 
				findModeToCaseless(data.findMode), 
				model.homes.items.map(item => ({ name:item.name, path:item.path })))
			.then(results => {
				data.items = results;
				application.distribute("onSearchComplete")
			});
			if (data.items && data.items.length) {
				table.time = 0;
				table.start();
			}
			else
				data.items = null;
			this.expand(table, true);
		}
		else {
			data.items = null;
			this.expand(table, false);
		}
	}
	onFinished(table) {
		let data = this.data;
		data.items = null;
		this.expand(table, this.data.expanded);
	}
	onFlow(table, holder) {
		let data = this.data;
		data.FIND_FOCUS.delegate("onSave");;
		super.onFlow(table, holder);
		table.first.distribute("onRestore");
	}
	onHold(table, holder) {
		let data = this.data;
		table.first.distribute("onSave");
		super.onHold(table, holder);
		data.FIND_FOCUS.delegate("onRestore");;
	}
	onSearchComplete(table) {
		table.stop();
		this.expand(table, this.data.expanded);
	}
};

class SearchHeaderBehavior extends HeaderBehavior {
	onFindEdited(row) {
		let table = this.held ? this.table : row.container;
		table.behavior.onFindEdited(table);
		return true;
	}
};

class ResultTableBehavior extends TableBehavior {
	expand(table, expandIt) {
		let data = this.data;
		let header = table.first;
		data.expanded = expandIt;
		table.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			if (data.items) {
				for (let item of data.items)
					table.add(new ResultRow(item));
			}
			else {
				searchService.searchResults(data.path)
				.then(results => {
					data.items = results;
					application.distribute("onSearchResultsComplete", data)
				});
			}
		}
		else {
			header.behavior.expand(header, false);
		}
	}
	onCreate(table, data) {
		this.data = data;
		this.expand(table, data.expanded);
	}
	onSearchResultsComplete(table, data) {
		if (this.data == data)
			this.expand(table, data.expanded);
	}
};

class ResultHeaderBehavior extends HeaderBehavior {
};

class ResultRowBehavior extends RowBehavior {
	onCreate(row, data) {
		this.data = data;
		row.last.select(data.delta, data.length);
	}
	onTap(row) {
		let data = this.data;
		model.selectFile(row.container.behavior.data.path, { selection: { offset:data.offset, length:data.length } });
	}
};

// TEMPLATES

import {
	Code
}
from "piu/Code";

import {
	FindField,
	findModeToCaseless,
	findModeToPattern,
} from "FindRow";

import {
	VerticalScrollbar,
} from "piu/Scrollbars";

export var FilePane = Container.template(function($) { return {
	left:0, right:0, top:0, bottom:0, skin:skins.paneBackground,
	Behavior: FilePaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:0, bottom:0, active:true, clip:true, Behavior:ScrollerBehavior, 
			contents: [
				Column($, {
					left:0, right:0, top:0, Behavior:HolderColumnBehavior, 
					contents: [
						BreakpointTable($.breakpoints, {}),
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

var BreakpointTable = Column.template($ => ({
	left:0, right:0, active:true, 
	Behavior:BreakpointTableBehavior,
	contents: [
		BreakpointHeader($, { name:"HEADER" }),
	],
}));

var BreakpointHeader = Row.template(function($) { return {
	left:0, right:0, height:27, skin:skins.tableHeader, active:true,
	Behavior: BreakpointHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs, variant:1 }),
		Label($, { left:0, right:0, style:styles.tableHeader, string:"BREAKPOINTS" }),
		IconButton($, { top:0, variant:5, active:true, visible:false, 
			Behavior: class extends ButtonBehavior {
				onBreakpointsChanged(button) {
					button.active = model.canClearAllBreakpoints();
					this.changeState(button, button.active ? 1 : 0);
				}
				onTap(button) {
					model.doClearAllBreakpoints();
				}
			},
		}),
	],
}});

var BreakpointFooter = Row.template(function($) { return {
	left:0, right:0, height:3, skin:skins.tableFooter,
}});

var BreakpointRow = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow, active:true, 
	Behavior:BreakpointRowBehavior,
	contents: [
		Content($, { width:rowIndent, }),
		Label($, { style:styles.breakpointRowName, string:$.name }),
		Label($, { style:styles.breakpointRowLine, string:" (" + $.line + ")" }),
	]
}});

var HomeTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: HomeTableBehavior,
	contents: [
		HomeHeader($, {}),
	],
}});

var HomeHeader = Row.template(function($) { return {
	left:0, right:0, height:27, skin:skins.tableHeader, active:true,
	Behavior: HomeHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs, variant:$.expanded ? 3 : 1 }),
		Label($, { name:"TITLE", left:0, right:0, style:styles.tableHeader, string:$.name }),
		IconButton($, { name:"CLOSE", top:0, variant:6, state:1, active:true, visible:false, Behavior:HomeCloseButtonBehavior }),
	],
}});

var HomeFooter = Row.template(function($) { return {
	left:0, right:0, height:3, skin:skins.tableFooter,
}});

var FolderTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: FolderTableBehavior,
	contents: [
		FolderHeader($, {}),
	],
}});

var FolderHeader = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow, active:true,
	Behavior: FolderRowBehavior,
	contents: [
		Content($, { width:rowIndent + (($.depth - 1) * 20) }),
		Content($, { width:20, skin:skins.glyphs, variant:$.expanded ? 3 : 1 }),
		Label($, { left:0, right:0, style:styles.tableRow, string:$.name }),
	],
}});

var FileRow = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow, active:true,
	Behavior: FileRowBehavior,
	contents: [
		Content($, { width:rowIndent + (($.depth - 1) * 20) }),
		Content($, { width:20 }),
		Label($, { left:0, right:0, style:styles.tableRow, string:$.name }),
	],
}});

var InfoRow = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow,
	Behavior: RowBehavior,
	contents: [
		Content($, { width:rowIndent + (($.depth - 1) * 20) }),
		Content($, { width:20 }),
		Label($, { left:0, right:0, style:styles.infoRow, string:$.name }),
	],
}});

var FileKindTemplates = {
	file: FileRow,
	folder: FolderTable,
	home: HomeTable,
	info: InfoRow,
};


var SearchTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: SearchTableBehavior,
	contents: [
		SearchHeader($, {}),
	],
}});

var SearchHeader = Row.template(function($) { return {
	left:0, right:1, height:27, skin:skins.tableHeader, active:true,
	Behavior: SearchHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs, variant:$.expanded ? 3 : 1 }),
		FindField($, { top:2, bottom:3 }),
		Content($, { width:2 }),
	],
}});

var SearchFooter = Row.template(function($) { return {
	left:0, right:0, height:3, skin:skins.tableFooter,
}});

var SearchSpinner = Container.template($ => ({
	left:0, right:0, height:26, skin:skins.tableRow, 
	contents: [
		Content($, { skin:skins.wait, Behavior: SpinnerBehavior }),
	],
}));

var SearchEmpty = Row.template($ => ({
	left:0, right:0, height:rowHeight, skin:skins.tableRow, 
	contents: [
		Label($, { left:0, right:0, style:styles.searchEmpty, string:"Not Found!" }),
	],
}));

var ResultTable = Column.template(function($) { return {
	left:0, right:0, active:true, clip:true,
	Behavior: ResultTableBehavior,
	contents: [
		ResultHeader($, {}),
	],
}});

var ResultHeader = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow, active:true,
	Behavior: ResultHeaderBehavior,
	contents: [
		Content($, { width:rowIndent }),
		Content($, { width:20, skin:skins.glyphs, variant:$.expanded ? 3 : 1 }),
		Label($, { left:0, right:0, style:styles.tableRow, string:$.name }),
		Label($, { style:styles.resultCount, string:$.count }),
	],
}});

var ResultRow = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.resultRow, active:true,
	Behavior: ResultRowBehavior,
	contents: [
		Content($, { width:rowIndent + 40 }),
		Code($, { left:0, skin:skins.resultLabel, style:styles.resultLabel, string:$.string, active:false, variant:1 }),
	],
}});

