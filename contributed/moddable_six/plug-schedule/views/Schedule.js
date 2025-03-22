import {} from "piu/shape";
import {Outline} from "commodetto/outline";
import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";
import { VerticalExpandingKeyboard as Keyboard } from "keyboard";

export const halfPI = Math.PI / 2;
export const twoPI = 2 * Math.PI;
export const minuteSeconds = 60;
export const hoursSeconds = 60 * minuteSeconds;
export const daySeconds = 24 * hoursSeconds;

class ScheduleBehavior extends View.Behavior {
	onDialogYes(container) {
		const view = this.view;
		const plug = controller.plugs[view.which];
		plug.schedule = 0;
		plug.startTime = 0;
		plug.stopTime = 0;
		controller.savePlug(view.which);
		container.distribute("onChanged");
	}
}

class NameBehavior extends Behavior {
	onCreate(container, view) {
		this.view = view;
		this.onKeyboardOK(container, controller.names[view.which]);
		container.interval = 500;
	}
	onKeyUp(container, key) {
		const label = container.first;
		if ('\r' == key) {
			controller.doPlayTap();
			const screen = container.container;
			const view = this.view;
			container.stop();
			this.onKeyboardOK(container, this.string);
			controller.names[view.which] = this.string;
			controller.saveName(view.which);
			screen.run(new View.DialogTransition(200, -1), screen.last);
			return;
		}
		if ('\b' == key) {
			controller.doPlayTap(true);
			this.string = this.string.slice(0, -1);
		}
		else {
			controller.doPlayTap();
			this.string += key;
		}
		label.string = this.string;
	}
	onKeyboardOK(container, string) {
		const label = container.first;
		label.state = 0;
		if (string == "") {
			label.style = new Style(assets.styles.ITALIC);
			label.string = "Enter Name";
		}
		else
			label.string = string;
		this.string = string;
	}
	onTimeChanged(container) {
		const label = container.first;
		label.state = label.state ? 0 : 1;
	}
	onTouchEnded(container) {
		controller.doPlayTap();
		const view = this.view;
		const screen = container.container;
		const label = container.first;
		label.string = this.string;
		label.style = new Style(assets.styles.field);
		container.start();
		screen.run(new View.DialogTransition(200, 1), new KeyboardContainer(view));
	}
}

class ScheduleShapeBehavior extends Behavior {
	onChanged(shape) {
		const plug = controller.plugs[this.view.which];
		const x = 93;
		const y = 93;
		const weight = 10;
		const r = 76;
		const delta = Math.PI / 90;
		let path, angle, limit;
		
		angle = -halfPI + ((plug.startTime / daySeconds) * twoPI);
		limit = -halfPI + ((plug.stopTime / daySeconds) * twoPI);
		if (Math.abs(angle - limit) > delta) {
			path = new Outline.CanvasPath;
			path.arc(x, y, r, angle, limit);
			shape.strokeOutline = Outline.stroke(path, weight, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
		}
		else
			shape.strokeOutline = null;
	}
	onCreate(shape, view) {
		this.view = view;
	}
	onDisplaying(shape) {
		this.onChanged(shape);
	}
}

class ClockShapeBehavior extends Behavior {
	onChanged(shape) {
		this.onUpdate(shape);
	}
	onCreate(shape, view) {
		this.view = view;
	}
	onDisplaying(shape) {
		this.onUpdate(shape);
	}
	onUpdate(shape) {
		const { hours, minutes } = controller;
		const time = (hours * 3600) + (minutes * 60);
		const x = 93;
		const y = 93;
		const weight = 22;
		const r = 82;
		const path = new Outline.CanvasPath;
		const delta = Math.PI / 90;
		const angle = -halfPI + ((time / daySeconds) * twoPI);
		path.arc(x, y, r, angle, angle + delta);
		shape.fillOutline = Outline.stroke(path, weight, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
		
		const view = this.view;
		const plug = controller.plugs[view.which];
		view.POWER.state = controller.isTimeBetween(time, plug.startTime, plug.stopTime) ? 1 : 0;
	}
}

class StartStopBehavior extends Behavior {
	changeLabel(container, label, time) {
		time = Math.round(time / 60);
		let minutes = time % 60;
		time = Math.round(time / 60);
		let hours = time % 24;
		label.string = controller.timeToString(hours, minutes);
		label.container.visible = true;
	}
	changeThumb(container, thumb, time) {
		const angle = -halfPI + ((time / daySeconds) * twoPI);
		thumb.x = container.x + 93 - 9 + Math.round(Math.cos(angle) * 76);
		thumb.y = container.y + 93 - 9 + Math.round(Math.sin(angle) * 76);
		thumb.visible = true;
	}
	computeTime(container, time) {
		return 300 * Math.round(((time + daySeconds) % daySeconds) / 300);
	}
	hitTime(container, x, y) {
		const angle = Math.atan2(y - (container.y + 93), x - (container.x + 93));
		let fraction = (angle + halfPI) / twoPI;
		if (fraction < 0)
			fraction += 1;
		if (fraction > 1)
			fraction -= 1;
		return Math.round(fraction * daySeconds);
	}
	onChanged(container) {
		const view = this.view;
		const plug = controller.plugs[view.which];
		if (plug.schedule) {
			const startTime = plug.startTime;
			const stopTime = plug.stopTime;
			this.changeLabel(container, view.START_LABEL, startTime);
			this.changeThumb(container, view.START_THUMB, startTime);
			this.changeLabel(container, view.STOP_LABEL, stopTime);
			this.changeThumb(container, view.STOP_THUMB, stopTime);
			view.NO_SCHEDULE.visible = false;
			view.DELETE.visible = view.DELETE.active = true;
		}
		else {
			view.START_LABEL.container.visible = false;
			view.START_THUMB.visible = false;
			view.STOP_LABEL.container.visible = false;
			view.STOP_THUMB.visible = false;
			view.NO_SCHEDULE.visible = true;
			view.DELETE.visible = view.DELETE.active = false;
		}
	}
	onCreate(container, view) {
		this.view = view;
	}
	onDisplaying(container) {
		this.onChanged(container);
	}
	onTouchBegan(container, id, x, y, ticks) {
		const view = this.view;
		const plug = controller.plugs[view.which];
		const time = this.hitTime(container, x, y);
		let flags;
		if (plug.schedule) {
			let { startTime, stopTime } = plug;
			if (Math.abs(startTime - time) < 3600)
				flags = 1;
			else if (Math.abs(stopTime - time) < 3600)
				flags = 2;
			else if (controller.isTimeBetween(time, startTime, stopTime))
				flags = 3;
			else
				flags = 0;
		}
		else {
			plug.schedule = 1;
			plug.startTime = plug.stopTime = this.computeTime(container, time);
			time;
			flags = 2;
		}
		this.flags = flags;
		if (flags) {
			container.captureTouch(id, x, y, ticks);
			this.anchorTime = time;
			this.startTime = plug.startTime;
			this.stopTime = plug.stopTime;
			if (flags & 1)
				view.START_THUMB.state = 1;
			if (flags & 2)
				view.STOP_THUMB.state = 1;
			container.distribute("onChanged");
		}
	}
	onTouchMoved(container, id, x, y, ticks) {
		const flags = this.flags;
		if (flags) {
			const view = this.view;
			const plug = controller.plugs[view.which];
			const time = this.hitTime(container, x, y) - this.anchorTime;
			const startTime = (flags & 1) ? this.computeTime(container, this.startTime + time) : plug.startTime;
			const stopTime = (flags & 2) ? this.computeTime(container, this.stopTime + time) : plug.stopTime;
			if ((plug.startTime != startTime) || (plug.stopTime != stopTime)) {
				plug.startTime = startTime;
				plug.stopTime = stopTime;
				controller.doPlayTumbler();
			}
			container.distribute("onChanged");
		}
	}
	onTouchEnded(container) {
		const flags = this.flags;
		if (flags) {
			const view = this.view;
			if (flags & 1)
				view.START_THUMB.state = 0;
			if (flags & 2)
				view.STOP_THUMB.state = 0;
			controller.savePlug(view.which);
		}
	}
	onUpdate(container) {
		const view = this.view;
		const plug = controller.plugs[view.which];
		const startTime = plug.startTime;
		const stopTime = plug.stopTime;
		const time = controller.hours
	}
}

class DeleteBehavior extends View.ButtonBehavior {
	onTap(content) {
		super.onTap(content);
		content.state = 0;
		content.container.container.run(new View.DialogTransition(200, 1), new View.DialogContainer("Delete current schedule?"));
	}
}

const KeyboardContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, Behavior:View.DialogBehavior,
	contents: [
		Keyboard($, { top:140, height:180, bottom:undefined, style:new Style(assets.styles.screen), target:$.FIELD })
	],
}));

const ScheduleContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:ScheduleBehavior,
	contents: [
		Container($, { 
			left:0, width:240, top:100, bottom:0,
			contents: [
				Row($, { 
					left:10, top:0, height:16,
					contents: [
						Label($, { string:"On: " }),
						Label($, { anchor:"START_LABEL", style:assets.styles.BOLD }),
					],
				}),
				Row($, { 
					right:10, top:0, height:16,
					contents: [
						Label($, { string:"Off: " }),
						Label($, { anchor:"STOP_LABEL", style:assets.styles.BOLD }),
					],
				}),
				Label($, { anchor:"NO_SCHEDULE", top:0, height:16, string:"No schedule" }),
				Container($, { 
					height:186, bottom:14, skin:assets.skins.oval, active:true, Behavior:StartStopBehavior,
					contents: [
						Content($, { anchor:"POWER", skin:assets.skins.power }),
						Shape($, { left:0, right:0, top:0, bottom:0, skin:assets.skins.schedule, state:1, Behavior:ScheduleShapeBehavior } ),
						Content($, { anchor:"START_THUMB", left:0, width:18, top:0, height:18, skin:assets.skins.thumb }),
						Content($, { anchor:"STOP_THUMB", left:0, width:18, top:0, height:18, skin:assets.skins.thumb }),
						Shape($, { left:0, right:0, top:0, bottom:0, skin:assets.skins.schedule, Behavior:ClockShapeBehavior } ),
					],
				}),
				Content($, { anchor:"DELETE", right:0, bottom:0, skin:assets.skins.delete, active:false, visible:false, Behavior:DeleteBehavior }),
			],
		}),
		Content($, { left:0, right:0, top:-320, height:320, skin:assets.skins.screen, state:1 }),
		Content($, { left:0, top:0, skin:assets.skins.topArc1 }),
		Content($, { right:0, top:0, skin:assets.skins.lamp }),
		Content($, { left:0, width:50, top:0, height:50, skin:assets.skins.back, active:true, Behavior:View.BackBehavior  }),
		Label($, { top:5, style:assets.styles.title, Behavior:View.ClockBehavior }),
		Container($, {
			anchor:"FIELD", left:0, width:240, top:63, height:28, active:true, Behavior:NameBehavior,
			contents: [
				Label($, { height:20, skin:assets.skins.field, style:assets.styles.field }),
			],
		}),
	]
}));

class ScheduleTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		const body = screen.first;
		const curtain = body.next;
		const topArc1 = curtain.next;
		const lamp = topArc1.next;
		const back = lamp.next;
		const title = back.next;
		const name = title.next;
		
		if (other.id == "Home") {
			this.from(curtain, { y:0 }, 250, Math.quadEaseOut, 0);
			this.from(topArc1, { y:screen.height }, 250, Math.quadEaseOut, -250);
			this.from(back, { y:screen.height }, 250, Math.quadEaseOut, -250);
			this.from(title, { y:screen.height + 5 }, 250, Math.quadEaseOut, -250);
			this.from(name, { y:screen.height + 63 }, 250, Math.quadEaseOut, -250);
			this.from(lamp, { x:screen.width }, 250, Math.quadEaseOut, 0);
			
		}
	}
}

export default class extends View {

	constructor(data) {
		super(data);
		this.which = data;
	}
	get Template() { return ScheduleContainer }
	get Timeline() { return ScheduleTimeline }
};
