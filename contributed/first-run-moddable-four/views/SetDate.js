/*
 * Copyright (c) 2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import assets from "assets";
import Timeline from "piu/Timeline";
import View from "View";
import Menu from "Menu";

const days = Object.freeze([
	"SUN","MON","TUE","WED","THU","FRI","SAT"
]);

const months = Object.freeze([
	"JAN",
	"FEB",
	"MAR",
	"APR",
	"MAY",
	"JUN",
	"JUL",
	"AUG",
	"SEP",
	"OCT",
	"NOV",
	"DEC",
]);

class SetDateBehavior extends View.Behavior {
	adjustDay(view) {
		const days = (new Date(view.year, view.month + 1, 0)).getDate();
		if (this.days != days) {
			this.days = days;
			if (view.day > days) {
				view.day = days;
				view.DAY.first.string = days;
			}
		}
	}
	changeFocus(view, anchor) {
		let former = view[this.focus];
		let current = view[anchor];
		former.state = 0;
		former.first.state = 0;
		former.last.state = 0;
		current.state = 1;
		current.first.state = 1;
		current.last.state = 1;
		view.UP.y = current.y + 2;
		view.DOWN.y = current.y + current.height - 2 - view.DOWN.height;
		this.focus = anchor;
	}
	
	onCreate(container, view) {
		super.onCreate(container, view);
		container.duration = 250;
		this.days = (new Date(view.year, view.month + 1, 0)).getDate();
		this.focus = "YEAR";
	}
	onDisplaying(container) {
		const view = this.view;
		this.changeFocus(view, "YEAR");
	}
	onFinished(container) {
		const focus = this.view[this.focus];
		focus.swap(focus.first, focus.last);
		controller.onJogDialTurned(application, 0);
	}
	onJogDialReleased(container) {
		const view = this.view;
		switch (this.focus) {
		case "YEAR":
			this.changeFocus(view, "MONTH");
			break;
		case "MONTH":
			this.changeFocus(view, "DAY");
			break;
		case "DAY":
			this.changeFocus(view, "YEAR");
			break;
		}
		return true;
	}
	onJogDialTurned(container, delta) {
		if (container.running)
			return false;
		const view = this.view;
		delta >>= 2;
		switch (this.focus) {
		case "YEAR":
			view.year += delta;
			view.YEAR.last.string = view.year;
			this.adjustDay(view);
			break;
		case "MONTH":
			let month = view.month + delta;
			month %= 12;
			if (month < 0)
				month += 12
			view.month = month;
			view.MONTH.last.string = months[view.month];
			this.adjustDay(view);
			break;
		case "DAY":
			let day = view.day - 1 + delta;
			const days = this.days;
			day %= days;
			if (day < 0)
				day += days
			day++;
			view.day = day;
			view.DAY.last.string = day;
			break;
		}
		this.delta = delta;
		container.time = 0;
		container.start();
		return true;
	}
	onTimeChanged(container) {
		const fraction = container.fraction;
		const focus = this.view[this.focus];
		const current = focus.last;
		const former = focus.first;
		if (this.delta > 0) {
			former.y = focus.y - (focus.height * fraction);
			current.y = focus.y + (focus.height * (1 - fraction));
		}
		else {
			former.y = focus.y + (focus.height * fraction);
			current.y = focus.y - (focus.height * (1 - fraction));
		}
	}
	onUndisplaying(container) {
		const view = this.view;
		const date = new Date();
		date.setFullYear(view.year);
		date.setMonth(view.month);
		date.setDate(view.day);
		application.delegate("setTime", Math.floor(date.valueOf() / 1000));
	}
}

const SetDateContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:SetDateBehavior,
	contents: [
		Container($, { 
			anchor:"YEAR", left:0, width:128, top:0, height:42, skin:assets.skins.focus, style:assets.styles.date, clip:true,
			contents: [
				Label($, { left:0, right:0, top:0, height:42, string:$.year }),
				Label($, { left:0, right:0, top:42, height:42 }),
			],
		}),
		Container($, { 
			anchor:"MONTH", left:0, width:128, top:42, height:44, skin:assets.skins.focus, style:assets.styles.date, clip:true,
			contents: [
				Label($, { left:0, right:0, top:0, height:44, string:months[$.month] }),
				Label($, { left:0, right:0, top:44, height:44 }),
			],
		}),
		Container($, { 
			anchor:"DAY", left:0, width:128, top:86, height:42, skin:assets.skins.focus, style:assets.styles.date, clip:true,
			contents: [
				Label($, { left:0, right:0, top:0, height:42, string:$.day }),
				Label($, { left:0, right:0, top:42, height:42 }),
			],
		}),
		Content($, { anchor:"UP", top: 2, left: 2, skin:assets.skins.arrows, state:1 }),
		Content($, { anchor:"DOWN", bottom: 2, left: 2, skin:assets.skins.arrows, state:1, variant:1 }),
	]
}));

class SetDateTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super();
		const year = screen.first;
		const month = year.next;
		const day = month.next;
		if (controller.going != direction) {
			this.from(year, { x:screen.x - year.width }, 200, Math.quadEaseOut, 0);
			this.from(month, { x:screen.x - month.width }, 200, Math.quadEaseOut, -100);
			this.from(day, { x:screen.x - day.width }, 200, Math.quadEaseOut, -100);
		}
		else {
			this.from(year, { x:screen.x + screen.width }, 200, Math.quadEaseOut, 0);
			this.from(month, { x:screen.x + screen.width }, 200, Math.quadEaseOut, -100);
			this.from(day, { x:screen.x + screen.width }, 200, Math.quadEaseOut, -100);
		}
	}
}

export default class extends View {
	static get Behavior() { return SetDateBehavior }
	
	constructor(data) {
		super(data);
		const date = new Date();
		this.year = date.getFullYear();
		this.month = date.getMonth();
		this.day = date.getDate();
	}
	get Template() { return SetDateContainer }
	get Timeline() { return SetDateTimeline }
};

class SetDateMenuItemBehavior {
	onDisplaying(container) {
		container.interval = 30000;
		container.start();
		this.onTimeChanged(container);
	}
	onTimeChanged(container) {
		const date = new Date();
		const label = container.first.first;
		label.string = days[date.getDay()]
		label.next.string = date.getDate();
	}
}

const SetDateMenuItemContainer = Container.template($ => ({
	left:0, width:128, top:0, height:128, clip:true, Behavior:SetDateMenuItemBehavior,
	contents: [
		Container($, {
			top:12, skin:assets.skins.date,
			contents: [
				Label($, { left:0, right:0, top:0, height:24, style:assets.styles.day, state:1 }),
				Label($, { left:0, right:0, top:24, height:40, style:assets.styles.date }),
			]
		}),
		Label($, { left:0, right:0, height:24, bottom:12, string:$.label }),
	]
}));

Menu.SetDate = SetDateMenuItemContainer;
