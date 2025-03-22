import Retention from "Retention";
import Sound from "piu/Sound"
import Time from "time";
import Timer from "timer";
import WiFi from "wifi";

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
		this.tumblerSound = new Sound({ path: "tumbler.wav" });
		this.tumblerSound.last = 0;

		WiFi.mode = WiFi.Mode.station;
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
	doPlayTumbler() {
		const now = Time.ticks, tumblerSound = this.tumblerSound;
		if (now > (tumblerSound.last + 80)) {
			tumblerSound.play();
			tumblerSound.last = now;
		}
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
	goTo(id, data) {
		let View = importNow(id);
		if (View) {
			let view = new View(data);
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
	isTimeBetween(time, startTime, stopTime) {
		if (stopTime < startTime)
			return !((stopTime <= time) && (time < startTime));
		return ((startTime <= time) && (time < stopTime));
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
		this.internal.ssid = Retention.readPreference(model.ssidKey) || "";
		this.internal.password = Retention.readPreference(model.passwordKey) || "";
		this.internal.authentication = Retention.readPreference(model.authenticationKey) || "";
		this.plugs = [
			Retention.readParams(model.plug, model.plugKeys[0], model.plugFormat),
			Retention.readParams(model.plug, model.plugKeys[1], model.plugFormat),
		];
		this.names = [
			Retention.readPreference(model.plugNameKeys[0]) || "Floor Lamp",
			Retention.readPreference(model.plugNameKeys[1]) || "",
		];
		Time.dst = this.dst ? 3600 : 0;
		Time.timezone = (this.timezone - 11) * 3600;
	}
	onDisplaying(container) {
		const date = new Date();
		this.hours = date.getHours();
		this.minutes = date.getMinutes();

		let data = this.model.home;
		let id = data.View;
		let View = importNow(id);
		let view = new View(data);
		view.id = id;
		
		if (this.internal.ssid) {
			this.history = [ view ];
			data = this.wifi;
			id = "GetTime";
			View = importNow(id);
			view = new View(data);
		}
		this.display(container, view, false);
		
		this.onTick(container);
		Timer.repeat(() => this.onTick(container), 15000);
		
		this.updatePlug(0);
		this.updatePlug(1);
	}
	onFinished(container) {
		container.distribute("onTimeout");
	}
	onMessage(container, message) {
		const json = JSON.parse(message);
		if ((json.touches == "startPlaying") || (json.touches == "startRecording")) {
			const model = this.model;
			let params = this.internal;
			if (Retention.filterParams(2, model.internal, params))
				Retention.writeParams(model.internalFormat, model.internalKey, params);
			this.internal.ssid = "";
			this.internal.password = "";
			this.internal.authentication = "";
			for (let i = 0; i < 2; i++) {
				params = this.plugs[i];
				if (Retention.filterParams(2, model.plug, params))
					Retention.writeParams(model.plugFormat, model.plugKeys[i], params);
			}
			this.names[0] = "Floor Lamp";
			this.names[1] = "";
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
		const date = new Date();
		const hours = date.getHours();
		const minutes = date.getMinutes();
		if ((this.hours != hours) || (this.minutes != minutes)) {
			this.hours = hours;
			this.minutes = minutes;
			this.updateScreen();
			this.updatePlug(0);
			this.updatePlug(1);
		}
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
	saveName(which) {
		let model = this.model;
		Retention.writePreference(model.plugNameKeys[which], this.names[which]);
		this.updateScreen();
	}
	savePlug(which) {
		const model = this.model;
		Retention.writeParams(model.plugFormat, model.plugKeys[which], this.plugs[which]);
		this.updateScreen();
		this.updatePlug(which);
	}
	setDST(application, dst) {
		Time.dst = dst;
		this.onTick(application);
		trace(`Set dst to ${new Date()}\n`);
	}
	setTime(application, time) {
		Time.set(time);
		this.onTick(application);
		trace(`Set time to ${new Date()}\n`);
	}
	setTimezone(application, timezone) {
		Time.timezone = timezone;
		this.onTick(application);
		trace(`Set timezone to ${new Date()}\n`);
	}
	
	timeToString(hours, minutes) {
		let ampm = "am";
		if (hours == 0)
			hours = 12;
		else if (hours == 12) {
			ampm = "pm";
		}
		else if (hours > 12) {
			hours -= 12;
			ampm = "pm";
		}
		if (minutes < 10)
			minutes = "0" + minutes.toString(10);
		return `${hours}:${minutes} ${ampm}`;
	}
	updatePlug(which) {
		const plug = this.plugs[which];
		const on = this.isTimeBetween((this.hours * 3600) + (this.minutes * 60), plug.startTime, plug.stopTime);
		if (plug.on !== on) {
			plug.on = on;
			trace(`PLUG[${ which }] "${ this.names[which] }" ${ plug.on ? "ON" : "OFF" }\n`);
		}
	}
	updateScreen() {
		if (this.going)
			return;
		let container = application.last;
		if (container)
			container.distribute("onUpdate");
	}
	get dst() {
		return this.internal.dst;
	}
	set dst(it) {
		this.setDST(application, it ? 3600 : 0);
		this.internal.dst = it;
		const model = this.model;
		Retention.writeParams(model.internalFormat, model.internalKey, this.internal);
	}
	get network() {
		return this.wifi.ssid;
	}
	get time() {
		return this.timeToString(this.hours, this.minutes);
	}
	get timezone() {
		return this.internal.timezone;
	}
	set timezone(it) {
		this.setTimezone(application, (it - 11) * 3600);
		this.internal.timezone = it;
		const model = this.model;
		Retention.writeParams(model.internalFormat, model.internalKey, this.internal);
	}
	get timezoneName() {
		return this.model.timezones[this.internal.timezone];
	}
	get wifi() {
		const { ssid, password, authentication } = this.internal;
		return { ssid, password, authentication, current:true } ;
	}
	set wifi(it) {
		let model = this.model;
		const { ssid = "", password = "", authentication = "" } = it;
		this.internal.ssid = ssid;
		this.internal.password = password;
		this.internal.authentication = authentication;
		Retention.writePreference(model.ssidKey, ssid);
		Retention.writePreference(model.passwordKey, password);
		Retention.writePreference(model.authenticationKey, authentication);
	}
}
Object.freeze(Controller.prototype);

export default Controller;
