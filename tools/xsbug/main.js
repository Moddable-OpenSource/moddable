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

import {} from "piu/PC";

import {
	applicationStyle,
	backgroundSkin,
	logoSkin,
	noCodeSkin
} from "assets";

import {
	CodeView,
	ErrorView,
} from "CodeView";

import {
	ConsolePane,
} from "ConsolePane";

import {
	DebugBehavior,
} from "DebugBehavior";

import {
	DebugPane,
} from "DebugPane";

import { 
	DividerLayoutBehavior,
	HorizontalDivider,
	VerticalDivider,
} from "piu/Dividers";

import {
	FilePane,
} from "FilePane";

import {
	MessagePane,
} from "MessagePane";

import {
	PreferencesView,
} from "PreferencesView";

import {
	TabsPane,
} from "TabsPane";

class Home {
	constructor(path) {
		this.depth = 0;
		this.expanded = true;
		this.items = [];
		this.name = system.getPathName(path);
		this.path = path;
		this.initialize();
	}
	close() {
		if (this.directoryNotifier)
			this.directoryNotifier.close();
	}
	initialize() {
		let directory = system.getPathDirectory(this.path);
		this.directoryNotifier = new system.DirectoryNotifier(directory, path => {
			this.onDirectoryChanged(path);
		});
	}
	onDirectoryChanged(path) {
		if (!system.fileExists(this.path))
			model.doCloseDirectory(this.path);
	}
	toJSON() {
		return {
			path: this.path,
			name: this.name,
			expanded: this.expanded,
			depth: this.depth,
			items: this.items,
		};
	}
};

class ApplicationBehavior extends DebugBehavior {
	onCreate(application) {
		global.model = this;
		super.onCreate(application);
  		application.interval = 100;
  		application.start();
		
		this.arrangement = true;
		this.featureDividerCurrent = 320;
		this.featureDividerStatus = true;
		this.HORIZONTAL_MAIN_DIVIDER = null;
		this.horizontalMainDividerCurrent = 240;
		this.horizontalMainDividerStatus = true;
		this.VERTICAL_MAIN_DIVIDER = null;
		this.verticalMainDividerCurrent = 240;
		this.verticalMainDividerStatus = true;
		
		this.findHint = "FIND";
		this.findMode = 1;
		this.findSelection = false;
		this.findString = "";
		
		this.history = {
			expanded: true,
			items: [],
		};
		this.homes = {
			expanded: true,
			items: [],
		};
		
		this.search = {
			expanded:false,
			findHint:"SEARCH",
			findMode:1,
			findString:"",
			items:[],
			message:null,
		}
		
		this.path = undefined;
		this.state = undefined;
		
		this.readPreferences();
		application.add(new MainContainer(this));
		this.doOpenView();
			
		this.test262.onPreferencesChanged();
		this.start();
		application.updateMenus();
	}

	selectMachine(machine, tab = 0) {
		if ((this.currentMachine != machine) || (this.currentTab != tab)) {
			application.distribute("onMachineDeselected", this.currentMachine, this.currentTab);
			let container = this.FEATURE;
			if (machine) {
				if (!this.currentMachine)
					container.replace(container.first, new DebugPane(this));
			}
			else {
				if ((this.currentMachine) || (this.currentTab != tab)) {
					if (tab == 0)
						container.replace(container.first, new FilePane(this));
					else
						container.replace(container.first, new MessagePane(this));
				}	
			}	
			this.currentMachine = machine
			this.currentTab = tab
			application.distribute("onMachineSelected", machine, tab);
		}
	}

/* EVENTS */
	onBreakpointsChanged(application) {
		application.invalidateMenus();
	}
	onCodeSelected(application) {
		application.invalidateMenus();
	}
	onMachineChanged(application) {
		application.invalidateMenus();
	}
	onMachineSelected(application) {
		application.invalidateMenus();
	}
	onMachinesChanged(application) {
		application.invalidateMenus();
	}
	onOpenFile(application, path) {
		let info = system.getFileInfo(path);
		if (info.directory)
			this.doOpenDirectoryCallback(path);
		else
			this.doOpenFileCallback(path);
	}
	onPathChanged(application, path) {
		application.invalidateMenus();
	}
	onQuit(application) {
		this.stop();
		application.distribute("onStateChanging", this.path);
		this.writePreferences();
		application.quit();
	}
/* APP MENU */
	canAbout() {
		return true;
	}
	canPreferences() {
		return true;
	}
	canQuit() {
		return true;
	}
	doAbout() {
		system.alert({ 
			type:"about",
			prompt:"xsbug",
			info:"Copyright 2017 Moddable Tech, Inc.\nAll rights reserved.\n\nThis application incorporates open source software from Marvell, Inc. and others.",
			buttons:["OK"]
		}, ok => {
		});
	}
	doPreferences() {
		this.selectFile("preferences", undefined);
	}
	doQuit(application) {
		this.onQuit(application);
	}
/* FILE MENU */
	canOpenFile() {
		return true;
	}
	canOpenDirectory() {
		return true;
	}
	canCloseFile() {
		return this.path ? true : false;
	}
	canCloseFiles() {
		return this.path ? true : false;
	}
	doOpenDirectory() {
		system.openDirectory({ prompt:"Open Folder" }, path => { if (path) this.doOpenDirectoryCallback(path); });
	}
	doOpenDirectoryCallback(path) {
		let items = this.homes.items;
		let home = items.find(item => item.path == path);
		if (!home) {
			home = new Home(path);
			items.push(home);
			items.sort((a, b) => a.name.compare(b.name));
			application.distribute("onHomesChanged");
		}
		return home;
	}
	doOpenFile() {
		system.openFile({ prompt:"Open File" }, path => { if (path) this.doOpenFileCallback(path); });
	}
	doOpenFileCallback(path) {
		if (path.endsWith(".js") || path.endsWith(".json") || path.endsWith(".xml") || path.endsWith(".xs"))
			this.selectFile(path);
	}
	doCloseDirectory(path) {
		let items = this.homes.items;
		let index = items.findIndex(item => item.path == path);
		if (index >= 0) {
			items[index].close();
			items.splice(index, 1);
			application.distribute("onHomesChanged");
		}
	}
	doCloseFile() {
		let items = this.history.items;
		if (items.length) {
			let item = items.shift();
			this.path = item.path;
			this.state = item.state;
			application.distribute("onHistoryChanged");
		}
		else {
			this.path = undefined;
			this.state = undefined;
		}
		this.doOpenView();
		application.distribute("onPathChanged", this.path);
	}
	doCloseFiles() {
		let items = this.history.items;
		items.length = 0;
		this.path = undefined;
		this.state = undefined;
		this.doOpenView();
		application.distribute("onPathChanged", this.path);
	}
/* HELP MENU */
	canSupport() {
		return true;
	}
	doSupport() {
		system.launchURL("http://moddable.tech");
	}

	doOpenView() {
		let Template = ErrorView;
		let path = this.path;
		if (path) {
			if (path == "preferences")
				Template = PreferencesView;
			else if (system.fileExists(path))
				Template = CodeView;
			else {
			}
		}
		else
			Template = NoCodePane;
		this.MAIN.replace(this.MAIN.first, new Template(this));
	}
	doToggleConsole(container) {
		let divider = this.arrangement ? this.HORIZONTAL_MAIN_DIVIDER : this.VERTICAL_MAIN_DIVIDER;
		divider.behavior.toggle(divider);
	}
	
	mapFile(path) {
		var alienPath = this.path;
		var separator = this.separator;
		var alienSeparator = this.alienSeparator;
		var alien = (alienPath.indexOf(alienSeparator) >= 0);
		var s1 = alienPath.split(alien ? alienSeparator : separator);
		var c1 = s1.length;
		var s2 = path.split(separator);
		var c2 = s2.length;
		var c = c1 > c2 ? c2 : c1;
		for (let i = 1; i <= c; i++) {
			if (s1[c1 - i] != s2[c2 - i]) {
				if (i > 1) {
					let remote = s1.slice(0, c1 - i + 1).join(alien ? alienSeparator : separator);
					let locale = s2.slice(0, c2 - i + 1).join(separator);
					var mappings = model.mappings;
					mappings.unshift({alien, locale, remote});
					mappings.slice(10);
					let state = model.state;
					model.doCloseFile();
					model.selectFile(path, state);
				}
				break;
			}
		}
	}
	selectFile(path, state) {
		path = this.mapPath(path, true);
		if (this.path != path) {
			let items = this.history.items;
			let index = items.findIndex(item => item.path == path);
			if (index >= 0)
				items.splice(index, 1);
			application.distribute("onStateChanging", this.state);
			if (this.path && (this.path != "preferences")) {
				items.unshift({ path:this.path, state:this.state });
				if (items.length > 32)
					items.length = 32;
			}
			application.distribute("onHistoryChanged");
			this.path = path;
			this.state = state;
			this.doOpenView();
			application.distribute("onPathChanged", this.path);
		}
		else {
			this.state = state;
			application.distribute("onStateChanged", state);
		}
	}

	readPreferences() {
		try {
			let string = system.readPreferenceString("main");
			if (string) {
				let preferences = JSON.parse(string);
				if ("arrangement" in preferences)
					this.arrangement = preferences.arrangement;
				if ("featureDividerCurrent" in preferences)
					this.featureDividerCurrent = preferences.featureDividerCurrent;
				if ("featureDividerStatus" in preferences)
					this.featureDividerStatus = preferences.featureDividerStatus;
				if ("horizontalMainDividerCurrent" in preferences)
					this.horizontalMainDividerCurrent = preferences.horizontalMainDividerCurrent;
				if ("horizontalMainDividerStatus" in preferences)
					this.horizontalMainDividerStatus = preferences.horizontalMainDividerStatus;
				if ("verticalMainDividerCurrent" in preferences)
					this.verticalMainDividerCurrent = preferences.verticalMainDividerCurrent;
				if ("verticalMainDividerStatus" in preferences)
					this.verticalMainDividerStatus = preferences.verticalMainDividerStatus;
				if ("breakOnExceptions" in preferences)
					this.breakOnExceptions = preferences.breakOnExceptions;
				if ("breakOnStart" in preferences)
					this.breakOnStart = preferences.breakOnStart;
				if ("breakpoints" in preferences) {
					this.breakpoints.expanded = preferences.breakpoints.expanded;
					preferences.breakpoints.items.forEach(item => { 
						if (system.fileExists(item.path))
							this.breakpoints.items.push(item);
					});
				}
				if ("history" in preferences) {
					this.history.expanded = preferences.history.expanded;
					preferences.history.items.forEach(item => { 
						if (system.fileExists(item.path))
							this.history.items.push(item);
					});
				}
				if ("homes" in preferences) {
					preferences.homes.items.forEach(item => { 
						if (system.fileExists(item.path)) {
							Object.setPrototypeOf(item, Home.prototype);
							item.initialize();
							this.homes.items.push(item);
						}
					});
				}
				if ("mappings" in preferences)
					this.mappings = preferences.mappings;
				if ("path" in preferences)
					this.path = preferences.path;
				if ("port" in preferences)
					this.port = preferences.port;
				if ("state" in preferences)
					this.state = preferences.state;
				if ("automaticInstruments" in preferences)
					this.automaticInstruments = preferences.automaticInstruments;
				if ("test262" in preferences) {
					if ("base" in preferences.test262)
						this.test262.base = preferences.test262.base;
					if ("filter" in preferences.test262)
						this.test262.filter = preferences.test262.filter;
				}	
			}
		}
		catch(e) {
		}
	}
	writePreferences() {
		try {
			let content;
			let preferences = {
				arrangement: this.arrangement,
				featureDividerCurrent: this.FEATURE_DIVIDER.behavior.current,
				featureDividerStatus: this.FEATURE_DIVIDER.behavior.status,
				horizontalMainDividerCurrent: (content = this.HORIZONTAL_MAIN_DIVIDER) ? content.behavior.current : this.horizontalMainDividerCurrent,
				horizontalMainDividerStatus: (content = this.HORIZONTAL_MAIN_DIVIDER) ? content.behavior.status : this.horizontalMainDividerStatus,
				verticalMainDividerCurrent: (content = this.VERTICAL_MAIN_DIVIDER) ? content.behavior.current : this.verticalMainDividerCurrent,
				verticalMainDividerStatus: (content = this.VERTICAL_MAIN_DIVIDER) ? content.behavior.status : this.verticalMainDividerStatus,
				breakOnExceptions: this.breakOnExceptions,
				breakOnStart: this.breakOnStart,
				breakpoints: this.breakpoints,
				history: this.history,
				homes: this.homes,
				mappings: this.mappings,
				path: this.path,
				port: this.port,
				state: this.state,
				automaticInstruments: this.automaticInstruments,
				test262: {
					base: this.test262.base,
					filter: this.test262.filter,
				},
			};
			let string = JSON.stringify(preferences, null, "\t");
			system.writePreferenceString("main", string);
		}
		catch(e) {
		}
	}
}

var MainContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0,
	contents: [
		TabsPane($, {}),
		Layout($, {
			left:0, right:0, top:27, bottom:0, Behavior:DividerLayoutBehavior,
			contents: [
				Container($, { 
					anchor:"FEATURE", left:0, width:0, top:0, bottom:0,
					contents: [
						FilePane($, {}),
					]
				}),
				($.arrangement) ? HorizontalLayout($, { width:0 }) : VerticalLayout($, { width:0 }),
				VerticalDivider($, { 
					anchor:"FEATURE_DIVIDER", left:$.featureDividerStatus ? $.featureDividerCurrent - 3 : 317, width:6,
					before:320, current:$.featureDividerCurrent, after:320, status:$.featureDividerStatus,
				}),
			],
		}),
	]
}));

var HorizontalLayout = Layout.template($ => ({
	right:0, top:0, bottom:0, Behavior:DividerLayoutBehavior,
	contents: [
		Container($, { 
			anchor:"MAIN", left:0, right:0, top:0, height:0,
			contents: [
 				NoCodePane($, {}),
			]
		}),
		Container($, { 
			anchor:"CONSOLE", left:0, right:0, height:0, bottom:0,
			contents: [
 				ConsolePane($, {}),
			]
		}),
		HorizontalDivider($, {  
			anchor:"HORIZONTAL_MAIN_DIVIDER",  width:6, bottom:$.horizontalMainDividerStatus ? $.horizontalMainDividerCurrent - 3 : 23, 
			before:160, current:$.horizontalMainDividerCurrent, after:26, status:$.horizontalMainDividerStatus,
		}),
	],
}));

var VerticalLayout = Layout.template($ => ({
	right:0, top:0, bottom:0, Behavior:DividerLayoutBehavior,
	contents: [
		Container($, { 
			anchor:"MAIN", left:0, width:0, top:0, bottom:0,
		}),
		Container($, { 
			anchor:"CONSOLE", width:0, right:0, top:0, bottom:0,
			contents: [
 				ConsolePane($, {}),
			]
		}),
		VerticalDivider($, {  
			anchor:"VERTICAL_MAIN_DIVIDER", width:6, right:$.verticalMainDividerStatus ? $.verticalMainDividerCurrent - 3 : 157, 
			before:160, current:$.verticalMainDividerCurrent, after:240, status:$.verticalMainDividerStatus,
		}),
	],
}));

var NoCodePane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:noCodeSkin,
	contents: [
		Content($, { skin:logoSkin }),
	],
}));

let DebuggerApplication = Application.template($ => ({
	skin:backgroundSkin,
	style:applicationStyle,
	Behavior: ApplicationBehavior,
	contents: [
	],
	menus: [
		{ 
			title:"xsbug",
			items: [
				{ title:"About xsbug", command:"About" },
				null,
				{ title:"Preferences", key:",", command:"Preferences" },
				null,
				{ title:"Services", command:"Services" },
				null,
				{ title:"Hide xsbug", key:"H", command:"HideApplication" },
				{ title:"Hide Others", option:true, key:"H", command:"HideOtherApplications" },
				{ title:"Show All", command:"ShowAllApplications" },
				null,
				{ title:"Quit xsbug", key:"Q", command:"Quit" },
			],
		},
		{ 
			title:"File",
			items: [
				{ title:"Open File...", key:"O", command:"OpenFile" },
				{ title:"Open Folder...", shift:true, key:"O", command:"OpenDirectory" },
				null,
				{ title:"Close", key:"W", command:"CloseFile" },
				{ title:"Close All", option:true, key:"W", command:"CloseFiles" },
				null,
				{ title:"Quit", key:"Q", command:"Quit" },
			],
		},
		{
			title:"Edit",
			items: [
				{ title:"Undo", key:"Z", command:"Undo" },
				{ title:"Redo", shift:true, key:"Z", command:"Redo" },
				null,
				{ title:"Cut", key:"X", command:"Cut" },
				{ title:"Copy", key:"C", command:"Copy" },
				{ title:"Paste", key:"V", command:"Paste" },
				{ title:"Clear", command:"Clear" },
				null,
				{ title:"Select All", key:"A", command:"SelectAll" },
				null,
				{ title:"Find", key:"F", command:"Find" },
				{ title:"Find Next", key:"G", command:"FindNext" },
				{ title:"Find Previous", shift:true, key:"G", command:"FindPrevious" },
				{ title:"Find Selection", key:"E", command:"FindSelection" },
				null,
				{ title:"Preferences", key:",", command:"Preferences" },
			],
		},
		{ 
			title:"Debug",
			items: [
				{ title:"Kill", key:"K", command:"Abort" },
				{ title:"Break", key:"B", command:"Break" },
				null,
				{ title:"Run", key:"R", command:"Go" },
				{ title:"Step", key:"T", command:"Step" },
				{ title:"Step In", key:"I", command:"StepIn" },
				{ title:"Step Out", key:"O", command:"StepOut" },
				null,
				{ state:0, titles: ["Set Breakpoint", "Clear Breakpoint"], shift:true, key:"B", command:"ToggleBreakpoint" },
				{ state:0, titles: ["Enable Breakpoint", "Disable Breakpoint"], command:"DisableBreakpoint" },
				{ title:"Clear All Breakpoints", option:true, key:"B", command:"ClearAllBreakpoints" },
			],
		},
		{
			title:"Help",
			items: [
				{ title:"Moddable Developer", command:"Support" },
				null,
				{ title:"About xsbug", command:"About" },
			],
		},
	],
	window: {
		title:"xsbug",
	},
}));

export default new DebuggerApplication(null, { 
	touchCount:1,
});

