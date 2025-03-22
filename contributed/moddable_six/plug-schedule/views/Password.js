import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";
import { VerticalExpandingKeyboard as Keyboard } from "keyboard";

class PasswordBehavior extends View.Behavior {
	onOK(container) {
		controller.goTo("GetTime", this.view.data);
	}
}

class FieldBehavior extends Behavior {
	onChanged(container, string) {
		const label = container.first;
		if (string == "") {
			label.style = new Style(assets.styles.ITALIC);
			label.string = "Enter Password";
			container.stop();
		}
		else {
			label.style = new Style(assets.styles.field);
			label.string = string;
			container.start();
		}
		this.string = string;
	}
	onCreate(container, view) {
		container.interval = 500;
		this.view = view;
		this.onChanged(container, view.data.password);
	}
	onKeyUp(container, key) {
		let string = this.string;
		if ('\r' == key) {
			controller.doPlayTap();
			container.stop();
			this.view.data.password = this.string;
			container.bubble("onOK");
			return;
		}
		if ('\b' == key) {
			controller.doPlayTap(true);
			string = string.slice(0, -1);
		}
		else {
			controller.doPlayTap();
			string += key;
		}
		this.onChanged(container, string);
	}
	onTimeChanged(container) {
		const label = container.first;
		label.state = label.state ? 0 : 1;
	}
}

const PasswordContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:PasswordBehavior,
	contents: [
		Container($, {
			left:0, top:0, skin:assets.skins.topArc2,
			contents: [
				Content($, { left:0, width:50, top:0, height:50, skin:assets.skins.back, active:true, Behavior:View.BackBehavior }),
				Label($, { top:5, style:assets.styles.title, string:"Login" }),
			]
		}),
		Container($, {
			left:0, width:240, top:58, bottom:0,
			contents: [
				Label($, { top:0, string:$.data.ssid }),
				Container($, {
					anchor:"FIELD", left:0, right:0, top:30, height:52, skin:assets.skins.topShadow, Behavior:FieldBehavior,
					contents: [
						Label($, { height:20, skin:assets.skins.field, style:assets.styles.field }),
					],
				}),
			]
		}),
		Keyboard($, { top:140, height:180, bottom:undefined, style:new Style(assets.styles.screen), target:$.FIELD })
	]
}));

class PasswordTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		let header = screen.first;
		let body = header.next;
		let keyboard = screen.last;
		this.from(header, { y:screen.y - header.height }, 250, Math.quadEaseOut, 0);
		this.from(keyboard, { y:screen.y + screen.height }, 250, Math.quadEaseOut, -250);
		if (controller.going != direction)
			this.from(body, { x:screen.x - body.width }, 250, Math.quadEaseOut, -125);
		else
			this.from(body, { x:screen.x + body.width }, 250, Math.quadEaseOut, -125);
	}
}

export default class extends View {
	constructor(data) {
		super(data);
	}
	get Template() { return PasswordContainer }
	get Timeline() { return PasswordTimeline }
};
