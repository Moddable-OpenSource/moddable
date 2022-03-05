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

// BEHAVIORS

import { 
	ButtonBehavior, 
} from "behaviors";

export function findModeToPattern(findMode, findString) {
	let findPattern = findString;
	if (findPattern) {
		if (!(findMode & 8))
			findPattern = findPattern.replace(/(\\|\^|\$|\.|\+|\*|\?|\(|\)|\[|\]|\{|\}|\|)/g, "\\$1");
		if (findMode & 2)
			findPattern = "\\b" + findPattern;
		if (findMode & 4)
			findPattern = findPattern + "\\b";
	}
	return findPattern;
}

export function findModeToCaseless(findMode) {
	return (findMode & 1) == 0;
}

class FindModeBehavior extends ButtonBehavior {
	changeState(container, state) {
		let button = container.first;
		let icon = button.next;
		if (this.data.findMode & this.mask) {
			button.state = 3;
			icon.state = state;
		}
		else {
			button.state = state;
			icon.state = 3;
		}
	}
	onCreate(container, data, dictionary) {
		super.onCreate(container, data, dictionary);
		this.mask = dictionary.mask;
	}
	onFound(container) {
		this.changeState(container, 1);
	}
	onRestore(container) {
		this.changeState(container, 1);
	}
	onTap(container) {
		var data = this.data;
		data.findMode ^= this.mask;
		this.changeState(container, 2);
		container.bubble("onFindEdited");
	}
}

// TEMPLATES

export var FindModeButton = Container.template(($, it) => ({
	width:20, height:20, Behavior:FindModeBehavior, active:true,
	contents: [
		Content($, { skin:skins.findModeButton, variant:it.variant }),
		Content($, { skin:skins.findModeIcon, variant:it.variant }),
	],
}));

export var FindField = Row.template($ => ({
	left:0, right:0, top:4, bottom:4, skin:skins.fieldScroller,
	contents: [
		Field($, {
			anchor:"FIND_FOCUS",
			left:1, right:1, top:1, bottom:1,
			clip:true,
			active:true,
			skin:skins.field,
			style:styles.field,
			placeholder:$.findHint,
			Behavior: class extends Behavior {
				onCreate(field, data) {
					 this.data = data;
				}
				onStringChanged(field) {
					var data = this.data;
					data.findString = field.string;
					field.bubble("onFindEdited");
				}
				onSave(field) {
					var data = this.data;
// 					field.placeholder = "";
// 					field.string = "";
					data.FIND_FOCUS = null;
				}
				onRestore(field) {
					var data = this.data;
					field.placeholder = data.findHint;
					field.string = data.findString;
					data.FIND_FOCUS = field;
				}
			},
		}),
		Container($, {
			width:80, top:0, bottom:0,
			contents: [
				FindModeButton($, { right:62, variant:0, mask:1 }),
				FindModeButton($, { right:42, variant:1, mask:2 }),
				FindModeButton($, { right:22, variant:2, mask:4 }),
				FindModeButton($, { right:2, variant:3, mask:8 }),
			],
		}),
	],
}));
	

export var FindRow = Row.template(function($) { return {
	left:0, right:0, top:26, height:27, visible:false, clip:true,
	contents: [
		Row($, {
			left:0, right:0, top:-30, height:30, skin:skins.paneHeader, state:1, 
			contents: [
				Content($, { width:4 }),
				FindField($, {}),
				IconButton($, {
					width:40, variant:7, active:false,
					Behavior: class extends ButtonBehavior {
						onFound(button, resultCount) {
							button.active = resultCount > 0;
							this.changeState(button, button.active ? 1 : 0);
						}
						onTap(button) {
							button.bubble("doFindPrevious");
						}
					},
				}),
				IconButton($, {
					width:40, variant:8, active:false,
					Behavior: class extends ButtonBehavior {
						onFound(button, resultCount) {
							button.active = resultCount > 0;
							this.changeState(button, button.active ? 1 : 0);
						}
						onTap(button) {
							button.bubble("doFindNext");
						}
					},
				}),
				Container($, {
					width:60, active:true,
					Behavior: class extends ButtonBehavior {
						onTap(button) {
							button.bubble("onFindDone");
						}
					},
					contents: [
						RoundContent($, { left:3, right:3, top:4, bottom:4, radius:5, skin:skins.iconButton }),
						Label($, { left:0, right:0, height:30, style:styles.iconButton, string:"Done" }),
					],
				}),
			],
		}),
	],
}});

