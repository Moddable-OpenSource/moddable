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

import WipeTransition from "piu/WipeTransition";

export class HorizontalTransition extends WipeTransition {
	constructor(backwards) {
		super(500, Math.quadEaseOut, backwards ? "left" : "right");
	}
	onBegin(application, container, fromView, Template, toView) {
		this.container = application;
		application.add(new Template(toView));
		this.die = new Die(null, {regionLength: 32});		//@@ how to size this?
		this.die.attach(application.last);
		this.width = this.die.width;
		this.height = this.die.height;
	}
	onEnd(application, container, fromView, Template, toView) {
		this.die.detach();
		application.remove(container);
		fromView.purge();
		delete this.die;
		delete this.container;
	}
}
Object.freeze(HorizontalTransition.prototype);

export class VerticalTransition extends WipeTransition {
	constructor(backwards) {
		super(500, Math.quadEaseOut, backwards ? "top" : "bottom");
	}
	onBegin(application, container, fromView, Template, toView) {
		this.container = application;
		application.add(new Template(toView));
		this.die = new Die(null, {regionLength: 32});		//@@ how to size this?
		this.die.attach(application.last);
		this.width = this.die.width;
		this.height = this.die.height;
	}
	onEnd(application, container, fromView, Template, toView) {
		this.die.detach();
		application.remove(container);
		fromView.purge();
		delete this.die;
		delete this.container;
	}
}
Object.freeze(VerticalTransition.prototype);

export default HorizontalTransition;