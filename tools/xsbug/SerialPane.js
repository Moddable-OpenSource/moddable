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
} from "behaviors";

class SerialPaneBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
		this.onSerialChanged(container);
	}
	onSerialChanged(container) {
		const column = container.first.first;
		const serial = this.data.serial;
		column.empty(2);
		if (serial.mod.spaceAvailable >= 0) {
			column.add(new ModTable(serial.mod));
		}
	}
}

class SerialButtonBehavior extends ButtonBehavior {
	onCreate(container, data) {
		super.onCreate(container, data);
		this.can = "can" + container.name;
		this.do = "do" + container.name;
		this.onSerialChanged(container);
	}
	onSerialChanged(container) {
		container.active = model[this.can](application, container.first.next);
		this.changeState(container, container.active ? 1 : 0);
	}
	onTap(container) {
		application.defer(this.do);
	}
}

class SerialProgressBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
		this.onSerialProgressChanged(container);
	}
	onSerialProgressChanged(container) {
		var data = this.data;
		container.first.width = Math.round(container.width * data.progress);
	}
}

class SerialTableBehavior extends TableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		column.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			this.fill(column);
			column.add(new SerialFooter(data));
		}
		else {
			header.behavior.expand(header, false);
		}
	}
	onCreate(column, data) {
		this.data = data;
		this.onSerialChanged(column);
	}
	onSerialChanged(column) {
		this.expand(column, this.data.expanded);
	}
}

class DeviceTableBehavior extends SerialTableBehavior {
	fill(column) {
		var data = this.data;
		if (data.mcu)
			column.add(new SerialRow(data.mcu));
		if (data.macAddress)
			column.add(new SerialRow(data.macAddress));
		if (data.pixelFormat)
			column.add(new SerialRow(data.pixelFormat));
	}
	hold(column) {
		return DeviceHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
}

class AppTableBehavior extends SerialTableBehavior {
	fill(column) {
		var data = this.data;
		if (data.progress >= 0)
			column.add(new SerialProgressRow(data));
		else if (data.signature)
			column.add(new SerialRow(data.signature));
		if (data.xsVersion)
			column.add(new SerialRow(data.xsVersion));
	}
	hold(column) {
		return AppHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
}

class ModTableBehavior extends SerialTableBehavior {
	fill(column) {
		var data = this.data;
		if (data.progress >= 0)
			column.add(new SerialProgressRow(data));
		else if (data.signature)
			column.add(new SerialRow(data.signature));
		else
			column.add(new SerialInfoRow("no mod"));
		if (data.spaceAvailable)
			column.add(new SerialRow(data.spaceAvailable + " bytes available"));
	}
	hold(column) {
		return ModHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
}

// TEMPLATES

import {
	VerticalScrollbar,
} from "piu/Scrollbars";

export var SerialPane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:skins.paneBackground,
	Behavior: SerialPaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:1, bottom:0, clip:true, active:true, Behavior:ScrollerBehavior,
			contents: [
				Column($, {
					left:0, right:0, top:0, Behavior:HolderColumnBehavior,
					contents: [
						DeviceTable($.serial.device, {}),
						AppTable($.serial.app, {}),
					],
				}),
				Container($, {
					left:0, right:0, top:0, height:26, clip:true, Behavior:HolderContainerBehavior,
				}),
				VerticalScrollbar($, {}),
			],
		}),
	],
}));

var DeviceTable = Column.template(function($) { return {
	left:0, right:0, active:true, 
	Behavior: DeviceTableBehavior,
	contents: [
		DeviceHeader($, { name:"HEADER" }),
	],
}});

var DeviceHeader = Row.template(function($) { return {
	left:0, right:0, height:27, skin:skins.tableHeader, active:true,
	Behavior: HeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs, variant:1 }),
		Label($, { width:52, style:styles.tableHeader, string:"DEVICE" }),
		SerialButton({ name:"Connect" }, { name:"SerialConnect", }),
	],
}});

var AppTable = Column.template(function($) { return {
	left:0, right:0, active:true, 
	Behavior: AppTableBehavior,
	contents: [
		AppHeader($, { name:"HEADER" }),
	],
}});

var AppHeader = Row.template(function($) { return {
	left:0, right:0, height:27, skin:skins.tableHeader, active:true,
	Behavior: HeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs, variant:1 }),
		Label($, { width:52, style:styles.tableHeader, string:"APP" }),
		SerialButton({ name:"Install" }, { name:"SerialInstallApp" }),
		SerialButton({ name:"Restart" }, { name:"SerialRestart", }),
	],
}});

var ModTable = Column.template(function($) { return {
	left:0, right:0, active:true, 
	Behavior: ModTableBehavior,
	contents: [
		ModHeader($, { name:"HEADER" }),
	],
}});

var ModHeader = Row.template(function($) { return {
	left:0, right:0, height:27, skin:skins.tableHeader, active:true,
	Behavior: HeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs, variant:1 }),
		Label($, { width:52, style:styles.tableHeader, string:"MOD" }),
		SerialButton({ name:"Install" }, { name:"SerialInstallMod" }),
		SerialButton({ name:"Uninstall" }, { name:"SerialUninstallMod", }),
	],
}});

var SerialButton = Container.template($ => ({
	height:26, active:true, Behavior:SerialButtonBehavior,
	contents: [
		RoundContent($, { left:3, right:3, top:4, bottom:4, radius:5, skin:skins.iconButton }),
		Label($, { left:0, right:0, style:styles.serialButton, string:$.name }),
	],
}));

var SerialInfoRow = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow,
	contents: [
		Content($, { width:rowIndent }),
		Content($, { width:20 }),
		Label($, { left:0, right:0, style:styles.infoRow, string:$ }),
	],
}});

var SerialProgressRow = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow,
	contents: [
		Content($, { width:rowIndent }),
		Content($, { width:20 }),
		Container($, { 
			left:0, right:0, height:10, skin:skins.progressBar, Behavior:SerialProgressBehavior,
			contents:[
				Content($, { left:0, width:0, top:0, bottom:0, skin:skins.progressBar, state:1 }),
			],
		}),
		Content($, { width:20 }),
		Content($, { width:rowIndent }),
	],
}});

var SerialRow = Row.template(function($) { return {
	left:0, right:0, height:rowHeight, skin:skins.tableRow,
	contents: [
		Content($, { width:rowIndent }),
		Content($, { width:20 }),
		Label($, { left:0, right:0, style:styles.tableRow, string:$ }),
	],
}});

var SerialFooter = Row.template(function($) { return {
	left:0, right:0, height:3, skin:skins.tableFooter,
}});


