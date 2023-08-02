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

const singleTransition = true;

class ViewBehavior extends Behavior {
	onCreate(container, view) {
		this.view = view;
	}
}

class View {
	static get Behavior() { return ViewBehavior }
	constructor() {
	}
	get historical() {
		return true;
	}
	deleteAnchors(data) {
		var prototype = Content.prototype;
		for (var key in data) {
			var property = data[key];
			if (prototype.isPrototypeOf(property)) {
				//trace("delete anchor " + key + "\n");
				delete data[key];
			}
			else if (Array.prototype.isPrototypeOf(property))
				property.forEach(this.deleteAnchors, this);
			else if (Object.prototype.isPrototypeOf(property))
				this.deleteAnchors(property);
		}
	}
	describe() {
	}
	onButtonHold(container) {
		if (controller.history.length)
			controller.goHome();
		return true;
	}
	onButtonPressed(container) {
		if (controller.history.length)
			controller.hold(1000);
		return true;
	}
	onButtonReleased(container) {
		if (controller.history.length)
			controller.goBack();
		return true;
	}
	purge() {
		this.deleteAnchors(this);
	}
	runTransitionBackwards(toView) {
		controller.container.run(null);
//		controller.container.replace(controller.container.first, new toView.Template(toView));
//		this.purge();
//		return;
		let FromTimeline = this.Timeline;
		let ToTimeline = toView.Timeline;
		if (FromTimeline && ToTimeline) {
			if (singleTransition)
				controller.container.run(new TimelineTransition(FromTimeline, ToTimeline), controller.container.first, toView.Template, this, toView);
			else {
				controller.container.run(new FromTimelineTransition(FromTimeline), controller.container.first, this, toView);
				controller.container.run(new ToTimelineTransition(ToTimeline), toView.Template, toView, this);
			}
		}
		else {
			let Transition = toView.Transition; 
			if (!Transition)
				Transition = this.Transition; 
			if (!Transition)
				Transition = DefaultTransition;
			controller.container.run(new Transition(true), controller.container.first, this, toView.Template, toView);
		}
	}
	runTransitionForwards(fromView) {
		controller.container.run(null);
//		controller.container.replace(controller.container.first, new this.Template(this));
//		fromView.purge();
//		return;
		let FromTimeline = fromView.Timeline;
		let ToTimeline = this.Timeline;
		if (FromTimeline && ToTimeline) {
			if (singleTransition)
				controller.container.run(new TimelineTransition(FromTimeline, ToTimeline), controller.container.first, this.Template, fromView, this);
			else {
				controller.container.run(new FromTimelineTransition(FromTimeline), controller.container.first, fromView, this);
				controller.container.run(new ToTimelineTransition(ToTimeline), this.Template, this, fromView);
			}
		}
		else {
			let Transition = this.Transition; 
			if (!Transition)
				Transition = fromView.Transition; 
			if (!Transition)
				Transition = DefaultTransition;
			controller.container.run(new Transition(false), controller.container.first, fromView, this.Template, this);
		}
	}
}
Object.freeze(View);

export default View;

class TimelineTransition extends Transition {
	constructor(FromTimeline, ToTimeline) {
		super(0);
		this.FromTimeline = FromTimeline;
		this.ToTimeline = ToTimeline;
	}
	onBegin(container, fromScreen, Template, fromView, toView) {
		this.fromScreen = fromScreen;
		const toScreen = this.toScreen = new Template(toView);
		container.add(toScreen);
		this.fromTimeline = new this.FromTimeline(fromScreen, fromView, toView, -1);
		this.toTimeline = new this.ToTimeline(toScreen, toView, fromView, 1);
		const fromDuration = this.fromTimeline.duration;
		const toDuration = this.toTimeline.duration;
		const duration = fromDuration + toDuration;
		this.duration = duration;
		this.fromFraction = fromDuration / duration;
		this.toFraction = toDuration / duration;
	}
	onEnd(container, fromScreen, Template, fromView, toView) {
		this.toTimeline = null;
		this.fromTimeline = null;
		container.remove(fromScreen);
		fromView.purge();
		application.purge();
	}
	onStep(fraction) {
		if (fraction < this.fromFraction) {
			this.fromScreen.visible = true
			this.fromTimeline.fraction = (this.fromFraction - fraction) / this.fromFraction;
			this.toScreen.visible = false
			this.toTimeline.fraction = 0;
		}
		else {
			this.fromScreen.visible = false
			this.fromTimeline.fraction = 0;
			this.toScreen.visible = true
			this.toTimeline.fraction = (fraction - this.fromFraction) / this.toFraction;
		}
	}
};
Object.freeze(TimelineTransition);

class FromTimelineTransition extends Transition {
	constructor(Timeline) {
		super(0);
		this.Timeline = Timeline;
	}
	onBegin(container, former, fromView, toView) {
		this.timeline = new this.Timeline(former, fromView, toView, -1);
		this.duration = this.timeline.duration;
	}
	onEnd(container, former, fromView) {
		this.timeline = null;
		container.remove(former);
		fromView.purge();
	}
	onStep(fraction) {
		this.timeline.fraction = 1 - fraction;
	}
};
Object.freeze(FromTimelineTransition);

class ToTimelineTransition extends Transition {
	constructor(Timeline) {
		super(0);
		this.Timeline = Timeline;
	}
	onBegin(container, Template, toView, fromView) {
		application.purge();
		let former = new Template(toView);
		container.add(former);
		this.timeline = new this.Timeline(former, toView, fromView, 1);
		this.duration = this.timeline.duration;
	}
	onEnd(container) {
		this.timeline = null;
	}
	onStep(fraction) {
		this.timeline.fraction = fraction;
	}
}
Object.freeze(ToTimelineTransition);

import DefaultTransition from "Transition";
