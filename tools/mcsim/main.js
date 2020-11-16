/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

import {} from "piu/PC";

import {
	applicationStyle,
	backgroundSkin,
	buttonsSkin,
	controlsMenuSkin,
	controlsMenuGlyphSkin,
	controlsMenuItemSkin,
	controlsMenuItemStyle,
	dotSkin,
	paneBorderSkin,
	paneHeaderSkin,
	paneHeaderStyle,
	paneFooterLeftStyle,
	paneFooterRightStyle,
} from "assets";

import {
	ButtonBehavior,
} from "piu/Buttons";

import { 
	DividerLayoutBehavior,
	HorizontalDivider,
	VerticalDivider,
} from "piu/Dividers";

import {
	Button,
	ControlsPane,
} from "ControlsPane";

import {
	DeviceContainer,
	DeviceScreen,
} from "DevicePane";

class NoDeviceBehavior extends Behavior {
}

let noDevice = {
	title:"NO SIMULATORS",
	ControlsTemplate: Container.template($ => ({
		left:0, right:0, top:0, bottom:0,
		contents:[
			Button({ label:"Locate..." }, {
				Behavior: class extends ButtonBehavior {
					onTap(container) {
						container.bubble("doLocateSimulators");
					}
				}
			}),
		]
	})),
	DeviceTemplate: DeviceContainer.template($ => ({ Behavior:NoDeviceBehavior, width:480, height:320, contents:[
		DeviceScreen($, { width:480, height:320 }),
	]})),
};

class ApplicationBehavior extends Behavior {
	onCreate(application) {
		let extension = (system.platform == "win") ? "dll" : "so";
		global.model = this;
  		application.interval = 100;
		
		this.keys = {};
		this.controlsCurrent = 320;
		this.controlsStatus = false;
		this.infoStatus = true;
		
		let path = system.applicationPath;
		path = system.getPathDirectory(path);
		path = system.getPathDirectory(path);
		path = system.getPathDirectory(path);
		path = system.getPathDirectory(path);
		path = system.buildPath(path, "simulators");
		
		this.devicesPath = system.fileExists(path) ? path : "";
		this.devices = [];
		this.deviceIndex = -1;
		this.deviceRotation = 0;

		this.libraryPath = "";
		this.localLibraryPath = system.buildPath(system.localDirectory, "mc", extension);
		this.archivePath = "";
		this.localArchivePath = system.buildPath(system.localDirectory, "mc", "xsa");
		
		this.readPreferences();
// 		
// 		try {
// 			let directory = system.getPathDirectory(system.applicationPath);
// 			directory = system.buildPath(directory, "autorun");
// 			if (system.fileExists(directory)) {
// 				let devicesPath = system.buildPath(directory, "simulators");
// 				let libraryPath = system.buildPath(directory, "mc", extension);
// 				if (system.fileExists(devicesPath) && system.fileExists(libraryPath)) {
// 					this.devicesPath = devicesPath;
// 					this.libraryPath = libraryPath;
// 				}
// 			}
// 		}
// 		catch {
// 		}

		application.add(new MainContainer(this));
	}
	onDisplaying(application) {
		if (this.devicesPath)
			this.reloadDevices(application);
		else
			this.selectDevice(application, -1);
		this.launchScreen();
	}
	onKeyDown(application, key) {
		if (this.keys[key])
			return;
		this.keys[key] = true;
		this.DEVICE.first.delegate("onKeyDown", key);
	}
	onKeyUp(application, key) {
		this.keys[key] = false;
		this.DEVICE.first.delegate("onKeyUp", key);
	}
	launchScreen() {
		if (this.libraryPath) {
			system.copyFile(this.libraryPath, this.localLibraryPath);
			if (this.archivePath) {
				system.copyFile(this.archivePath, this.localArchivePath);
				this.SCREEN.launch(this.localLibraryPath, this.localArchivePath);
			}
			else
				this.SCREEN.launch(this.localLibraryPath);
			this.DEVICE.first.defer("onLaunch");
		}
		application.updateMenus();
		application.distribute("onInfoChanged");
	}
	quitScreen() {
		if (this.SCREEN)
			this.SCREEN.quit();
		if (system.fileExists(this.localArchivePath))
			system.deleteFile(this.localArchivePath);
		if (system.fileExists(this.localLibraryPath))
			system.deleteFile(this.localLibraryPath);
	}
	reloadDevices(application, flag) {
		let devices = this.devices = [];
		let index = this.deviceIndex;

		this.selectDevice(application, -1);
		
		application.purge();
			
		let iterator = new system.DirectoryIterator(this.devicesPath);
		let info = iterator.next();
		while (info) {
			if (!info.directory) {
				if (info.name.endsWith(".js")) {
					try {
						let compartment = new Compartment({...globalThis, Date, Math});
						let device = compartment.importNow(info.path).default;
						if (device && (("DeviceTemplate" in device) || ("DeviceTemplates" in device))) {
							device.compartment = compartment;
							device.default = device;
							if (!device.sortingTitle)
								device.sortingTitle = device.title;
							devices.push(device);
							let name = device.applicationName;
							if (name) {
								if (system.platform == "win") {
									name = name.replaceAll("/", "\\\\");
									name += "\\\\mc.dll";
								}
								else
									name += "/mc.so";
								device.applicationFilter = new RegExp(name);
							}
						}
					}
					catch (e) {
					}
				}
			}
			info = iterator.next();
		}
		devices.sort((a, b) => a.sortingTitle.compare(b.sortingTitle));
		
		let length = devices.length;
		if (index < 0)
			index = 0;
		else if (index >= length)
			index = length - 1;
		this.selectDevice(application, index);
	}
	selectDevice(application, index) {
		application.distribute("onDeviceUnselected");
	
		this.deviceIndex = index;
		let device = (index < 0) ? noDevice : this.devices[index];
		let rotation = this.deviceRotation;
		let Templates = device.DeviceTemplates;
		let Template = device.DeviceTemplates;
		if (Templates) {
			if (rotation in Templates)
				Template = Templates[rotation];
			else
				Template = Templates[0];
		}
		else
			Template = device.DeviceTemplate;
		
		let screenContainer = new Template(device);
		let container = this.DEVICE;
		container.replace(container.first, screenContainer);
		
		if (this.CONTROLS) {
			let controlsColumn = new device.ControlsTemplate(device);
			let scroller = this.CONTROLS.first;
			scroller.replace(scroller.first, controlsColumn);
		}
		
// 		let path = device.applicationPath;
// 		if (path && system.fileExists(path))
// 			this.libraryPath = path;
// 		else
// 			this.libraryPath = "";
			
		application.distribute("onDeviceSelected", device);
	}
	
/* EVENTS */
	onAbort(application) {
	 this.doCloseFile(application);
	}
	onOpenFile(application, path) {
		let info = system.getFileInfo(path);
		if (info.directory)
			application.defer("doLocateSimulatorsCallback", new String(path));
		else
			application.defer("doOpenFileCallback", new String(path));
	}
	onQuit(application) {
		this.writePreferences();
		application.quit();
	}
	onRotate(application, delta) {
		let rotation = this.deviceRotation + delta;
		if (rotation < 0) rotation += 360;
		this.deviceRotation = rotation % 360;;
		this.quitScreen();
		this.selectDevice(application, this.deviceIndex);
		this.launchScreen();
	}
	onSelectDevice(application, index) {
		this.quitScreen();
		this.selectDevice(application, index);
		this.launchScreen();
	}
	
/* APP MENU */
	canAbout() {
		return true;
	}
	canQuit() {
		return true;
	}
	doAbout() {
		system.alert({ 
			type:"about",
			prompt:"mcsim",
			info:"Copyright 2018 Moddable Tech, Inc.\nAll rights reserved.\n\nThis application incorporates open source software from Marvell, Inc. and others.",
			buttons:["OK"]
		}, ok => {
		});
	}
	doQuit() {
		this.onQuit(application);
	}
/* FILE MENU */
	canOpenFile() {
		return true;
	}
	canCloseFile() {
		return this.SCREEN && this.SCREEN.running;
	}
	canReloadFile() {
		return this.SCREEN && this.SCREEN.running;
	}
	canLocateSimulators() {
		return true;
	}
	canReloadSimulators() {
		return true;
	}
	doCloseFile() {
		this.archivePath = "";
		this.libraryPath = "";
		this.quitScreen();
		application.updateMenus();
		application.distribute("onInfoChanged");
	}
	doOpenFile() {
		system.openFile({ prompt:"Open File", path:system.documentsDirectory }, path => { if (path) application.defer("doOpenFileCallback", new String(path)); });
	}
	doOpenFileCallback(application, path) {
		let extension = (system.platform == "win") ? ".dll" : ".so";
		if (path.endsWith(extension)) {
			let index = this.devices.findIndex(device => device.applicationFilter.test(path));
			if (index < 0) index = 0;
			this.quitScreen();
			if (index != this.deviceIndex)
				this.selectDevice(application, index);
			this.libraryPath = path;
			this.launchScreen();
		}
		if (path.endsWith(".xsa")) {
			this.quitScreen();
			this.archivePath = path;
			this.launchScreen();
		}
	}
	doReloadFile() {
		this.quitScreen();
		this.launchScreen();
	}
	doLocateSimulators() {
		system.openDirectory({ prompt:"Locate", path:system.documentsDirectory }, path => { if (path) application.defer("doLocateSimulatorsCallback", new String(path)); });
	}
	doLocateSimulatorsCallback(application, path) {
		this.devicesPath = path;
		this.archivePath = "";
		this.libraryPath = "";
		this.doReloadSimulators();
		application.updateMenus();
		application.distribute("onInfoChanged");
	}
	doReloadSimulators() {
		this.quitScreen();
		this.reloadDevices(application);
		this.launchScreen();
	}
/* VIEW MENU */
	canToggleControls(target, item) {
		let divider = this.VERTICAL_DIVIDER;
		item.state = divider.behavior.status ? 1 : 0;
		return true;
	}
	doToggleControls(target, item) {
		let divider = this.VERTICAL_DIVIDER;
		divider.behavior.toggle(divider);
		this.controlsStatus = divider.behavior.status;
		application.updateMenus();
	}
	canToggleInfo(target, item) {
		item.state = this.infoStatus ? 1 : 0;
		return true;
	}
	doToggleInfo(target, item) {
		if (this.infoStatus) {
			this.infoStatus = false;
			this.BODY.coordinates = { left:0, right:0, top:27, bottom:0 };
			this.FOOTER.coordinates = { left:0, right:0, height:0, bottom:0 };
		}
		else {
			this.infoStatus = true;
			this.BODY.coordinates = { left:0, right:0, top:27, bottom:27 };
			this.FOOTER.coordinates = { left:0, right:0, height:27, bottom:0 };
		}
		application.updateMenus();
	}
	canSelectRotation(target, item) {
		item.check = this.deviceRotation == item.value;
		return true;
	}
	doSelectRotation(target, value) {
		this.deviceRotation = value;
		this.quitScreen();
		this.selectDevice(application, this.deviceIndex);
		this.launchScreen();
	}
/* HELP MENU */
	canSupport() {
		return true;
	}
	doSupport() {
		system.launchURL("http://moddable.tech");
	}

/* PREFERENCES */
	readPreferences() {
		try {
			let string = system.readPreferenceString("main");
			if (string) {
				let preferences = JSON.parse(string);
				if ("controlsCurrent" in preferences)
					this.controlsCurrent = preferences.controlsCurrent;
				if ("controlsStatus" in preferences)
					this.controlsStatus = preferences.controlsStatus;
				if ("infoStatus" in preferences)
					this.infoStatus = preferences.infoStatus;
				if (("devicesPath" in preferences) && system.fileExists(preferences.devicesPath))
					this.devicesPath = preferences.devicesPath;
				if ("deviceIndex" in preferences)
					this.deviceIndex = preferences.deviceIndex;
				if ("deviceRotation" in preferences)
					this.deviceRotation = preferences.deviceRotation;
				if (("libraryPath" in preferences) && system.fileExists(preferences.libraryPath))
					this.libraryPath = preferences.libraryPath;
				if (("archivePath" in preferences) && system.fileExists(preferences.archivePath))
					this.archivePath = preferences.archivePath;
			}
		}
		catch(e) {
		}
	}
	writePreferences() {
		this.controlsCurrent = this.VERTICAL_DIVIDER.behavior.current;
		this.controlsStatus = this.VERTICAL_DIVIDER.behavior.status;
		try {
			let content;
			let preferences = {
				controlsCurrent: this.controlsCurrent,
				controlsStatus: this.controlsStatus,
				infoStatus: this.infoStatus,
				devicesPath: this.devicesPath,
				deviceIndex: this.deviceIndex,
				deviceRotation: this.deviceRotation,
				libraryPath: this.libraryPath,
				archivePath: this.archivePath,
			};
			let string = JSON.stringify(preferences, null, "\t");
			system.writePreferenceString("main", string);
		}
		catch(e) {
		}
	}
}

class HeaderBehavior extends Behavior {	
	onDeviceSelected(row, device) {
		const glyph = row.first;
		const label = glyph.next;
		label.string = device.title;
		if (model.devices.length > 0) {
			glyph.state = 1;
			label.state = 1;
		}
		else {
			glyph.state = 0;
			label.state = 0;
		}
	}
	onMenuSelected(row, index) {
		if (index >= 0)
			model.onSelectDevice(application, index);
	}
	onMouseEntered(row, x, y) {
		if (model.devices.length > 1)
			row.state = 1;
	}
	onMouseExited(row, x, y) {
		if (model.devices.length > 1)
			row.state = 0;
	}
	onTouchBegan(row) {
		if (model.devices.length > 1)
			row.state = 2;
	}
	onTouchEnded(row) {
		if (model.devices.length > 1) { 
			row.state = 1;
			let data = {
				button: row,
				items: model.devices.map((device, index) => ({ title: device.title, index })),
			};
			data.items.splice(model.deviceIndex, 1);
			application.add(new ControlsMenu(data));
		}
	}
};

const pixelFormatNames = [
	"16-bit RGB 565 Little Endian",
	"16-bit RGB 565 Big Endian",
	"8-bit Gray",
	"8-bit RGB 332",
	"4-bit Gray",
	"4-bit Color Look-up Table",
];

class FooterBehavior extends Behavior {	
	onInfoChanged(row) {
		let left = "";
		let right = "";
		if (model.libraryPath) {
			left += system.getPathName(system.getPathDirectory(model.libraryPath))
			if (model.archivePath) {
				left += " + " + system.getPathName(system.getPathDirectory(model.archivePath))
			}
			const screen = model.SCREEN;
			if (screen.width && screen.height) {
				right += screen.width + " x " + screen.height;
				right += " - " + pixelFormatNames[model.SCREEN.pixelFormat];
			}
		}
		row.first.string = left;
		row.last.string = right;
	}
};

class ControlsMenuBehavior extends Behavior {	
	onClose(layout, index) {
		let data = this.data;
		application.remove(application.last);
		data.button.delegate("onMenuSelected", index);
	}
	onCreate(layout, data) {
		this.data = data;
	}
	onFitVertically(layout, value) {
		let data = this.data;
		let button = data.button;
		let container = layout.first;
		let scroller = container.first;
		let size = scroller.first.measure();
		let y = button.y + button.height + 1
		let height = Math.min(size.height, application.height - y - 20);
		container.coordinates = { left:button.x, width:size.width + 20, top:y, height:height + 10 }
		scroller.coordinates = { left:10, width:size.width, top:0, height:height }
// 		scroller.first.content(model.deviceIndex).first.visible = true;
		return value;
	}
	onTouchEnded(layout, id, x, y, ticks) {
		var content = layout.first.first.first;
		if (!content.hit(x, y))
			this.onClose(layout, -1);
	}
};

class ControlsMenuItemBehavior extends ButtonBehavior {
	onTap(item) {
		item.bubble("onClose", this.data.index);
	}
}

var ControlsMenu = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: ControlsMenuBehavior,
	contents: [
		Container($, { skin:controlsMenuSkin, contents:[
			Scroller($, { clip:true, active:true, contents:[
				Column($, { left:0, right:0, top:0, 
					contents: $.items.map($$ => new ControlsMenuItem($$)),
				}),
			]}),
		]}),
	],
}));

var ControlsMenuItem = Row.template($ => ({
	left:0, right:0, height:30, skin:controlsMenuItemSkin, active:true,
	Behavior:ControlsMenuItemBehavior,
	contents: [
		Content($, { width:20, height:30, skin:dotSkin, visible:false }),
		Label($, {left:0, right:20, height:30, style:controlsMenuItemStyle, string:$.title }),
	]
}));

var MainContainer = Container.template($ => ({ 
	left:0, right:0, top:0, bottom:0, 
	contents: [
		Row($, { left:0, right:0, top:0, height:26, skin:paneHeaderSkin, active:true, Behavior:HeaderBehavior, contents: [
			Content($, { width:30, height:26, skin:controlsMenuGlyphSkin, }),
			Label($, { left:0, right:0, style:paneHeaderStyle, }),
			Content($, {
				width:30, skin:buttonsSkin, variant:3, active:true, visible:true, 
				Behavior: class extends ButtonBehavior {
					onTap(button) {
						button.bubble("onRotate", 90);
					}
				},
			}),
		]}), 
		Content($, { left:0, right:0, top:26, height:1, skin:paneBorderSkin, }),
		Layout($, { anchor:"BODY", left:0, right:0, top:27, bottom:$.infoStatus ? 27 : 0, Behavior:DividerLayoutBehavior, contents: [
			Container($, { left:0, width:0, top:0, bottom:0, contents: [
				ControlsPane($, { anchor:"CONTROLS" }),
			]}),
			Container($, { anchor:"DEVICE", width:0, right:0, top:0, bottom:0, skin:backgroundSkin, clip:true, contents:[
				Content($, {}),
			]}),
			VerticalDivider($, { 
				anchor:"VERTICAL_DIVIDER", left:$.controlsStatus ? $.controlsCurrent - 3 : -3, width:6,
				before:0, current:$.controlsCurrent, after:320, status:$.controlsStatus,
			}),
		]}),
		Container($, { anchor:"FOOTER", left:0, right:0, height:$.infoStatus ? 27 : 0, bottom:0, clip:true, contents:[
			Content($, { left:0, right:0, height:1, bottom:26, skin:paneBorderSkin, }),
			Row($, { left:0, right:0, height:26, bottom:0, skin:paneHeaderSkin, Behavior:FooterBehavior, contents: [
				Label($, { left:0, right:0, style:paneFooterLeftStyle, }),
				Label($, { left:0, right:0, style:paneFooterRightStyle, }),
			]}), 
		]}),
	]
}));

let mcsimApplication = Application.template($ => ({
	style:applicationStyle,
	Behavior: ApplicationBehavior,
	contents: [
	],
	menus: [
		{ 
			title:"mcsim",
			items: [
				{ title:"About mcsim", command:"About" },
				null,
				{ title:"Services", command:"Services" },
				null,
				{ title:"Hide mcsim", key:"H", command:"HideApplication" },
				{ title:"Hide Others", option:true, key:"H", command:"HideOtherApplications" },
				{ title:"Show All", command:"ShowAllApplications" },
				null,
				{ title:"Quit mcsim", key:"Q", command:"Quit" },
			],
		},
		{ 
			title:"File",
			items: [
				{ title:"Open Application...", key:"O", command:"OpenFile" },
				{ title:"Close Application", key:"W", command:"CloseFile" },
				{ title:"Reload Application", key:"R", command:"ReloadFile" },
				null,
				{ title:"Locate Simulators...", key:"L", command:"LocateSimulators" },
				{ title:"Reload Simulators", shift:true, key:"R", command:"ReloadSimulators" },
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
			],
		},
		{
			title:"View",
			items: [
				{ state:0, titles: ["Show Controls", "Hide Controls"], key:"K", command:"ToggleControls" },
				{ state:0, titles: ["Show Info", "Hide Info"], key:"I", command:"ToggleInfo" },
				null,
				{ title:"0°", value:0, command:"SelectRotation" },
				{ title:"90°", value:90, command:"SelectRotation" },
				{ title:"180°", value:180, command:"SelectRotation" },
				{ title:"270°", value:270, command:"SelectRotation" },
			],
		},
		{
			title:"Help",
			items: [
				{ title:"Moddable Developer", command:"Support" },
				null,
				{ title:"About mcsim", command:"About" },
			],
		},
	],
	window: {
		title:"mcsim",
	},
}));

export default new mcsimApplication(null, { 
	touchCount:1,
});

