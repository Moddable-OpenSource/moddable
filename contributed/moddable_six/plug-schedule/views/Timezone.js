import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

class TimezoneBehavior extends View.Behavior {
	onBack(container) {
		controller.timezone = this.view.timezone;
		controller.goBack();
	}
}

class TimezonePortBehavior extends Behavior {
	onCreate(container, view) {
		this.view = view;
		this.offsets = Uint16Array.of(
			0, // Samoa
			7, // Hawaii
			6, // Alaska
			29, // Pacific
			34, // Mountain
			55, // Central
			72, // Eastern
			80, // Atlantic
			86, // Uruguay
			113, // SGSSI
			123, // Azores
			129, // Greenwich Mean
			141, // Central European
			157, // Eastern European
			167, // Indian Ocean
			185, // Arabian
			188, // Pakistan
			198, // Bangladesh
			213, // Thailand
			212, // China
			239, // Japan
			259, // Australian Eastern
			268, // Vanuatu
			279, // New Zealand
		);
		this.widths = Uint16Array.of(
			16, // Samoa
			30, // Hawaii
			35, // Alaska
			30, // Pacific
			40, // Mountain
			25, // Central
			27, // Eastern
			27, // Atlantic
			55, // Uruguay
			19, // SGSSI
			21, // Azores
			28, // Greenwich Mean
			38, // Central European
			46, // Eastern European
			43, // Indian Ocean
			23, // Arabian
			48, // Pakistan
			47, // Bangladesh
			36, // Thailand
			54, // China
			39, // Japan
			26, // Australian Eastern
			22, // Vanuatu
			41, // New Zealand
		);
	}
	onDraw(port) {
		const timezone = this.view.timezone;
		let texture = new Texture({ path: `timezone-${ timezone }.png` });
		port.drawTexture(texture, assets.ORANGE, this.offsets[timezone], 0, 0, 0, this.widths[timezone], texture.height);
	}
}

class TimezoneScrollBehavior extends Behavior {
	onCreate(container, view) {
		this.view = view;
	}
	onChanged(container) {
		const view = this.view;
		view.timezone = this.index;
		view.NAME.string = controller.model.timezones[this.index];
		view.PORT.invalidate();
	}
	onDisplaying(container) {
		const view = this.view;
		const timezone = view.timezone;
		this.index = timezone;
		this.delta = Math.round(timezone * 80 / 23);
		view.MAP.x = view.PORT.x = container.x - this.delta;
		this.onChanged(container);
	}
	onTouchBegan(container, id, x, y, ticks) {
		container.captureTouch(id, x, y, ticks);
		this.x = x - container.x;
		this.anchor = this.delta;
		this.onTouchMoved(container, id, x, y, ticks);
	}
	onTouchEnded(container, id, x, y, ticks) {
	}
	onTouchMoved(container, id, x, y, ticks) {
		const view = this.view;
		x -= container.x;
		if (x < 0) x = 0;
		else if (x > 240) x = 240;
		if (x < this.x) {
			this.delta = this.anchor - Math.round((this.x - x) / this.x * this.anchor);
		}
		else {
			this.delta = this.anchor + Math.round((x - this.x) / (240 - this.x) * (80 - this.anchor));
		}
		view.MAP.x = view.PORT.x = container.x - this.delta;
		
		let index = Math.round((x + this.delta) * 24 / 320);
		if (index < 0)
			index = 0;
		else if (index > 23)
			index = 23;
		if (this.index != index) {
			controller.doPlayTumbler();
			this.index = index;
			this.onChanged(container);
		}
	}
}

const TimezoneContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:TimezoneBehavior,
	contents: [
		Container($, {
			left:0, top:0, skin:assets.skins.topArc2,
			contents: [
				Content($, { left:0, width:50, top:0, height:50, skin:assets.skins.back, active:true, Behavior:View.BackBehavior }),
				Label($, { top:5, style:assets.styles.title, string:$.data.title }),
			]
		}),
		Container($, {
			left:0, width:240, top:45, bottom:0,
			contents: [
				Label($, { anchor:"NAME", top:10, string:"Pacific" }),
				Container($, { 
					left:0, right:0, top:40, height:164, clip:true, active:true, Behavior:TimezoneScrollBehavior,
					contents: [
						Content($, { anchor:"MAP", left:0, width:320, top:0, height:164, skin:assets.skins.timezoneMap }),
						Port($, { anchor:"PORT", left:0, width:320, top:0, height:164, Behavior:TimezonePortBehavior }),
						Content($, { left:0, right:0, top:0, height:10, skin:assets.skins.topShadow }),
						Content($, { left:0, right:0, height:10, bottom:0, skin:assets.skins.bottomShadow }),
					]
				}),
				Text($, { left:10, right:10, bottom:10, style:assets.styles.ITALIC, string:$.data.comments }),
			]
		}),
	]
}));

export default class extends View {
	constructor(data) {
		super(data);
		this.timezone = controller.timezone;
	}
	get Template() { return TimezoneContainer }
};
