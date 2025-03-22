import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

class CapacityBehavior extends View.Behavior {
	onDelete(container, string) {
		const view = this.view;
		const label = this.view.FIELD;
		let value = label.string;
		if (value.length > 1)
			value = value.slice(0, -1);
		else
			value = "0";
		label.string = value;
		controller[view.data.id] = parseInt(value);
	}
	onDigit(container, string) {
		const view = this.view;
		const label = this.view.FIELD;
		let value = label.string;
		if (value == "0")
			value = string;
		else if (value.length < 4)
			value += string;
		label.string = value;
		controller[view.data.id] = parseInt(value);
	}
}

class DeleteIconBehavior extends View.ButtonBehavior {
	onTap(label) {
		super.onTap(label);
		label.state = 0;
		label.bubble("onDelete", label.string);
	}
}

class DigitLabelBehavior extends View.ButtonBehavior {
	onTap(label) {
		super.onTap(label);
		label.state = 0;
		label.bubble("onDigit", label.string);
	}
}

const DigitLabel = Label.template($ => ({
	width:78, top:0, height:50, skin:assets.skins.button, active:true, Behavior:DigitLabelBehavior,
}));

const CapacityContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, style:assets.styles.screen, Behavior:CapacityBehavior,
	contents: [
		new $.constructor.TitleBar($),
		Column($, {
			left:0, width:240, top:50, bottom:0, style:assets.styles.pad,
			contents: [
				Label($, { anchor:"FIELD", left:0, right:0, height:50, skin:assets.skins.field, string:$.value }),
				Content($, { height:6 }),
				Row($, {
					left:3, top:1,
					contents: [
						DigitLabel($, { string:"1" }),
						DigitLabel($, { string:"2" }),
						DigitLabel($, { string:"3" }),
					],
				}),
				Row($, {
					left:3, top:1,
					contents: [
						DigitLabel($, { string:"4" }),
						DigitLabel($, { string:"5" }),
						DigitLabel($, { string:"6" }),
					],
				}),
				Row($, {
					left:3, top:1,
					contents: [
						DigitLabel($, { string:"7" }),
						DigitLabel($, { string:"8" }),
						DigitLabel($, { string:"9" }),
					],
				}),
				Row($, {
					left:3, top:1,
					contents: [
						DigitLabel($, { width:156, string:"0" }),
						Container($, { 
							width:78, top:0, height:52, skin:assets.skins.button, active:true, Behavior: DeleteIconBehavior, 
							contents: [
								Content($, { skin:assets.skins.icons, state:2, variant:4 }),
							],
						}),
					],
				}),
			],
		}),
	]
}));

export default class extends View {
	constructor(data) {
		super(data);
		this.value = controller[data.id];
	}
	get Template() { return CapacityContainer }
};
