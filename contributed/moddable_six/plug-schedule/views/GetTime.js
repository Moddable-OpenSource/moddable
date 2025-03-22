import {} from "piu/shape";
import {Outline} from "commodetto/outline";
import WiFi from "wifi";
import SNTP from "sntp";

import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

import { halfPI, twoPI, daySeconds } from "Schedule";

class GetTimeBehavior extends View.Behavior {
	onBack(container) {
		if (this.wifi) {
			this.wifi.close();
			this.wifi = null;
		}
		WiFi.disconnect();
		const view = this.view;
		if (view.data.authentication == "none")
			this.backCount--;
		const length = controller.history.length
		if (this.backCount > length)
			this.backCount = length;
		controller.goBack(this.backCount);
	}
	onCreate(container, view) {
		super.onCreate(container, view);
		this.backCount = 3;
		this.wifi = null;
		if (controller.history.length == 1)
			controller.wifi = {}; 
	}
	onDisplaying(container) {
		const host = "pool.ntp.org";
		const view = this.view;
		view.PROGRESS.string = `Joining ${ view.data.ssid }...`;
		WiFi.mode = WiFi.Mode.station;
		this.wifi = new WiFi(view.data, (msg, code) => {
		   switch (msg) {
		   case WiFi.gotIP:
				view.PROGRESS.string = `Getting time from ${host}...`;
				new SNTP({ host }, (message, value) => {
					if (SNTP.time === message) {
						controller.setTime(application, value);
						controller.wifi = view.data; 
						view.PROGRESS.string = `Got time from ${host}`;
						view.WAIT.visible = false;
						view.TIME.string = controller.time;
						if (controller.history.length == 1) {
							this.onBack(container);
							return;
						}
					}
					else if (SNTP.error === message) {
						this.onError(container, `Can't get time from ${host}`);
						this.backCount = 2;
					}
				});
				break;
			case WiFi.connected:
				view.PROGRESS.string = `Joined ${ view.data.ssid }`
				break;
			case WiFi.disconnected:
				if (-1 === code) {
					this.onError(container, `Password rejected`);
					this.backCount = 1;
				}
				else {
					this.onError(container, `Can't join ${ view.data.ssid }`);
					this.backCount = 2;
				}
				break;
			}
		});
	}
	onError(container, message) {
		const view = this.view;
		view.PROGRESS.string = message;
		view.WAIT.visible = false;
		view.ALERT.visible = true;
	}
}

class WaitContainerBehavior extends Behavior {
	onCreate(container, view) {
		this.timeline = new Timeline();
		this.angle0 = 0;
	}
	onDisplaying(container) {
		this.cx = container.x + (container.width >> 1);
		this.cy = container.y + (container.height >> 1);
		let content = container.first;
		let delay = 1000;
		while (content) {
			this.timeline.on(content.behavior, { angle:[- halfPI, twoPI - halfPI] }, 1500, Math.quadEaseInOut, delay); 
			content = content.next;
			delay = -1400;
		}
		container.duration = this.timeline.duration;
		container.time = 1000;
		container.start();
	}
	onFinished(container) {
		container.time = 0;
		container.start();
	}
	onTimeChanged(container) {
		this.timeline.seekTo(container.time);
		let content = container.first;
		while (content) {
			const behavior = content.behavior;
			const angle = behavior.angle;
			const radius = behavior.radius;
			content.x = this.cx + Math.round(radius * Math.cos(angle)) - behavior.dx;
			content.y = this.cy + Math.round(radius * Math.sin(angle)) - behavior.dy;
			content = content.next;
		}
	}
}

class WaitContentBehavior extends Behavior {
	onCreate(content) {
		this.angle = 0;
		this.radius = 0;
	}
	onDisplaying(content) {
		this.angle = 0;
		this.radius = ((content.container.height - content.height) >> 1) - content.coordinates.top;
		this.dx = content.width >> 1;
		this.dy = content.height >> 1;
	}
}

const GetTimeContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:GetTimeBehavior,
	contents: [
		Container($, {
			left:0, width:240, top:60, bottom:0,
			contents: [
				Container($, { 
					left:0, right:0, top:0, height:200,
					contents: [
						Container($, { 
							anchor:"WAIT", width:200, height:200, Behavior:WaitContainerBehavior,
							contents: [
								Content($, { left:80, top:0, skin:assets.skins.wait, variant:0, Behavior:WaitContentBehavior }),
								Content($, { left:80, top:-2, skin:assets.skins.wait, variant:1, Behavior:WaitContentBehavior }),
								Content($, { left:80, top:-4, skin:assets.skins.wait, variant:2, Behavior:WaitContentBehavior }),
								Content($, { left:80, top:-6, skin:assets.skins.wait, variant:3, Behavior:WaitContentBehavior }),
							]
						}),
						Label($, { anchor:"TIME", style:assets.styles.tumbler }),
						Content($, { anchor:"ALERT", skin:assets.skins.alert, visible:false }),
					]
				}),
				Text($, { anchor:"PROGRESS", style:assets.styles.ITALIC, left:20, right:20, bottom:20, }),
			]
		}),
		Container($, {
			left:0, right:0, top:-320, height:320, skin:assets.skins.screen, state:1,
			contents: [
				Content(-1, { right:0, bottom:0, skin:assets.skins.settings }),
			]
		}),
		Container($, {
			left:0, top:0, skin:assets.skins.topArc2,
			contents: [
				Content($, { left:0, width:50, top:0, height:50, skin:assets.skins.back, active:true, Behavior:View.BackBehavior }),
				Label($, { top:5, style:assets.styles.title, string:"Get Time" }),
			]
		}),
	]
}));

class GetTimeTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		const body = screen.first;
		const curtain = body.next;
		const header = curtain.next;
		
		if (other.id == "Home") {
			this.from(curtain, { y:0 }, 250, Math.quadEaseOut, 0);
			this.from(header, { y:screen.height }, 250, Math.quadEaseOut, -250);
		}
		else {
			this.from(header, { y:screen.y - header.height }, 250, Math.quadEaseOut, 0);
			if (controller.going != direction)
				this.from(body, { x:screen.x - body.width }, 250, Math.quadEaseOut, -125);
			else
				this.from(body, { x:screen.x + body.width }, 250, Math.quadEaseOut, -125);
		}
	}
}

export default class extends View {
	constructor(data) {
		super(data);
	}
	get Template() { return GetTimeContainer }
	get Timeline() { return GetTimeTimeline }
};
