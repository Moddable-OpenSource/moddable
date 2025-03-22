import assets from "assets";
import Timeline from "piu/Timeline";
import { ToggleBehavior } from "ToggleBehavior";
import View from "Common";

class SetTimeBehavior extends View.Behavior {
	onBack(container) {
		const view = this.view;
		let time = view.hours * 3600;
		time += view.minutes * 60;
		if (!view.value)
			time += 12 * 3600;
		if (controller.dst)
			time -= 3600;
		time -= (controller.timezone - 11) * 3600;
		controller.setTime(application, time);	
		controller.goBack();
	}
}

class TumblerBehavior extends View.VerticalScrollerBehavior {
	onCreate(scroller, data) {
		super.onCreate(scroller, data);
		this.tracking = false;
	}
	onChanging(scroller, index) {
		controller.doPlayTumbler();
	}
	onChanged(scroller, index) {
		trace(index + "\n");
	}
	onDisplaying(scroller) {
		this.lineHeight = scroller.first.first.height;
		this.lineOffset = (scroller.height - this.lineHeight) >> 1;
		scroller.scrollTo(0, (this.index * this.lineHeight) - this.lineOffset);
	}
	onFinished() {
		controller.doPlayTumbler();
	}
	onScrolled(scroller) {
		if (scroller.tracking) {
			this.tracking = true;
			const index = Math.round((scroller.scroll.y + this.lineOffset) / this.lineHeight) % scroller.first.length;
			if (this.index != index) {
				this.index = index;
				this.onChanging(scroller, index);
			}
		}
		else {
			if (this.tracking) {
				this.onChanged(scroller, this.index);
			}
			this.tracking = false;
		}
	}
	snap(scroller, position, direction) {
		let result = (position + this.lineOffset) / this.lineHeight;
		result = (direction < 0) ? Math.ceil(result) :  Math.floor(result);
		result = (this.lineHeight * result) - this.lineOffset;
		return result;
	}
}

class HoursTumblerBehavior extends TumblerBehavior {
	onCreate(scroller, data) {
		super.onCreate(scroller, data);
		const column = scroller.first;
		let i = 0;
		column.add(new Label(data, { string:12 }));
		i++;
		while (i < 12) {
			column.add(new Label(data, { string:i }));
			i++;
		}	
		this.index = this.data.hours;
	}
	onChanged(scroller, index) {
		this.data.hours = index;
	}
}

class MinutesTumblerBehavior extends TumblerBehavior {
	onCreate(scroller, data) {
		super.onCreate(scroller, data);
		const column = scroller.first;
		let i = 0;
		while (i < 10) {
			column.add(new Label(data, { string:'0' + i }));
			i++;
		}	
		while (i < 60) {
			column.add(new Label(data, { string:i }));
			i++;
		}
		this.index = this.data.minutes;
	}
	onChanged(scroller, index) {
		this.data.minutes = index;
	}
}

class MeridiemToggleBehavior extends ToggleBehavior {
	onValueChanged(container) {
		controller.doPlayToggle();
	}
}

const SetTimeContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:SetTimeBehavior,
	contents: [
		Container($, {
			left:0, top:0, skin:assets.skins.topArc3,
			contents: [
				Content($, { left:0, width:50, top:0, height:50, skin:assets.skins.back, active:true, Behavior:View.BackBehavior }),
				Label($, { top:5, style:assets.styles.title, string:$.data.title }),
			]
		}),
		Container($, {
			left:0, width:240, top:100, bottom:0,
			contents: [
				Scroller($, {
					left:15, width:90, top:20, height:100, clip:true, active:true, backgroundTouch:true, looping:true, Behavior:HoursTumblerBehavior,
					contents: [
						Column($, { 
							left:0, right:0, top:0, style:assets.styles.tumbler,
						}),
						Content($, { left:0, right:0, top:0, bottom:0, skin:assets.skins.tumbler }),
					]
				}),
				Label($, { top:20, height:99, style:assets.styles.tumbler, string:":" }),
				Scroller($, {
					right:15, width:90, top:20, height:100, clip:true, active:true, backgroundTouch:true, looping:true, Behavior:MinutesTumblerBehavior,
					contents: [
						Column($, { 
							left:0, right:0, top:0, style:assets.styles.tumbler,
						}),
						Content($, { left:0, right:0, top:0, bottom:0, skin:assets.skins.tumbler }),
					]
				}),
				Container($, {
					width:80, top:140, height:40, style:assets.styles.L, active:true, Behavior:MeridiemToggleBehavior,
					contents: [
						Content($, { width:80, height:27, skin:assets.skins.meridiemBar }),
						Content($, { left:0, width:40, height:23, skin:assets.skins.meridiemButton }),
					]
				}),
			]
		}),
	]
}));

export default class extends View {
	constructor(data) {
		super(data);
		let { hours, minutes } = controller;
		let ampm = true;
		if (hours >= 12) {
			hours -= 12;
			ampm = false;
		}
		this.value = ampm;
		this.hours = hours;
		this.minutes = minutes;
	}
	get Template() { return SetTimeContainer }
};
