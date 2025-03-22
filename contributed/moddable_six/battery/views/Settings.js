import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

class SettingsBehavior extends View.Behavior {
}

class SettingBehavior extends Behavior {
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
		container.active = this.data.View ? true : false;
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
		controller.doPlayTap();
		this.view.itemData = this.data;
		controller.goWith(this.data);
	}
	onUndisplaying(container) {
		container.active = false;
	}
}

class ResetBehavior extends Behavior {
	onTouchBegan(container, id, x, y, ticks) {
		container.state = 1;
	}
	onTouchCancelled(container) {
		container.state = 0;
	}
	onTouchEnded(container) {
		controller.doPlayTap();
		container.state = 0;
	}
}

const SettingsContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, /*skin:assets.skins.screen,*/ style:assets.styles.screen, Behavior:SettingsBehavior,
	contents: [
		new $.constructor.TitleBar($),
		Scroller($, {
			left:0, width:240, top:50, bottom:0, clip:true, active:true, backgroundTouch:true, Behavior:View.VerticalScrollerBehavior,
			contents: [
				Column($, { 
					left:0, right:0, top:0, contents: $.data.items.map($$ => $$.id ? new SettingContainer($$, { view:$ }) : new ResetContainer($$, { view:$ }) ),
				}),
				VerticalScrollbar($, {}),
			]
		}),
	]
}));

const SettingContainer = Row.template($ => ({
	left:5, right:20, height:42, skin:assets.skins.setting, style:assets.styles.setting, Behavior:SettingBehavior,
	contents: [
		Label($, { left:0, right:0, style:assets.styles.LEFT, string:$.name }),
		Label($, { left:0, right:0, style:assets.styles.RIGHT, string:controller[$.id] + $.unit })
	]
}));

const ResetContainer = Row.template($ => ({
	left:5, right:20, height:42, skin:assets.skins.setting, style:assets.styles.setting,
	contents: [
		Label($, { left:0, right:0, style:assets.styles.LEFT, string:$.name }),
		Label($, { width:50, height:30, skin:assets.skins.reset, style:assets.styles.reset, string:"Reset", active:true, Behavior:ResetBehavior })
	]
}));

class VerticalScrollbarBehavior extends Behavior {
	onCreate(scrollbar) {
		this.former = 0;
	}
	onScrolled(scrollbar, scroller = scrollbar.container) {
		var thumb = scrollbar.first;
		var size = scroller.height;
		var range = scroller.first.height;
		if (size < range) {
			var height = scrollbar.height;
			thumb.y = scrollbar.y + Math.round(scroller.scroll.y * height / range);
			thumb.height = Math.round(height * size / range);
			scrollbar.visible = scrollbar.active = true;
		}
		else {
			thumb.height = 0;
			scrollbar.visible = scrollbar.active = false;
		}
	}
	onTouchBegan(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 3;
		scrollbar.captureTouch(id, x, y, ticks);
		let thumbY = thumb.y;
		let thumbHeight = thumb.height;
		if ((y < thumbY) || ((thumbY + thumbHeight) <= y))
			this.anchor = 0 - (thumbHeight >> 1);
		else
			this.anchor = thumbY - y;
		this.min = scrollbar.y;
		this.max = scrollbar.y + scrollbar.height - thumbHeight;
		this.onTouchMoved(scrollbar, id, x, y, ticks);
	}
	onTouchEnded(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 2;
	}
	onTouchMoved(scrollbar, id, x, y, ticks) {
		var scroller = scrollbar.container;
		var thumb = scrollbar.first;
		y += this.anchor;
		if (y < this.min)
			y = this.min;
		else if (y > this.max)
			y = this.max;
		thumb.y = y;

		scroller.scrollTo(scroller.scroll.x, (y - this.min) * (scroller.first.height / scrollbar.height));
	}
};

const VerticalScrollbar = Container.template($ => ({
	width:40, right:0, top:0, bottom:0, active:true, Behavior:VerticalScrollbarBehavior,
	contents: [
		Content($, { right:0, width:20, top:0, height:0, skin:assets.skins.scrollbarY }),
	],
}));

class SettingsTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		const { delay, duration } = assets.transitions;
		const header = screen.first;
		const body = screen.last;
		if (controller.going != direction) {
			this.from(body, { x:screen.x - screen.width }, delay + duration, Math.quadEaseOut, 0);
			this.from(header.last, { y:screen.y - header.height }, duration, Math.quadEaseOut, -duration);
		}
		else {
			duration == delay;
			this.from(header, { x:screen.x + screen.width }, delay + duration, Math.quadEaseIn, 0);
			this.from(body, { x:screen.x + screen.width }, delay + duration, Math.quadEaseIn, -(delay + duration));
		}
		this.simultaneous = true;
	}
}

export default class extends View {
	constructor(data) {
		super(data);
		this.itemData = null;
		this.scroll = { x:0, y:0 };
	}
	get Template() { return SettingsContainer }
	get Timeline() { return SettingsTimeline }
};
