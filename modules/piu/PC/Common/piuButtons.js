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

export class PopupMenuBehavior extends Behavior {	
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
		let y = Math.max(button.y - ((size.height / data.items.length) * data.selection), 0);
		let height = Math.min(size.height, application.height - y - 20);
		container.coordinates = { left:button.x - 15, width:button.width + 30, top:y, height:height + 10 };
		scroller.coordinates = { left:10, width:button.width + 10, top:0, height:height };
		scroller.first.content(data.selection).first.visible = true;
		return value;
	}
	onTouchEnded(layout, id, x, y, ticks) {
		var content = layout.first.first.first;
		if (!content.hit(x, y))
			this.onClose(layout, -1);
	}
};

export class PopupMenuItemBehavior extends ButtonBehavior {
	onTap(item) {
		item.bubble("onClose", item.index);
	}
}

export class PopupButtonBehavior extends ButtonBehavior {
	onDisplaying(container) {
		let data = this.data;
		super.onDisplaying(container);
		this.selection = data.items.findIndex(item => item.value == data.value);
		container.first.next.string = data.items[this.selection].title;
	}
	onMenuSelected(container, index) {
		if ((index >= 0) && (this.selection != index)) {
			let data = this.data;
			let item = data.items[index];
			this.selection = index;
			data.value = item.value;
			container.first.next.string = item.title;
		}
	}
	onTap(container) {
		let data = this.data;
		let it = {
			button: container,
			items: data.items,
			selection: this.selection,
		};
		application.add(new PopupMenu(it));
	}
}

// TEMPLATES

export var Button = Container.template(($, it) => ({
	width:80, height:30, Behavior:ButtonBehavior, active:true,
	contents: [
		RoundContent($, { left:5, right:5, top:5, height:20, border:1, radius:10, skin:skins.button, state:0 }),
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

export var PopupMenu = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: PopupMenuBehavior,
	contents: [
		Container($, { skin:skins.popupMenuShadow, contents:[
			Scroller($, { clip:true, active:true, skin:skins.popupMenu, contents:[
				Column($, { left:0, right:0, top:0, 
					contents: $.items.map($$ => new PopupMenuItem($$)),
				}),
			]}),
		]}),
	],
}));

export var  PopupMenuItem = Row.template($ => ({
	left:0, right:0, height:30, skin:skins.popupMenuItem, active:true,
	Behavior:PopupMenuItemBehavior,
	contents: [
		Content($, { width:25, height:30, skin:skins.popupIcons, variant:1, visible:false }),
		Label($, { left:0, right:0, height:30, style:styles.popupMenuItem, string:$.title }),
	]
}));

export var PopupButton = Container.template($ => ({
	width:80, height:30, Behavior:PopupButtonBehavior, active:true,
	contents: [
		RoundContent($, { left:5, right:5, top:5, height:20, border:1, radius:5, skin:skins.popupButton, state:0 }),
		Label($, { left:20, right:0, height:30, style:styles.popupButton }),
		Content($, { right:5, skin:skins.popupIcons }),
	],
}));
