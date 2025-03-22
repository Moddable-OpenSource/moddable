import assets from "assets";
import Timeline from "piu/Timeline";
import View from "View";
import { HorizontalScrollerBehavior, VerticalScrollerBehavior } from "ScrollerBehaviors";

const NONE = 0;
const LEFT = -1;
const RIGHT = 1;
const TOP = -2;
const BOTTOM = 2;

class CommonBehavior extends Behavior {
	onBack(container) {
		controller.goBack();
	}
	onCreate(container, view) {
		this.view = view;
		container.active = true;
		container.backgroundTouch = true;
	}
	onSwipeBottom(container) {
	}
	onSwipeLeft(container) {
	}
	onSwipeRight(container) {
		if (controller.history.length > 0)
			this.onBack(container);
	}
	onSwipeTop(container) {
	}
	onTouchBegan(container, id, x, y, ticks) {
		x -= container.x;
		y -= container.y;
		this.x = x;
		this.y = y;
		this.ticks = ticks;
		if (x < 20)
			this.direction = RIGHT;
		else if (x >= (container.width - 20))
			this.direction = LEFT;
		else if (y < 20)
			this.direction = BOTTOM;
		else if (y >= (container.height - 20))
			this.direction = TOP;
		else
			this.direction = NONE;
	}
	onTouchEnded(container, id, x, y, ticks) {
		x -= container.x;
		y -= container.y;
		x -= this.x;
		y -= this.y
		ticks -= this.ticks;
		if (ticks < 500) {
			if (Math.abs(x) >= Math.abs(y)) {
				if ((x > 0) && (this.direction == RIGHT)) {
					this.onSwipeRight(container);
				}
				else if ((x < 0) && (this.direction == LEFT)) {
					this.onSwipeLeft(container);
				}
			}
			else {
				if ((y > 0) && (this.direction == BOTTOM)) {
					this.onSwipeBottom(container);
				}
				else if ((y < 0) && (this.direction == TOP)) {
					this.onSwipeTop(container);
				}
			}
		}
	}
}

class CommonButtonBehavior extends Behavior {
	changeState(container, state) {
		container.state = state;
	}
	onCreate(container, data) {
		this.data = data;
	}
	onTap(container) {
		controller.doPlayTap();
	}
	onTouchBegan(container, id, x, y, ticks) {
		container.captureTouch(id, x, y, ticks);
		this.changeState(container, 1);
	}
	onTouchMoved(container, id, x, y, ticks) {
		this.changeState(container, container.hit(x, y) ? 1 : 0);
	}
	onTouchEnded(container) {
		if (container.state == 1) {
			this.onTap(container);
		}
	}
}

class CommonItemBehavior extends Behavior {
	changeState(container, state) {
		container.state = container.first.state = container.last.state = state;
	}
	tweenState(container, from, to, duration) {
		this.from = from;
		this.to = to;
		container.duration = duration;
		container.time = 0;
		container.start();
	}
	onCreate(container, data, it) {
		this.data = data;
		this.view = it.view;
	}
	onDisplaying(container) {
		if (this.view.itemData == this.data) {
			this.view.itemData = null;
			this.changeState(container, 1);
		}
		else
			this.changeState(container, 0);
	}
	onDisplayed(container) {
		if (container.state == 1)
			this.tweenState(container, 1, 0, 250);
	}
	onTimeChanged(container) {
		this.changeState(container, this.from + (container.fraction * (this.to - this.from)));
	}
	onTouchBegan(container, id, x, y, ticks) {
		this.tweenState(container, 0, 1, 250);
	}
	onTouchCancelled(container) {
		container.stop();
		this.tweenState(container, container.fraction, 0, container.time);
	}
	onTouchEnded(container) {
		if (controller.going)
			return;
		controller.doPlayTap();
		this.view.itemData = this.data;
		container.bubble("onItemSelected", this.data);
	}
	onUndisplaying(container) {
	}
}

class CommonHorizontalScrollerBehavior extends HorizontalScrollerBehavior {
}

class CommonVerticalScrollerBehavior extends VerticalScrollerBehavior {
}

class CommonClockBehavior extends Behavior {
	onDisplaying(label) {
		this.onUpdate(label);
	}
	onUpdate(label) {
		label.string = controller.time;
	}
}

class CommonBackBehavior extends CommonButtonBehavior {
	onTap(content) {
		if (controller.going)
			return;
		controller.doPlayTap(true);
		content.bubble("onBack");
	}
}

class DialogBehavior extends Behavior {
	onClose(container) {
		container.container.run(new CommonDialogTransition(200, -1), container);
	}
	onDialogNo(container) {
		this.onClose(container);
	}
	onDialogYes(container) {
		this.onClose(container);
	}
}

class DialogNoButtonBehavior extends CommonButtonBehavior {
	onTap(container) {
		controller.doPlayTap(true);
		container.bubble("onDialogNo");
	}
}

class DialogYesButtonBehavior extends CommonButtonBehavior {
	onTap(container) {
		controller.doPlayTap();
		container.bubble("onDialogYes");
	}
}

const CommonDialogContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, Behavior:DialogBehavior,
	contents: [
		Container($, { 
			width:200, top:110, height:158, skin:assets.skins.dialog, style:assets.styles.dialog,
			contents: [
				Text($, { left:17, right:17, top:20, string:$ }),
				Content($, { left:17, right:17, bottom:75, height:2, skin:assets.skins.bar }),
				Label($, { left:17, width:74, height:38, bottom:21, skin:assets.skins.button, string:"No", active:true, Behavior:DialogNoButtonBehavior }),
				Label($, {  width:74, right:17, height:38, bottom:21, skin:assets.skins.button, string:"Yes", active:true, Behavior:DialogYesButtonBehavior }),
			],
		}),
	],
}));

class CommonDialogTransition extends Transition {
	constructor(duration, direction) {
		super(duration);
		this.direction = direction;
	}
	onBegin(container, content) {
		if (this.direction > 0)
			container.add(content);
		this.timeline = new Timeline();
		this.timeline.from(content.first, { y:container.y + container.height }, this.duration, Math.quadEaseOut, 0); 
	}
	onEnd(container, content, view) {
		this.timeline = null;
		if (this.direction < 0)
			container.remove(content);
	}
	onStep(fraction) {
		if (this.direction < 0)
			this.timeline.fraction = 1 - fraction;
		else
			this.timeline.fraction = fraction;
	}
};

class CommonTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		let header = screen.first;
		let body = screen.last;
		this.from(header, { y:screen.y - header.height }, 250, Math.quadEaseOut, 0);
		if (controller.going != direction)
			this.from(body, { x:screen.x - body.width }, 250, Math.quadEaseOut, -125);
		else
			this.from(body, { x:screen.x + body.width }, 250, Math.quadEaseOut, -125);
	}
}

export default class extends View {
	static get Behavior() { return CommonBehavior; }
	static get BackBehavior() { return CommonBackBehavior; }
	static get ButtonBehavior() { return CommonButtonBehavior; }
	static get ClockBehavior() { return CommonClockBehavior; }
	static get DialogContainer() { return CommonDialogContainer; }
	static get DialogTransition() { return CommonDialogTransition; }
	static get ItemBehavior() { return CommonItemBehavior; }
	static get HorizontalScrollerBehavior() { return CommonHorizontalScrollerBehavior; }
	static get VerticalScrollerBehavior() { return CommonVerticalScrollerBehavior; }
	
	constructor(data = {}) {
		super();
		this.data = data;
	}
	get Timeline() { return CommonTimeline }
};
