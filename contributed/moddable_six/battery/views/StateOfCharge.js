import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

class StateOfChargeBehavior extends View.Behavior {
	get fade() {
		return 0;
	}
	set fade(it) {
		const view = this.view;
		let row = view.LEGEND_Y.first;
		while (row) {
			let content = row.first;
			while (content) {
				content.state = it;
				content = content.next;
			}
			row = row.next;	
		}
		let label = view.LEGEND_X.first;
		while (label) {
			label.state = it;
			label = label.next;	
		}
	}
}

const StateOfChargeContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, style:assets.styles.screen, Behavior:StateOfChargeBehavior,
	contents: [
		new $.constructor.TitleBar($, { state:1 }),
		Content($, { anchor:"SEPARATOR_1", left:10, right:10, height:1, top:49, skin:assets.skins.separator }),
		Column($, {
			anchor:"LEGEND_Y", left:0, right:0, top:57, style:assets.styles.charge,
			contents: $.legends.map($$ => new StateOfChargeRow($$)),
		}),
		Scroller($, {
			left:33, right:37, top:65, bottom:35, clip:true, active:true, backgroundTouch:true,
			Behavior:HorizontalScrollberBehavior,
			contents: [
				Container($, {
					right:0, width:(48 * 10) - 5, top:0, bottom:0, style:assets.styles.charge,
					contents: [
						Port($, { left:0, right:0, top:0, bottom:20, Behavior: View.StateOfChargePortBehavior }),
						Row($, {
							anchor:"LEGEND_X", right:0, height:20, bottom:0,
							contents: [
								$.horizontal.map($$ => new Label($, { width:20, style:assets.styles.RIGHT, string:$$})),
								Label($, { width:40, style:assets.styles.RIGHT, string:"now" }),
							]
						}),
					]
				}),
			]
		}),
		HorizontalScrollbar($, {}),
	]
}));

const StateOfChargeRow = Row.template($ => ({
	left:0, right:0, height:50,
	contents: [
		Label($, { width:30, height:50, style:assets.styles.RIGHT, string:$.left }),
		Content($, { left:3, right:3, top:7, height:1, skin:assets.skins.separator, state:1 }),
		Label($, { width:34, height:50, style:assets.styles.LEFT, string:$.right }),
	]
}));

class HorizontalScrollberBehavior extends View.HorizontalScrollerBehavior {
	onScrolled(scroller) {
		const scrollbar = scroller.next;
		scrollbar.behavior.onScrolled(scrollbar, scroller);
	}
}

class HorizontalScrollbarBehavior extends Behavior {
	onCreate(scrollbar) {
		this.former = 0;
	}
	onScrolled(scrollbar, scroller = scrollbar.container) {
		var thumb = scrollbar.first;
		var size = scroller.width;
		var range = scroller.first.width;
		if (size < range) {
			var width = scrollbar.width;
			thumb.width = Math.round(width * size / range);
			thumb.x = scrollbar.x + width - thumb.width + Math.round(scroller.scroll.x * width / range);
			scrollbar.visible = scrollbar.active = true;
		}
		else {
			thumb.width = 0;
			scrollbar.visible = scrollbar.active = false;
		}
	}
	onTouchBegan(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 2;
		scrollbar.captureTouch(id, x, y, ticks);
		let thumbX = thumb.x;
		let thumbWidth = thumb.width;
		if ((x < thumbX) || ((thumbX + thumbWidth) <= x))
			this.anchor = 0 - (thumbWidth >> 1);
		else
			this.anchor = thumbX - x;
		this.min = scrollbar.x;
		this.max = scrollbar.x + scrollbar.width - thumbWidth;
		this.onTouchMoved(scrollbar, id, x, y, ticks);
	}
	onTouchEnded(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 1;
	}
	onTouchMoved(scrollbar, id, x, y, ticks) {
		var scroller = scrollbar.previous;
		var thumb = scrollbar.first;
		x += this.anchor;
		if (x < this.min)
			x = this.min;
		else if (x > this.max)
			x = this.max;
		thumb.x = x;
		scroller.scrollTo(Math.round((thumb.x - scrollbar.x - scrollbar.width + thumb.width) * scroller.first.width / scrollbar.width), 0);
	}
};

const HorizontalScrollbar = Container.template($ => ({
	left:10, right:10, height:16, bottom:10, clip:true, skin:assets.skins.scrollbarX, active:true, Behavior:HorizontalScrollbarBehavior,
	contents: [
		Content($, { left:0, width:0, top:0, bottom:0, skin:assets.skins.scrollbarX, state:1, variant:1 }),
	],
}));

class StateOfChargeTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		const { delay, duration } = assets.transitions;
		const header = screen.first;
		const footer = screen.last;
		this.from(header, { x:screen.x + screen.width }, duration, Math.quadEaseOut, 0);
		this.from(footer, { y:screen.y + screen.height }, duration, Math.quadEaseOut, -duration);
		this.from(screen.behavior, { fade:1 }, delay + duration, Math.quadEaseOut, -duration);
	}
}

export default class extends View {
	constructor(data) {
		super(data);
		const from = controller.zeroCharge;
		const to = controller.fullCharge;
		
		const legends = [];
		for (let value = 100; value >= 0; value -= 25)
			legends.push({ left: value + "%", right:(from + ((to - from) * value / 100)).toFixed(1) + "V" });
		this.legends = legends;
		
		const horizontal = [];
		for (let value = 23; value > 1; value--)
			horizontal.push(value.toString());
		this.horizontal = horizontal;
	}
	get Template() { return StateOfChargeContainer }
	get Timeline() { return StateOfChargeTimeline }
};
