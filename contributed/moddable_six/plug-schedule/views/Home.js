import {} from "piu/shape";
import {Outline} from "commodetto/outline";
import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

import { halfPI, twoPI, daySeconds } from "Schedule";

class HomeBehavior extends View.Behavior {
	onTapPlug(container, which) {
		this.view.which = which;
		if (which < 0)
			controller.goWith(controller.model.settings);
		else
			controller.goTo("Schedule", which);
	}
}

class ArcShapeBehavior extends Behavior {
	onCreate(shape, data) {
		this.data = data;
		const path = new Outline.CanvasPath;
		path.arc(0, 0, 320, -halfPI, halfPI);
		this.outline = Outline.stroke(path, 2, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
		this.from = { x:-227, y:95 };
		this.to = { x:0, y:0 };
		this.y = 320 - Math.round(320 * Math.sin(Math.acos((120) / 320)));
		this.dy = 160 - this.y;
	}
	onDisplaying(shape) {
		this.onFinished(shape);
	}
	onFinished(shape) {
		const to = this.to;
		to.y = this.y + Math.round(Math.random() * this.dy);
		to.x = 240 - Math.round(320 * Math.cos(Math.asin((320 - to.y) / 320)));
		shape.time = 0;
		shape.start();
	}
	onTimeChanged(shape) {
		const fraction = Math.quadEaseOut(shape.fraction);
		const from = this.from;
		const to = this.to;
		const x = from.x + ((to.x - from.x) * fraction);
		const y = from.y + ((to.y - from.y) * fraction);
		shape.fillOutline = this.outline.clone().translate(x, y);
		if (fraction > 0.8)
			shape.state = (fraction - 0.8) * 5;
		else
			shape.state = 0;
	}
}

class PlugBehavior extends Behavior {
	onCreate(container, which) {
		this.which = which;
	}
	onTouchEnded(container) {
		if (controller.going)
			return;
		controller.doPlayTap();
		container.bubble("onTapPlug", this.which);
	}
}

class ScheduleShapeBehavior extends Behavior {
	onCreate(shape, which) {
		const plug = controller.plugs[which];
		const x = 70;
		const y = 70;
		const weight = 6;
		const r = 70 - 8;
		const delta = Math.PI / 90;
		let path, angle, limit;
		
		path = new Outline.CanvasPath;
		angle = 0;
		limit = twoPI;
		while (angle < limit) {
			path.arc(x, y, r, angle, angle + delta);
			angle += delta;
			angle += delta;
		}
		shape.fillOutline = Outline.stroke(path, weight, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);

		path = new Outline.CanvasPath;
		
		angle = -halfPI + ((plug.startTime / daySeconds) * twoPI);
		limit = -halfPI + ((plug.stopTime / daySeconds) * twoPI);
		if (limit < angle)
			limit += twoPI;
		
		while (angle < limit) {
			path.arc(x, y, r, angle, angle + delta);
			angle += delta;
			angle += delta;
		}
		shape.strokeOutline = Outline.stroke(path, weight, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
	}
}

class ClockShapeBehavior extends Behavior {
	onDisplaying(shape) {
		this.onUpdate(shape);
	}
	onUpdate(shape) {
		const { hours, minutes } = controller;
		const time = (hours * 3600) + (minutes * 60);
		const x = 70;
		const y = 70;
		const weight = 16;
		const r = 70 - 8;
		const path = new Outline.CanvasPath;
		const delta = Math.PI / 90;
		const angle = -halfPI + ((time / daySeconds) * twoPI);
		path.arc(x, y, r, angle, angle + delta);
		shape.fillOutline = Outline.stroke(path, weight, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
	}
}

const PlugContainer = Container.template($ => ({
	left:0, width:240, top:$ == 0 ? 32 : 172, height:140, clip:true,
	contents: [
		Container($, { 
			left:$ == 0 ? 90 : 10, width:140, top:0, bottom:0, active:true, Behavior:PlugBehavior,
			contents: [
				Content($, { skin:assets.skins.plug }),
				Container($, { 
					left:0, right:0, top:0, bottom:0,
					contents: [
						Shape($, { left:0, right:0, top:0, bottom:0, skin:assets.skins.schedule, Behavior:ScheduleShapeBehavior } ),
						Shape($, { left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, Behavior:ClockShapeBehavior } ),
					],
				}),
				$ == 0
				? Text($, { width:75, right:140, top:80, style:assets.styles.RIGHT, string:controller.names[0] })
				: Text($, { left:140, width:75, top:40, style:assets.styles.LEFT, string:controller.names[1] }),
			],
		}),
	], 
}));

const HomeContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, state:1, Behavior:HomeBehavior,
	contents: [
		Content($, { left:0, top:0, skin:assets.skins.leftArc1 }),
		Shape($, { left:0, width:240, top:0, height:320, skin:assets.skins.arc, duration:1500, Behavior:ArcShapeBehavior } ),
		Shape($, { left:0, width:240, top:0, height:320, skin:assets.skins.arc, duration:3000, Behavior:ArcShapeBehavior } ),
		Content($, { left:0, top:0, skin:assets.skins.smartTitle }),
		Content($, { left:30, top:0, skin:assets.skins.plugTitle }),
		Label($, { right:15, top:5, style:assets.styles.L, state:1, Behavior:View.ClockBehavior }),
		
		PlugContainer(0, {}),
		PlugContainer(1, {}),
		Content(-1, { right:0, bottom:0, skin:assets.skins.settings, active:true, Behavior:PlugBehavior }),
	]
}));

class HomeTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		
		const arc1 = screen.first;
		const arc2 = arc1.next;
		const arc3 = arc2.next;
		const smartTitle = arc3.next;
		const plugTitle = smartTitle.next;
		const clock = plugTitle.next;
		const plug0 = clock.next;
		const plug1 = plug0.next;
		const setting = plug1.next;
		
		if (view.which == 0) {
			this.from(plug0, { x:screen.x - plug0.width }, 250, Math.quadEaseOut, 0);
			this.from(plug1, { x:screen.x - plug1.width }, 250, Math.quadEaseOut, -125);
			this.from(setting, { y:screen.y + screen.height }, 250, Math.quadEaseOut, -125);
		}
		else if (view.which == 1) {
			this.from(plug1, { x:screen.x - plug1.width }, 250, Math.quadEaseOut, 0);
			this.from(plug0, { x:screen.x - plug0.width }, 250, Math.quadEaseOut, -125);
			this.from(setting, { y:screen.y + screen.height }, 250, Math.quadEaseOut, -125);
		}
		else {
			this.from(plug0, { x:screen.x - plug0.width }, 250, Math.quadEaseOut, -125);
			this.from(plug1, { x:screen.x - plug1.width }, 250, Math.quadEaseOut, -250);
		}
		this.from(clock, { y:screen.y - clock.height }, 250, Math.quadEaseOut, -250);
		this.from(plugTitle, { y:screen.y - plugTitle.height }, 250, Math.quadEaseOut, -250);
		this.from(arc3, { x:screen.x - arc2.width }, 250, Math.quadEaseOut, -125);
		this.from(arc2, { x:screen.x - arc2.width }, 250, Math.quadEaseOut, -125);
		this.from(arc1, { x:screen.x - arc1.width }, 250, Math.quadEaseOut, -125);
		this.from(smartTitle, { x:screen.x - smartTitle.width }, 250, Math.quadEaseOut, -125);
	}
}

export default class extends View {

	constructor(data) {
		super(data);
	}
	get Template() { return HomeContainer }
	get Timeline() { return HomeTimeline }
};
