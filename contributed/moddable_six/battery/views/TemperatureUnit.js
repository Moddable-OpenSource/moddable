import assets from "assets";
import Timeline from "piu/Timeline";
import { ToggleBehavior } from "ToggleBehavior";
import View from "Common";

class TemperatureUnitBehavior extends View.Behavior {
	onBack(container) {
		const view = this.view;
		controller.temperatureUnit = view.value ? "°F" : "°C";
		controller.goBack();
	}
	onCreate(container, view) {
		super.onCreate(container, view);
	}
}
class TemperatureUnitToggleBehavior extends ToggleBehavior {
	onValueChanged(container) {
		controller.doPlayToggle();
	}
}

const TemperatureUnitContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, style:assets.styles.screen, Behavior:TemperatureUnitBehavior,
	contents: [
		new $.constructor.TitleBar($),
		Container($, {
			left:0, width:240, top:50, bottom:0, clip:true,
			contents: [
				Content($, { top:26, skin:assets.skins.status, state:1 }),
				Container($, {
					width:80, top:140, height:30, active:true, Behavior:TemperatureUnitToggleBehavior,
					contents: [
						Row($, { 
							width:80, height:30, skin:assets.skins.temperatureUnitBar, style:assets.styles.BOLD,
							contents: [
								Label($, { width:40, top:0, bottom:0, string:"°F" }),
								Label($, { width:40, top:0, bottom:0, string:"°C" }),
							],
						}),
						Content($, { left:0, width:40, height:30, skin:assets.skins.temperatureUnitButton }),
					]
				}),
			]
		}),
	]
}));

export default class extends View {
	constructor(data) {
		super(data);
		this.value = controller.temperatureUnit == '°F';
	}
	get Template() { return TemperatureUnitContainer }
};
