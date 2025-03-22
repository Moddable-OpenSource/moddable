import assets from "assets";
import Timeline from "piu/Timeline";
import View from "View";
import { HorizontalScrollerBehavior, VerticalScrollerBehavior } from "ScrollerBehaviors";

class CommonBehavior extends Behavior {
	onBack(container) {
		controller.goBack();
	}
	onCreate(container, view) {
		this.view = view;
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

class CommonHorizontalScrollerBehavior extends HorizontalScrollerBehavior {
}

class CommonVerticalScrollerBehavior extends VerticalScrollerBehavior {
}


class CommonStateOfChargePortBehavior extends Behavior {
    onDraw(port) {
    	let { index, levels, signs } = controller;
    	const { width, height } = port;
    	const dx = 5;
        for (let x = width - dx; x > 0; x -= 10) {
        	const level = levels[index];
        	const sign = signs[index];
        	const color = level < 50 ? assets.ORANGE : sign > 0 ? assets.GREEN : assets.BLUE;
        	const dy = Math.round(level * height / 100);
            port.fillColor(color, x, height - dy, dx, dy);
         	index--;
            if (index < 0)
            	index = levels.length - 1;
        }
    }
	onUpdate(port) {
        port.invalidate();
	}
}

class TitleBarBehavior extends CommonButtonBehavior {
	changeState(container, state) {
		super.changeState(container, state);
// 		container.first.state = state;
	}
	onCreate(container, data) {
		this.data = data;
	}
	onTap(container) {
		if (controller.going)
			return;
		controller.doPlayTap(true);
		container.bubble("onBack");
	}
}

const TitleBarContainer = Container.template($ => ({
	left:0, width:240, top:0, height:50, skin:assets.skins.title,
	contents: [
		Content($, { left:0, top:0, skin:assets.skins.icons, variant:3, active:true, Behavior:TitleBarBehavior }),
		Label($, { left:0, right:0, top:4, height:42, style:assets.styles.L, string:$.data.title }),
	], 
}));


class CommonTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		const { delay, duration } = assets.transitions;
		const header = screen.first;
		const body = screen.last;
		this.from(body, { x:screen.x + screen.width }, delay + duration, Math.quadEaseIn, 0);
		this.from(header.last, { y:screen.y - header.height }, duration, Math.quadEaseIn, -duration);
		this.simultaneous = true;
	}
}

export default class extends View {
	static get Behavior() { return CommonBehavior; }
	static get ButtonBehavior() { return CommonButtonBehavior; }
	static get HorizontalScrollerBehavior() { return CommonHorizontalScrollerBehavior; }
	static get VerticalScrollerBehavior() { return CommonVerticalScrollerBehavior; }
	static get StateOfChargePortBehavior() { return CommonStateOfChargePortBehavior; }
	
	static get TitleBar() { return TitleBarContainer; }
	
	constructor(data = {}) {
		super();
		this.data = data;
	}
	get Timeline() { return CommonTimeline }
};
