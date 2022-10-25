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

import { 
	mxFramesView,
	mxLocalsView,
	mxGlobalsView,
	mxFilesView,
	mxBreakpointsView,
	mxModulesView,
	mxInstrumentsView,
} from "DebugBehavior";

// ASSETS

import {
	headerHeight,
	footerHeight,
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

class DebugPaneBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onMachineChanged(container, machine) {
		if (this.machine == machine) {
			let spinner = container.last;
			let scroller = container.first;
			let column = scroller.first;
			let data = this.data;
			spinner.empty(0);
			column.empty(0);
			if (machine.instrumentsView.lines.length > 0) {
				column.add(ProfileTable(machine.profile, {}));
				column.add(InstrumentTable(data, {}));
			}
			if (machine.broken) {
				column.add(CallTable(data, {}));
				column.add(DebugTable(data, { Behavior:LocalsTableBehavior }));
				column.add(DebugTable(data, { Behavior:ModulesTableBehavior }));
				column.add(DebugTable(data, { Behavior:GlobalsTableBehavior }));
			}
		}
	}
	onMachineDeselected(container, machine) {
		if (machine)
			machine.debugScroll = container.first.scroll;
	}
	onMachineSelected(container, machine) {
		this.machine = machine;
		if (machine) {
			this.onMachineChanged(container, machine);
			container.first.scroll = machine.debugScroll;
		}
	}
};

class DebugButtonBehavior extends ButtonBehavior {
	onCreate(container, data) {
		super.onCreate(container, data);
		this.can = "can" + container.name;
		this.do = "do" + container.name;
	}
	onMachineChanged(container, machine) {
		container.active = this.data[this.can]();
		this.changeState(container, container.active ? 1 : 0);
	}
	onMachineSelected(container, machine) {
		container.active = this.data[this.can]();
		this.changeState(container, container.active ? 1 : 0);
	}
	onTap(container) {
		this.data[this.do]();
	}
};

class DebugTableBehavior extends TableBehavior {
	addRows(column) {
		let header = column.first;
		let view = this.view;
		column.empty(1);
		if (view && view.expanded) {
			let data = this.data;
			header.behavior.expand(header, true);
			view.lines.forEach(data => column.add(new (this.lineTemplate)(data)));
			column.add(new this.footerTemplate(data));
		}
		else
			header.behavior.expand(header, false);
		//model.onHover(shell);
	}
	hold(column) {
		let header = column.first;
		let result = DebugHeader(this.data, {left:0, right:0, top:0, height:header.height, skin:header.skin});
		let view = this.view;
		result.behavior.expand(result, view && view.expanded);
		result.last.string = header.last.string;
		return result;
	}
	onCreate(column, data) {
		let machine = data.currentMachine;
		let view = machine ? machine.views[this.viewIndex] : null;
		this.data = data;
		this.machine = machine;
		this.view = view;	
		this.addRows(column);
	}
	onMachineSelected(column, machine) {
		this.machine = machine;
		if (machine) {
			this.view = machine.views[this.viewIndex];
			this.addRows(column);
			column.visible = true;
		}
		else {
			this.view = null;
			column.empty(1);
			column.visible = false;
		}
	}
	onMachineViewChanged(column, viewIndex) {
		if (this.viewIndex == viewIndex)
			this.addRows(column);
	}
	toggle(column) {
		var view = this.view;
		if (view)
			view.expanded = !view.expanded;
		this.addRows(column);
	}
	trigger(column, row) {
	}
};

class DebugHeaderBehavior extends HeaderBehavior {
};

class DebugRowBehavior extends RowBehavior {
	onTap(row) {
		row.bubble("doToggleItem", this.data.value);
	}
};

class CallTableBehavior extends DebugTableBehavior {
	onCreate(column, data) {
		this.footerTemplate = CallFooter;
		this.lineTemplate = CallRow;
		this.viewIndex = mxFramesView;
		super.onCreate(column, data);
	}
};

class CallHeaderBehavior extends HeaderBehavior {
};

class CallRowBehavior extends RowBehavior {
	onCreate(row, data) {
		super.onCreate(row, data);
		this.select(row, this.data.value == model.currentMachine.frame);
	}
	onMachineFrameChanged(row, value) {
		this.select(row, this.data.value == value);
	}
	onTap(row) {
		row.bubble("doSelectItem", this.data.value);
	}
	select(row, selectIt) {
		if (selectIt) {
			row.skin = skins.callRow; 
			row.last.style = styles.callRow; 
			this.flags |= 4;
		}
		else {
			row.skin = skins.tableRow; 
			row.last.style = styles.tableRow;
			this.flags &= ~4;
		}
		this.changeState(row);
	}
};

class LocalsTableBehavior extends DebugTableBehavior {
	onCreate(column, data) {
		this.footerTemplate = DebugFooter;
		this.lineTemplate = DebugRow;
		this.viewIndex = mxLocalsView;
		super.onCreate(column, data);
		column.content("HEADER").last.string = "LOCALS";
	}
};

class ModulesTableBehavior extends DebugTableBehavior {
	onCreate(column, data) {
		this.footerTemplate = DebugFooter;
		this.lineTemplate = DebugRow;
		this.viewIndex = mxModulesView;
		super.onCreate(column, data);
		column.content("HEADER").last.string = "MODULES";
	}
};

class GlobalsTableBehavior extends DebugTableBehavior {
	onCreate(column, data) {
		this.footerTemplate = DebugFooter;
		this.lineTemplate = DebugRow;
		this.viewIndex = mxGlobalsView;
		super.onCreate(column, data);
		column.content("HEADER").last.string = "GLOBALS";
	}
};

class InstrumentTableBehavior extends DebugTableBehavior {
	addRows(column) {
		let header = column.first;
		let view = this.view;
		column.empty(1);
		if (view && view.expanded) {
			let data = this.data;
			header.behavior.expand(header, true);
			for (let line of view.lines) {
				if (!line.skip)
					column.add(new InstrumentRow(line));
			}
			column.add(new this.footerTemplate(data));
		}
		else
			header.behavior.expand(header, false);
		//model.onHover(shell);
	}
	onCreate(column, data) {
		this.footerTemplate = DebugFooter;
		this.lineTemplate = InstrumentRow;
		this.viewIndex = mxInstrumentsView;
		super.onCreate(column, data);
		column.content("HEADER").last.string = "INSTRUMENTS";
	}
}

class InstrumentHeaderBehavior extends HeaderBehavior {
};

class InstrumentRowBehavior extends RowBehavior {
};

class InstrumentPortBehavior extends Behavior {
	onCreate(port, data) {
		this.hover = -1;
		this.track = data;
	}
	onMouseEntered(port, x, y) {
		this.hoverTo(port, x);
		port.invalidate();
	}
	onMouseMoved(port, x, y) {
		this.hoverTo(port, x);
		port.invalidate();
	}
	onMouseExited(port, x, y) {
		port.previous.string = this.track.value;
		this.hover = -1;
		port.invalidate();
	}
	hoverTo(port, x) {
		x -= port.x;
		let track = this.track;
		let machine = track.machine;
		let values = track.values;
		let samplesCount = machine.samplesCount;
		let samplesIndex = machine.samplesIndex + 1;
		let samplesLoop = machine.samplesLoop;
		let src = samplesLoop ? samplesCount : samplesIndex;
		let dst = Math.floor(port.width / 3);
		let count = (src < dst) ? src : dst;
		let index = samplesIndex - count;
		let hover = -1;
		let i = Math.floor(x / 3);
		let value;
		if (index < 0) {
			index += samplesCount;
			if (i < (samplesCount - index)) {
				hover = x;
				value = values[i];
			}
			i -= (samplesCount - index);
		}
		if ((hover < 0) && (i < samplesIndex)) {
			hover = x;
			value = values[i];
		}
		if (hover < 0)
			port.previous.string = track.value;
		else
			port.previous.string = value.toString() + (track.slash ? track.slash.unit : track.unit);
		this.hover = hover;
	}
	onDraw(port) {
		let track = this.track;
		let machine = track.machine;
		let max = track.max;
		let min = track.min;
		let values = track.values;
		let samplesCount = machine.samplesCount;
		let samplesIndex = machine.samplesIndex + 1;
		let samplesLoop = machine.samplesLoop;
		let width = port.width;
		let height = port.height - 2;

		let src = samplesLoop ? samplesCount : samplesIndex;
		let dst = Math.floor(width / 3);
		let count = (src < dst) ? src : dst;
		let index = samplesIndex - count;
		let x = 0, y = 1;
		let range = (max > min) ? (height / (max - min)) : 1;
		let hover = this.hover;
		if (hover < 0)
			hover = (count * 3) - 2;

		if (index < 0) {
			index += samplesCount;
			while (index < samplesCount) {
				let delta = Math.max(Math.round((values[index] - min) * range), 1);
				if ((x <= hover) && (hover < (x + 3))) {
					port.fillColor(colors.instrumentBarHover, x, y + height - delta, 2, delta);
				}
				else
					port.fillColor(colors.instrumentBar, x, y + height - delta, 2, delta);
				index++;
				x += 3;
			}
			index = 0;
		}
		while (index < samplesIndex) {
			let delta = Math.max(Math.round((values[index] - min) * range), 1);
			if ((x <= hover) && (hover < x + 3)) {
				port.fillColor(colors.instrumentBarHover, x, y + height - delta, 2, delta);
			}
			else
				port.fillColor(colors.instrumentBar, x, y + height - delta, 2, delta);
			index++;
			x += 3;
		}
		port.fillColor(colors.instrumentLine, 0, height + 1, width, 1);
	}
	onMachineSampled(port) {
		port.invalidate();
	}
};

class InstrumentLabelBehavior extends Behavior {
	onCreate(label, data) {
		this.track = data;
	}
	onMachineSampled(label) {
		label.string = this.track.value;
	}
};

// TEMPLATES

import {
	VerticalScrollbar,
} from "piu/Scrollbars";

import {
	ProfileTable,
} from "ProfilePane";

export var DebugPane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:skins.paneBackground, 
	Behavior: DebugPaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:27, bottom:0, clip:true, active:true, Behavior:ScrollerBehavior, 
			contents: [
				Column($, {
					left:0, right:1, top:0, clip:true, Behavior:HolderColumnBehavior, 
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
		DebugToolsHeader(model, { top:0 }),
		Container($, { left:0, right:0, top:27, bottom:0 }),
	]
}));

var DebugToolsHeader = Row.template($ => ({
	left:4, right:4, height:headerHeight,
	contents: [
		DebugToolButton($, { name:"Abort", variant:0 }),
		DebugToolButton($, { name:"Break", variant:12 }),
		DebugToolButton($, { name:"Go", variant:1 }),
		DebugToolButton($, { name:"Step", variant:2 }),
		DebugToolButton($, { name:"StepIn", variant:3 }),
		DebugToolButton($, { name:"StepOut", variant:4 }),
	],
}));


var DebugToolButton = IconButton.template($ => ({
	active:false, Behavior: DebugButtonBehavior,
}));


var CallTable = Column.template($ => ({
	left:0, right:0, active:true, 
	Behavior:CallTableBehavior,
	contents: [
		CallHeader($, { name:"HEADER" }),
	],
}));

var CallHeader = Row.template(function($) { return {
	left:0, right:0, height:headerHeight, skin:skins.tableHeader, active:true,
	Behavior: CallHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs }),
		Label($, { left:0, right:0, style:styles.tableHeader, string:"CALLS" }),
	],
}});

var CallFooter = Row.template(function($) { return {
	left:0, right:0, height:footerHeight, skin:skins.tableFooter,
}});

var CallRow = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow, active:true, 
	Behavior:CallRowBehavior,
	contents: [
		Content($, { width:rowIndent, }),
		Label($, { style:styles.tableRow, string:$.name }),
	]
}});

var DebugTable = Column.template($ => ({
	left:0, right:0, active:true,
	contents: [
		DebugHeader($, { name:"HEADER" }),
	],
}));

var DebugHeader = Row.template(function($) { return {
	left:0, right:0, height:headerHeight, skin:skins.tableHeader, active:true,
	Behavior: DebugHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs }),
		Label($, { left:0, right:0, style:styles.tableHeader }),
	],
}});

var DebugFooter = Row.template(function($) { return {
	left:0, right:0, height:footerHeight, skin:skins.tableFooter,
}});

var DebugRow = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow, active:$.state > 0, 
	Behavior:DebugRowBehavior,
	contents: [
		Content($, { width:rowIndent + ($.column * 20) }),
		Content($, { skin:skins.glyphs, variant:$.state }),
		Label($, { style:styles.debugRowName, string:$.name, state:$.variant }),
		Label($, { style:styles.debugRowValue, string:$.state == 0 ? (" = " + $.value) : "", state:$.variant }),
	]
}});

var InstrumentTable = Column.template($ => ({
	left:0, right:0, active:true,
	Behavior: InstrumentTableBehavior,
	contents: [
		InstrumentHeader($, { name:"HEADER" }),
	],
}));

var InstrumentHeader = Row.template(function($) { return {
	left:0, right:0, height:headerHeight, skin:skins.tableHeader, active:true,
	Behavior: InstrumentHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs }),
		Label($, { left:0, right:0, style:styles.tableHeader, string:"INSTRUMENTS" }),
	],
}});

var InstrumentFooter = Row.template(function($) { return {
	left:0, right:0, height:footerHeight, skin:skins.tableFooter,
}});

var InstrumentRow = Container.template(function($) { return {
	left:0, right:0, height:36, skin:skins.tableRow,
	Behavior: InstrumentRowBehavior,
	contents: [
		Label($, { left:rowIndent + 10, right:10, top:4, bottom:4, style:styles.instrumentRowName, string:$.name } ),
		Label($, { left:rowIndent + 10, right:10, top:4, bottom:4, style:styles.instrumentRowValue, string:$.value, Behavior:InstrumentLabelBehavior } ),
		Port($, { left:rowIndent + 10, right:10, height:14, bottom:4, active:true, Behavior:InstrumentPortBehavior } ),
	]
}});

