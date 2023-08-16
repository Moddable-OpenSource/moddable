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

import Time from "time";
import Timer from "timer";
import Power from "power";

const keys = Object.freeze({
	button: { pressed:"onButtonPressed", released:"onButtonReleased", hold:"onButtonHold" },
	jogDial: { pressed:"onJogDialPressed", released:"onJogDialReleased", hold:"onJogDialHold" }
}, true);

class Controller extends Behavior {
	constructor() {
		super();
		this.bluetoothName = "M4UART";
		this.forgottenHistory = 0;
		this.going = 0;
		this.history = [];
		this.view = null;
		
		this.delta = 0;
		this.accelerometer = new Host.Accelerometer({
			interrupt: {
				polarity: 0,
				enable: 0x2a,
				threshold: 0x06,
				duration: 0x02
			}
		});
		this.power = new Power({});
		//@@
		if ("irq_fired" in this.accelerometer)
			this.gravity = 9.80665;
		else
			this.gravity = 1
		new Host.Button({
			onPush: (value) => {
				if (value === 1)
					this.onPressed(application, keys.button);
				else
					this.onReleased(application, keys.button);
			}
		});
		new Host.JogDial({
			onTurn: (delta) => {
				this.onJogDialTurned(application, delta);
			},
			onPushAndTurn: (delta) => {
				this.onJogDialTurned(application, delta);
			},
			onPush: (value) => {
				if (value === 0)
					this.onPressed(application, keys.jogDial);
				else
					this.onReleased(application, keys.jogDial);
			}
		});
	}
	display(container, view, backwards) {
		if (this.view) {
			this.onScreenUndisplaying(container, this.view);
			if (backwards)
				this.view.runTransitionBackwards(view);
			else {
				if (this.view.historical)
					this.history.push(this.view);
				view.runTransitionForwards(this.view);
			}
		}
		else {
			this.container.add(new view.Template(view));
			this.going = 0;
			this.onScreenDisplayed(container, view);
		}
		this.view = view;
		if (this.forgottenHistory) {
			this.history.length = this.forgottenHistory;
			this.forgottenHistory = 0;
		}
		trace("HISTORY:");
		this.history.forEach(view => trace(" " + view.id));
		trace("\n");
	}
	forgetHistory(forgottenHistory = 1) {
		this.forgottenHistory = forgottenHistory;
	}
	goBack(count = 1) {
		if (this.history.length) {
			let view;
			while (count) {
				view = this.history.pop();
				count--;
			}
			this.going = -1;
			this.container.defer("display", view, true);
		}
	}
	goHome() {
		if (this.history.length) {
			let view = this.history[0];
			this.history.length = 0;
			this.going = -1;
			this.container.defer("display", view, true);
		}
	}
	goTo(id) {
		let View = importNow(id);
		if (View) {
			let view = new View();
			view.id = id;
			this.going = 1;
			this.container.defer("display", view, false);
		}
	}
	goWith(data) {
		let id = data.View;
		let View = importNow(id);
		if (View) {
			let view = new View(data);
			view.id = id;
			this.going = 1;
			this.container.defer("display", view, false);
		}
	}
	hold(duration) {
		this.keyWaitTimerID = Timer.set(() => {
			this.keyWaitTimerID = undefined;
			let key = this.key;
			this.key = undefined;
			if (key)
				this.onBubble(application, key.hold);
		}, duration);
	}
	onBubble(container, id, param) {
		if (container.first.delegate(id, param))
			return true;
		let view = this.view;
		let method = view[id];
		if (method && method.call(view, container, param))
			return true;
		let c = this.history.length;
		for (let i = this.history.length - 1; i >= 0; i--) {
			view = this.history[i];
			method = view[id];
			if (method && method.call(view, container, param))
				return true;
		}
	}
	onCreate(container, $) {
		this.container = container;
		this.model = $.model;
		container.duration = $.model.timeout;
		container.interval = 1000;
	}
	onDisplaying(container) {
		screen.dither = false;
		const power = this.power;
		let tag = power.getRetainedValue(0);
		const history = [];
		let view = null;
		const wakenWith = power.wakenWith;
		if (wakenWith == "reset")
			tag = 0;
		if (tag) {
			const Home = importNow("Home");
			view = new Home();
			view.id = "Home";
		
			if (power.getRetainedValue(1)) {
				history.push(view);
			
				const Menu = importNow("Menu");
				view = new Menu(this.model.menu);
				view.id = "Menu";
				view.index = power.getRetainedValue(2);
				
				if (power.getRetainedValue(3)) {
					history.push(view);
					
					const item = this.model.menu.items[view.index];
				
					const View = importNow(item.View);
					view = new View(item);
					view.id = item.View;
				
				}
			}
			if (tag == 999) {
				view.motionDetected = wakenWith == "accelerometer";
			}
			else {
				history.push(view);
			
				const Asleep = importNow("Asleep");
				view = new Asleep();
				view.id = "Asleep";
				view.wakenWith = wakenWith;
			}
			
			this.history = history;
			this.display(container, view, false);
		
			power.setRetainedValue(0, 0);
			power.setRetainedValue(1, 0);
			power.setRetainedValue(2, 0);
			power.setRetainedValue(3, 0);
			power.setRetainedValue(4, 0);
		}
		else {
			let Splash = importNow("Splash");
			let splash = new Splash();
			splash.id = "Splash";
			this.display(container, splash, false);
		}
	}
	onFinished(container) {
		container.distribute("onTimeout");
	}
	onPressed(container, key) {
		container.stop();
		if (this.going) {
			screen.context.onIdle();
			container.run(null);
		}
		if (this.key)
			return;
		this.pressing = true;
		this.key = key;
		if (this.onBubble(container, key.pressed))
			return;
	}
	onReleased(container, key) {
		container.time = 0;
		container.start();
		if (this.key != key)
			return;
		if (this.keyWaitTimerID !== undefined) {
			Timer.clear(this.keyWaitTimerID);
			this.keyWaitTimerID = undefined;
		}
		this.pressing = false;
		if (this.going)
			return;
		this.key = undefined;
		if (this.onBubble(container, key.released))
			return;
	}
	onJogDialTurned(container, delta) {
		container.stop();
		container.time = 0;
		container.start();
		delta += this.delta;
		this.delta = delta;
		if ((-4 < delta) && (delta < 4))
			return;
		if (this.going) {
			screen.context.onIdle();
			container.run(null);
		}
		if (this.onBubble(container, "onJogDialTurned", delta))
			this.delta = 0;
	}
	onScreenDisplayed(container, view) {
		container.time = 0;
		container.start();
		container.first.distribute("onDisplayed");
	}
	onScreenUndisplaying(container, view) {
		container.stop();
		container.first.distribute("onUndisplaying");
	}
	onTimeout(container) {
// 		controller.sleep(undefined, 666);
		controller.goTo("Asleep");
// 		container.time = 0;
// 		container.start();
	}
	onTransitionEnded(container) {
		application.purge();
		this.going = 0;
		this.onScreenDisplayed(container, this.view);
		this.updateScreen();
		let key = this.key;
		if (!this.pressing && key) {
			this.key = undefined;
			if (this.onBubble(container, key.released))
				return;
		}
	}
	redisplay() {
		this.onScreenUndisplaying(application, this.view);
		this.container.replace(this.container.first, new this.view.Template(this.view));
		this.onScreenDisplayed(application, this.view);
	}
	sampleAccelerometer() {
		const gravity = this.gravity;
		const result = this.accelerometer.sample();
		result.x *= gravity;
		result.y *= gravity;
		result.z *= gravity;
		return result;
	}
	sleep(params, tag) {
		const history = this.history;
		const power = this.power;
		switch (history.length) {
		case 0:
			power.setRetainedValue(0, tag);
			power.setRetainedValue(1, 0);
			power.setRetainedValue(2, 0);
			power.setRetainedValue(3, 0);
			power.setRetainedValue(4, 0);
			break;
		case 1:
			power.setRetainedValue(0, tag);
			power.setRetainedValue(1, 1);
			power.setRetainedValue(2, this.view.index);
			power.setRetainedValue(3, 0);
			power.setRetainedValue(4, 0);
			break;
		case 2:
			power.setRetainedValue(0, tag);
			power.setRetainedValue(1, 1);
			power.setRetainedValue(2, history[1].index);
			power.setRetainedValue(3, 1);
			power.setRetainedValue(4, 0);
			break;
		}
		application.stop();
		power.sleep(params);
	}
	
	setTime(application, time) {
		Time.set(time);
		trace(`Set time to ${new Date()}\n`);
	}
	updateScreen() {
		if (this.going)
			return;
		let container = this.container.last;
		if (container)
			container.distribute("onUpdate");
	}
}
Object.freeze(Controller.prototype);

export default Controller;
