/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

import {} from "piu/PC";

import {
	applicationStyle,
	backgroundSkin,
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

import MessagesPane from "MessagesPane";

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
		global.model = this;
  		application.interval = 100;
		
		this.orientation = false;
		this.horizontalDividerCurrent = 320;
		this.horizontalDividerStatus = true;
		this.verticalDividerCurrent = 320;
		this.verticalDividerStatus = true;
		
		this.devicesPath = "";
		this.devices = [];
		this.deviceIndex = -1;

		this.screenPath = "";
		this.localScreenPath = system.buildPath(system.localDirectory, "mc.so");
		
		this.messagesKind = false;
		
		this.readPreferences();

		if (this.orientation)
			application.add(new VerticalContainer(this));
		else
			application.add(new HorizontalContainer(this));
					
	}
	onDisplaying(application) {
		if (this.devicesPath)
			this.reloadDevices(application);
		else
			this.selectDevice(application, -1);
		this.launchScreen();
	}

	launchScreen() {
		if (this.screenPath) {
			system.copyFile(this.screenPath, this.localScreenPath);
			this.SCREEN.launch(this.localScreenPath);
			this.DEVICE.first.defer("onConfigure");
		}
		application.updateMenus();
	}
	quitScreen() {
		if (this.SCREEN)
			this.SCREEN.quit();
		if (system.fileExists(this.localScreenPath))
			system.deleteFile(this.localScreenPath);
		application.updateMenus();
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
						let device = require.weak(info.path);
						if (device)
							devices.push(device);
					}
					catch (e) {
					}
				}
			}
			info = iterator.next();
		}
		devices.sort((a, b) => a.title.compare(b.title));
		
		let length = devices.length;
		if (index < 0)
			index = 0;
		else if (index >= length)
			index = length - 1;
		this.selectDevice(application, index);
	}
	selectDevice(application, index) {
		this.deviceIndex = index;
		let device = (index < 0) ? noDevice : this.devices[index];
		
		let screenContainer = new device.DeviceTemplate(device);
		let container = this.DEVICE;
		container.replace(container.first, screenContainer);
		
		let controlsColumn = new device.ControlsTemplate(device);
		let scroller = this.CONTROLS.first;
		scroller.replace(scroller.first, controlsColumn);
		
		application.distribute("onDeviceSelected", device);
		application.updateMenus();
	}
	
/* EVENTS */
	onOpenFile(application, path) {
		let info = system.getFileInfo(path);
		if (info.directory)
			this.doLocateSimulatorsCallback(path);
		else
			this.doOpenFileCallback(path);
	}
	onQuit(application) {
		this.writePreferences();
		application.quit();
	}
	onSelectDevice(application, index) {
		this.quitScreen();
		this.selectDevice(application, index);
		this.launchScreen();
	}
	onToggleMessages(application) {
		let deviceIndex = this.deviceIndex;
		this.horizontalDividerCurrent = this.HORIZONTAL_DIVIDER.behavior.current;
		this.horizontalDividerStatus = this.HORIZONTAL_DIVIDER.behavior.status;
		this.verticalDividerCurrent = this.VERTICAL_DIVIDER.behavior.current;
		this.verticalDividerStatus = this.VERTICAL_DIVIDER.behavior.status;
		this.quitScreen();
		this.selectDevice(application, -1),
		if (this.orientation) {
			application.replace(application.first, new HorizontalContainer(this));
			this.orientation = false;
		}
		else {
			application.replace(application.first, new VerticalContainer(this));
			this.orientation = true;
		}
		this.selectDevice(application, deviceIndex),
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
		this.quitScreen();
		this.screenPath = "";
	}
	doOpenFile() {
		system.openFile({ prompt:"Open File", path:system.documentsDirectory }, path => { if (path) this.doOpenFileCallback(path); });
	}
	doOpenFileCallback(path) {
		let extension = (system.platform == "win") ? ".dll" : ".so";
		if (path.endsWith(extension)) {
			this.quitScreen();
			this.screenPath = path;
			this.launchScreen();
		}
	}
	doReloadFile() {
		this.quitScreen();
		this.launchScreen();
	}
	doLocateSimulators() {
		system.openDirectory({ prompt:"Locate", path:system.documentsDirectory }, path => { if (path) this.doLocateSimulatorsCallback(path); });
	}
	doLocateSimulatorsCallback(path) {
		this.devicesPath = path;
		this.doReloadSimulators();
	}
	doReloadSimulators() {
		this.quitScreen();
		this.reloadDevices(application);
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
				if ("orientation" in preferences)
					this.orientation = preferences.orientation;
				if ("horizontalDividerCurrent" in preferences)
					this.horizontalDividerCurrent = preferences.horizontalDividerCurrent;
				if ("horizontalDividerStatus" in preferences)
					this.horizontalDividerStatus = preferences.horizontalDividerStatus;
				if ("verticalDividerCurrent" in preferences)
					this.verticalDividerCurrent = preferences.verticalDividerCurrent;
				if ("verticalDividerStatus" in preferences)
					this.verticalDividerStatus = preferences.verticalDividerStatus;
				if (("devicesPath" in preferences) && system.fileExists(preferences.devicesPath))
					this.devicesPath = preferences.devicesPath;
				if ("deviceIndex" in preferences)
					this.deviceIndex = preferences.deviceIndex;
				if (("screenPath" in preferences) && system.fileExists(preferences.screenPath))
					this.screenPath = preferences.screenPath;
				if ("messagesKind" in preferences)
					this.messagesKind = preferences.messagesKind;
			}
		}
		catch(e) {
		}
	}
	writePreferences() {
		try {
			let content;
			let preferences = {
				orientation: this.orientation,
				horizontalDividerCurrent: this.HORIZONTAL_DIVIDER.behavior.current,
				horizontalDividerStatus: this.HORIZONTAL_DIVIDER.behavior.status,
				verticalDividerCurrent: this.VERTICAL_DIVIDER.behavior.current,
				verticalDividerStatus: this.VERTICAL_DIVIDER.behavior.status,
				devicesPath: this.devicesPath,
				deviceIndex: this.deviceIndex,
				screenPath: this.screenPath,
				messagesKind: this.messagesKind,
			};
			let string = JSON.stringify(preferences, null, "\t");
			system.writePreferenceString("main", string);
		}
		catch(e) {
		}
	}
}

var VerticalContainer = Container.template($ => ({ left:0, right:0, top:0, bottom:0, contents: [
	Layout($, { left:0, right:0, top:0, bottom:0, Behavior:DividerLayoutBehavior, contents: [
		Layout($, { left:0, width:0, top:0, bottom:0, Behavior:DividerLayoutBehavior, contents: [
			Container($, { left:0, right:0, top:0, height:0, contents: [
				ControlsPane($, { anchor:"CONTROLS" }),
			]}),
			Container($, { left:0, right:0, height:0, bottom:0, contents: [
				MessagesPane($, { anchor:"MESSAGES" }),
			]}),
			HorizontalDivider($, { 
				anchor:"HORIZONTAL_DIVIDER", width:6, bottom:$.horizontalDividerStatus ? $.horizontalDividerCurrent - 3 : 23, 
				before:160, current:$.horizontalDividerCurrent, after:26, status:$.horizontalDividerStatus,
			}),
		]}),
		Container($, { anchor:"DEVICE",  width:0, right:0, top:0, bottom:0, skin:backgroundSkin, clip:true, contents:[
			Content($, {}),
		]}),
		VerticalDivider($, { 
			anchor:"VERTICAL_DIVIDER", left:$.verticalDividerStatus ? $.verticalDividerCurrent - 3 : 317, width:6,
			before:320, current:$.verticalDividerCurrent, after:320, status:$.verticalDividerStatus,
		}),
	]}),
]}));

var HorizontalContainer = Container.template($ => ({ left:0, right:0, top:0, bottom:0, contents: [
	Layout($, { left:0, right:0, top:0, bottom:0, Behavior:DividerLayoutBehavior, contents: [
		Layout($, { left:0, right:0, top:0, height:0, Behavior:DividerLayoutBehavior, contents: [
			Container($, { left:0, width:0, top:0, bottom:0, contents: [
				ControlsPane($, { anchor:"CONTROLS" }),
			]}),
			Container($, { width:0, right:0, top:0, bottom:0, contents: [
				MessagesPane($, { anchor:"MESSAGES" }),
			]}),
			VerticalDivider($, { 
				anchor:"VERTICAL_DIVIDER", left:$.verticalDividerStatus ? $.verticalDividerCurrent - 3 : 317, width:6,
				before:320, current:$.verticalDividerCurrent, after:320, status:$.verticalDividerStatus,
			}),
		]}),
		Container($, { anchor:"DEVICE",  left:0, right:0, height:0, bottom:0, skin:backgroundSkin, clip:true, contents:[
			Content($, {}),
		]}),
		HorizontalDivider($, { 
			anchor:"HORIZONTAL_DIVIDER", width:6, bottom:$.horizontalDividerStatus ? $.horizontalDividerCurrent - 3 : 23, 
			before:160, current:$.horizontalDividerCurrent, after:26, status:$.horizontalDividerStatus,
		}),
	]}),
]}));

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

