import Retention from "Retention";
import Sound from "piu/Sound"
import Time from "time";
import Timer from "timer";

class Controller extends Behavior {
	constructor() {
		super();
		this.forgottenHistory = 0;
		this.going = 0;
		this.history = [];
		this.view = null;
		this.tapSound = new Sound({ path: "tap.wav" });
		this.tapBackSound = new Sound({ path: "tap-back.wav" });
		this.toggleSound = new Sound({ path: "toggle.wav" });
		
		this.inputs = new Uint8Array([0,0,0,0,0,0,0,0,0,1,1,2,3,4,7,10,14,19,25,32,41,51,61,71,80,88,95,99,100,99,95,88,80,71,61,51,41,32,25,19,14,10,7,4,3,1,0,0]);
		this.outputs = new Uint8Array([10,10,5,5,10,10,5,5,10,10,5,5,10,10,20,34,48,46,45,46,48,46,45,46,48,45,51,55,40,38,30,28,25,28,30,45,60,65,75,70,60,50,40,28,15,18,20,15]);
		this.levels = new Uint8Array([40,39,37,36,35,34,32,31,30,28,27,26,26,25,24,21,17,12,7,4,1,0,1,4,8,13,21,28,35,46,56,67,77,86,94,98,99,97,91,83,74,66,59,54,50,48,45,42]);
		this.signs = new Int8Array([-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1]);
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
	doPlayTap(back) {
		if (back)
			this.tapBackSound.play();
		else
			this.tapSound.play();
	}
	doPlayToggle() {
		this.toggleSound.play();
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
		
		const model = this.model;
		this.internal = Retention.readParams(model.internal, model.internalKey, model.internalFormat);
	}
	onDisplaying(container) {
		const data = this.model.home;
		const id = data.View;
		const View = importNow(id);
		const view = new View(data);
		view.id = id;
		this.history = [];
		this.display(container, view, false);
		
		this.index = 0;
		this.next = 1;
		this.fraction = 0;
		this.hours = 0;
		this.minutes = 0;
		this.tickCount = 0;
		Timer.repeat(() => this.onTick(container), 100);
	}
	onFinished(container) {
		container.distribute("onTimeout");
	}
	onMessage(container, message) {
		const json = JSON.parse(message);
		if ((json.touches == "startPlaying") || (json.touches == "startRecording")) {
			const model = this.model;
			const params = this.internal;
			if (Retention.filterParams(2, model.internal, params))
				Retention.writeParams(model.internalFormat, model.internalKey, params);
		}
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
	onTick(container) {
		this.tickCount++;
		let time = this.tickCount;
		this.minutes = time % 60;
		time = Math.floor(time / 30);
		this.index = time % 48;
		let next = this.index + 1;
		if (next == 48)
			next = 0;
		this.next = next;
		this.fraction = (this.minutes % 30) / 30;
		
		this.hours = this.index >> 1;
		
		this.updateScreen();
// 		trace(`${this.time} ${this.level}\n`);
	}
	onTimeout(container) {
		container.time = 0;
		container.start();
	}
	onTransitionEnded(container) {
		application.purge();
		this.going = 0;
		this.onScreenDisplayed(container, this.view);
		this.updateScreen();
	}
	redisplay() {
		this.onScreenUndisplaying(application, this.view);
		this.container.replace(this.container.first, new this.view.Template(this.view));
		this.onScreenDisplayed(application, this.view);
	}

	setTime(application, time) {
		Time.set(time);
		trace(`Set time to ${new Date()}\n`);
	}
	updateScreen() {
		if (this.going)
			return;
		let container = application.last;
		if (container)
			container.distribute("onUpdate");
	}
	
	setInternalParam(key, value) {
		let params = this.internal;
		if (params[key] != value) {
			let model = this.model;
			params[key] = value;
			Retention.writeParams(model.internalFormat, model.internalKey, params);
			this.updateScreen();
			return true;
		}
	}
	
	get alarm() {
		return this.internal.alarm;
	}
	set alarm(it) {
		this.setInternalParam("alarm", it);
	}
	get capacity() {
		return this.internal.capacity;
	}
	set capacity(it) {
		this.setInternalParam("capacity", it);
	}
	get currentLimit() {
		return this.internal.currentLimit;
	}
	set currentLimit(it) {
		this.setInternalParam("currentLimit", it);
	}
	get fullCharge() {
		return this.internal.fullCharge;
	}
	get fullChargeString() {
		return this.internal.fullCharge.toFixed(1);
	}
	set fullCharge(it) {
		this.setInternalParam("fullCharge", it);
	}
	set fullChargeString(it) {
		this.setInternalParam("fullCharge", parseFloat(it));
	}
	get input() {
		let from = this.inputs[this.index] / 3;
		let to = this.inputs[this.next] / 3;
		return Math.round(from + ((to - from) * this.fraction));
	}
	get level() {
		let from = this.levels[this.index];
		let to = this.levels[this.next];
		return Math.round(from + ((to - from) * this.fraction));
	}
	get output() {
		let from = this.outputs[this.index] / 2;
		let to = this.outputs[this.next] / 2;
		return Math.round(from + ((to - from) * this.fraction));
	}
	get sign() {
		return this.signs[this.next];
	}
	get temperatureUnit() {
		return this.internal.temperatureUnit;
	}
	set temperatureUnit(it) {
		this.setInternalParam("temperatureUnit", it);
	}
	get time() {
		let ampm = "am";
		let hours = this.hours;
		if (hours == 0)
			hours = 12;
		if (hours > 12) {
			hours -= 12;
		  ampm = "pm";
		}
		let minutes = this.minutes;
		if (minutes < 10)
			minutes = "0" + minutes.toString(10);
		return `${hours}:${minutes} ${ampm}`;
	}
	get type() {
		return this.internal.type;
	}
	set type(it) {
		this.setInternalParam("type", it);
	}
	get zeroCharge() {
		return this.internal.zeroCharge;
	}
	get zeroChargeString() {
		return this.internal.zeroCharge.toFixed(1);
	}
	set zeroCharge(it) {
		this.setInternalParam("zeroCharge", it);
	}
	set zeroChargeString(it) {
		this.setInternalParam("zeroCharge", parseFloat(it));
	}
}
Object.freeze(Controller.prototype);

export default Controller;
