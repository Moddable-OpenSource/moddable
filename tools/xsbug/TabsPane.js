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
	ButtonBehavior,
	ScrollerBehavior,
} from "behaviors";

import {
	PopupMenuBehavior,
	PopupMenu,
} from "piu/Buttons";

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
		if (model.visibleTabs[1])
			row.add(new BubblesTab(null));
		if (model.visibleTabs[2])
			row.add(new ProfileTab(null));
		if (model.visibleTabs[3])
			row.add(new SerialTab(null));
		if (model.visibleTabs[4])
			row.add(new Test262Tab(null));
		machines.forEach(machine => row.add(new MachineTab(machine)));
		this.onMeasureHorizontally(layout);
	}
	onMeasureHorizontally(layout, width) {
		let scroller = layout.first;
		let row = scroller.first;
		let sum = this.data.machines.reduce((sum, machine) => sum + styles.tab.measure(machine.title).width, 62);
		row.width = Math.max(sum, application.width);
		return width;
	}
};

class TabBehavior extends Behavior {
	changeState(container, state) {
		container.state = state;
		var content = container.first.first;
		while (content) {
			content.state = state;
			content = content.next;
		}
	}
	isSelected(container) {
		return false;
	}
	onMouseEntered(container, x, y) {
		application.cursor = cursors.arrow;
		this.changeState(container, this.isSelected(container) ? 0 : 2);
	}
	onMouseExited(container, x, y) {
		this.changeState(container, this.isSelected(container) ? 0 : 1);
	}
	onTouchBegan(container) {
		this.changeState(container, 0);
		this.select(container);
	}
	select(container) {
	}
};

class BreakpointsTabBehavior extends TabBehavior {
	isSelected(container) {
		return (model.currentMachine == null) && (model.currentTab == 0);
	}
	onCreate(container) {
		this.onMachineSelected(container, model.currentMachine, model.currentTab);
	}
	onMachineSelected(container, machine, tab) {
		this.changeState(container, (machine == null) && (tab == 0) ? 0 : 1);
	}
	select(container) {
		model.selectMachine(null, 0);
	}
};

class BubblesTabBehavior extends TabBehavior {
	isSelected(container) {
		return (model.currentMachine == null) && (model.currentTab == 1);
	}
	onCreate(container) {
		this.onMachineSelected(container, model.currentMachine, model.currentTab);
	}
	onMachineSelected(container, machine, tab) {
		this.changeState(container, (machine == null) && (tab == 1) ? 0 : 1);
	}
	select(container) {
		model.selectMachine(null, 1);
	}
};

class ProfileTabBehavior extends TabBehavior {
	isSelected(container) {
		return (model.currentMachine == null) && (model.currentTab == 2);
	}
	onCreate(container) {
		this.onMachineSelected(container, model.currentMachine, model.currentTab);
	}
	onMachineSelected(container, machine, tab) {
		this.changeState(container, (machine == null) && (tab == 2) ? 0 : 1);
	}
	select(container) {
		model.selectMachine(null, 2);
	}
};

class SerialTabBehavior extends TabBehavior {
	isSelected(container) {
		return (model.currentMachine == null) && (model.currentTab == 3);
	}
	onCreate(container) {
		this.onMachineSelected(container, model.currentMachine, model.currentTab);
	}
	onMachineSelected(container, machine, tab) {
		this.changeState(container, (machine == null) && (tab == 3) ? 0 : 1);
	}
	select(container) {
		model.selectMachine(null, 3);
	}
};

class Test262TabBehavior extends TabBehavior {
	isSelected(container) {
		return (model.currentMachine == null) && (model.currentTab == 4);
	}
	onCreate(container) {
		this.onMachineSelected(container, model.currentMachine, model.currentTab);
	}
	onMachineSelected(container, machine, tab) {
		this.changeState(container, (machine == null) && (tab == 4) ? 0 : 1);
	}
	select(container) {
		model.selectMachine(null, 4);
	}
};

class MachineTabBehavior extends TabBehavior {
	changeState(container, state) {
		container.state = container.first.last.state = state;
	}
	isSelected(container) {
		return model.currentMachine == this.machine ;
	}
	onCloseTab(container) {
		this.machine.close();
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
	onMouseEntered(container, x, y) {
		container.last.visible = !this.machine.broken;
		super.onMouseEntered(container, x, y);
	}
	onMouseExited(container, x, y) {
		container.last.visible = false;
		super.onMouseExited(container, x, y);
	}
	select(container) {
		model.selectMachine(this.machine);
	}
};

class ColorsButtonBehavior extends ButtonBehavior {
	onCreate(container) {
		const data = {
			button: container,
			items: [
				{ title:"Light Colors", value:0 },
				{ title:"Dark Colors", value:1 }
			],
		};
		if (system.platform == "mac")
			data.items.push({ title:"Default", value:2 });
		data.selection = data.items.findIndex(item => item.value == model.colors);
		super.onCreate(container, data);
	}
	onMenuSelected(container, index) {
		const data = this.data;
		if ((index >= 0) && (data.selection != index)) {
			let item = data.items[index];
			data.selection = index;
			model.colors = data.items[index].value;
			application.delegate("onColorsChanged");
		}
	}
	onTap(container) {
		application.add(new PopupMenu(this.data, { Behavior:ColorsMenuBehavior } ));
	}
}

class ColorsMenuBehavior extends PopupMenuBehavior {
	onFitVertically(layout, value) {
		let data = this.data;
		let button = data.button;
		let container = layout.first;
		let scroller = container.first;
		let size = scroller.first.measure();
		let y = button.y + button.height + 1
		let height = Math.min(size.height, application.height - y - 20);
		container.coordinates = { right:0, width:size.width + 30, top:y, height:height + 10 };
		scroller.coordinates = { left:10, width:size.width + 10, top:0, height:height };
		scroller.first.content(data.selection).first.visible = true;
		return value;
	}
}

export var TabsPane = Layout.template($ => ({
	left:0, right:0, top:0, height:27, skin:skins.tabsPane, Behavior:TabsPaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:27, top:0, bottom:1, clip:true, active:true, Behavior:ScrollerBehavior, 
			contents: [
				Row($, {
					left:0, width:0, top:0, bottom:0, 
					contents: [
					]
				}),
			]
		}),
		IconButton($, {
			right:0, variant:13, 
			Behavior: ColorsButtonBehavior,
		}),
	]
}));

var BreakpointsTab = Container.template($ => ({
	width:52, top:0, bottom:0, skin:skins.tab, active:true, Behavior:BreakpointsTabBehavior,
	contents: [
		Content($, { left:5, skin:skins.lineBreakpoint, state:1 }),
		Label($, { 
			left:5, right:5, height:16, style:styles.tabBreakpoint,
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

var BubblesTab = Container.template($ => ({
	width:52, top:0, bottom:0, skin:skins.tab, active:true, Behavior:BubblesTabBehavior,
	contents: [
		Label($, { 
			left:2, right:5, height:16, skin:skins.tabBubble, style:styles.tabBubble,
			Behavior: class extends Behavior {
				onCreate(label) {
					this.onBubblesChanged(label);
				}
				onBubblesChanged(label) {
					label.string = model.bubbles.items.length;
				}
			},
		}),
	],
}));

var ProfileTab = Container.template($ => ({
	top:0, bottom:0, skin:skins.tab, active:true, Behavior:ProfileTabBehavior,
	contents: [
		Container($, { 
			top:0, bottom:0,
			contents: [
				Label($, { top:0, bottom:0, style:styles.tabTest262, string:"PROFILE" }),
			],
		}),
	],
}));

var SerialTab = Container.template($ => ({
	top:0, bottom:0, skin:skins.tab, active:true, Behavior:SerialTabBehavior,
	contents: [
		Container($, { 
			top:0, bottom:0,
			contents: [
				Label($, { top:0, bottom:0, style:styles.tabTest262, string:"SERIAL" }),
			],
		}),
	],
}));

var Test262Tab = Container.template($ => ({
	top:0, bottom:0, skin:skins.tab, active:true, Behavior:Test262TabBehavior,
	contents: [
		Container($, { 
			top:0, bottom:0,
			contents: [
				Label($, { top:0, bottom:0, style:styles.tabTest262, string:"TEST" }),
			],
		}),
	],
}));

var MachineTab = Container.template($ => ({
	top:0, bottom:0, skin:skins.tab, active:true, Behavior:MachineTabBehavior,
	contents: [
		Container($, { 
			top:0, bottom:0,
			contents: [
				Content($, { left:6, visible:$.broken, skin:skins.lineCall, state:1 }),
				Label($, { top:0, bottom:0, style:styles.tab, string:$.title }),
			],
		}),
		IconButton($, { left:0, width:26, active:true, visible:false, variant:6, state:1, Behavior:ButtonBehavior, name:"onCloseTab" }),
	],
}));
