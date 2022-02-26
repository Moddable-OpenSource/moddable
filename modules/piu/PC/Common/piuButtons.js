/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

export class ButtonBehavior extends Behavior {
	changeState(container, state) {
		container.state = state;
		var content = container.first;
		while (content) {
			content.state = state;
			content = content.next;
		}
	}
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		this.onStateChanged(container);
	}
	onMouseEntered(container, x, y) {
		application.cursor = cursors.arrow;
		this.changeState(container, 2);
	}
	onMouseExited(container, x, y) {
		this.changeState(container, container.active ? 1 : 0);
	}
	onStateChanged(container) {
		this.changeState(container, container.active ? 1 : 0);
	}
	onTap(container) {
		let name = container.name;
		if (name)
			container.bubble(name, this.data);
		else
			debugger;
	}
	onTouchBegan(container, id, x, y, ticks) {
		this.changeState(container, 3);
		container.captureTouch(id, x, y, ticks);
	}
	onTouchEnded(container, id, x, y, ticks) {
		if (container.hit(x, y)) {
			this.changeState(container, 2);
			this.onTap(container);
		}
		else {
			this.changeState(container, 1);
		}
	}
	onTouchMoved(container, id, x, y, ticks) {
		this.changeState(container, container.hit(x, y) ? 3 : 2);
	}
};

// TEMPLATES

export var Button = Container.template(($, it) => ({
	width:80, height:30, Behavior:ButtonBehavior, active:true,
	contents: [
		RoundContent($, { left:5, right:5, top:5, height:20, border:1, radius:5, skin:skins.button, state:0 }),
		Label($, { left:0, right:0, height:30, style:styles.button, string:it.string }),
	],
}));

export var IconButton = Container.template(($, it) => ({
	Behavior:ButtonBehavior, active:true,
	contents: [
		RoundContent($, { left:2, right:2, top:2, bottom:2, radius:4, skin:skins.iconButton }),
		Content($, { skin:skins.icons, variant:it.variant }),
	],
}));
globalThis.IconButton = IconButton;

export var PopupButton = Container.template(($, it) => ({
	width:80, height:30, Behavior:ButtonBehavior, active:true,
	contents: [
		RoundContent($, { left:5, right:5, top:5, height:20, border:1, radius:5, skin:skins.button, state:0 }),
		Label($, { left:20, right:0, height:30, style:styles.popupButton, string:it.string }),
		Content($, { right:5, skin:skins.popupIcons }),
	],
}));
