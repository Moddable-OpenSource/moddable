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

import {
	tabBreakpointSkin,
	tabBreakpointStyle,
	tabBrokenSkin,
	tabSkin,
	tabStyle,
	tabsPaneSkin,
	buttonsSkin,
} from "assets";

import {
	ButtonBehavior,
	ScrollerBehavior,
} from "behaviors";

class TabsPaneBehavior extends Behavior {
	onCreate(layout, data) {
		this.data = data;
		this.onMachinesChanged(layout, data.machines);
	}
	onMachinesChanged(layout, machines) {
		let scroller = layout.first;
		let row = scroller.first;
		row.empty(0);
		row.add(new BreakpointsTab(null));
		machines.forEach(machine => row.add(new Tab(machine)));
		this.onMeasureHorizontally(layout);
	}
	onMeasureHorizontally(layout, width) {
		let scroller = layout.first;
		let row = scroller.first;
		let sum = this.data.machines.reduce((sum, machine) => sum + tabStyle.measure(machine.title).width, 62);
		row.width = Math.max(sum, application.width);
		return width;
	}
};

class BreakpointsTabBehavior extends Behavior {
	changeState(container, state) {
		container.state = state;
		var content = container.first.first;
		while (content) {
			content.state = state;
			content = content.next;
		}
	}
	onCreate(container, machine) {
		this.machine = machine;
		this.onMachineSelected(container, model.currentMachine);
	}
	onMachineChanged(container, machine) {
		if (this.machine == machine) {
			container.first.first.visible = machine.broken;
		}
	}
	onMachineSelected(container, machine) {
		this.changeState(container, this.machine == machine ? 0 : 1);
	}
	onTouchBegan(container) {
		this.changeState(container, 0);
		model.selectMachine(this.machine);
	}
	onMouseEntered(container, x, y) {
		application.cursor = cursors.arrow;
		this.changeState(container, this.machine != model.currentMachine ? 2 : 0);
	}
	onMouseExited(container, x, y) {
		this.changeState(container, this.machine != model.currentMachine ? 1 : 0);
	}
};

class TabBehavior extends BreakpointsTabBehavior {
	onCloseTab(container) {
		this.machine.close();
	}
	onMouseEntered(container, x, y) {
		container.last.visible = !this.machine.broken;
		super.onMouseEntered(container, x, y);
	}
	onMouseExited(container, x, y) {
		container.last.visible = false;
		super.onMouseExited(container, x, y);
	}
};

export var TabsPane = Layout.template($ => ({
	left:0, right:0, top:0, height:27, skin:tabsPaneSkin, Behavior:TabsPaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:0, bottom:1, clip:true, active:true, Behavior:ScrollerBehavior, 
			contents: [
				Row($, {
					left:0, width:0, top:0, bottom:0, 
					contents: [
					]
				}),
			]
		}),
	]
}));

var Tab = Container.template($ => ({
	top:0, bottom:0, skin:tabSkin, active:true, Behavior:TabBehavior,
	contents: [
		Container($, { 
			top:0, bottom:0,
			contents: [
				Content($, { left:3, width:20, visible:$.broken, skin:tabBrokenSkin, }),
				Label($, { top:0, bottom:0, style:tabStyle, string:$.title }),
			],
		}),
		Content($, { left:0, width:26, active:true, visible:false, skin:buttonsSkin, variant:6, state:1, Behavior:ButtonBehavior, name:"onCloseTab" }),
	],
}));

var BreakpointsTab = Container.template($ => ({
	width:52, top:0, bottom:0, skin:tabSkin, active:true, Behavior:BreakpointsTabBehavior,
	contents: [
		Label($, { 
			left:5, right:5, height:16, skin:tabBreakpointSkin, style:tabBreakpointStyle,
			Behavior: class extends Behavior {
				onCreate(label) {
					this.onBreakpointsChanged(label);
				}
				onBreakpointsChanged(label) {
					label.string = model.breakpoints.items.length;
				}
			},
		}),
	],
}));
