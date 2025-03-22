import {} from "piu/shape";
import {Outline} from "commodetto/outline";
import { ToggleBehavior } from "ToggleBehavior";
import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

class SettingsBehavior extends View.Behavior {
	onItemSelected(container, data) {
		controller.goWith(data);
	}
	onUpdate(container) {
		const view = this.view;
		view.COLUMN.first.last.string = controller.time;
	}
}

class DSTToggleBehavior extends ToggleBehavior {
	onCreate(container, data) {
		this.data = { value:controller.dst };
	}
	onValueChanged(container) {
		controller.doPlayToggle();
		controller.dst = this.data.value
	}
}

const SettingsContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:SettingsBehavior,
	contents: [
		Container($, {
			left:0, width:240, top:60, bottom:0,
			contents: [
				Content($, { left:0, right:0, top:0, height:10, skin:assets.skins.topShadow }),
				Column($, { 
					anchor:"COLUMN", left:0, right:0, top:0, contents: $.data.items.map($$ => new $.constructor[$$.Item]($$, { view:$ })),
				}),
				Text($, { left:10, right:10, bottom:15, style:assets.styles.ITALIC, string:$.data.comments }),
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
				Content($, { left:0, width:50, top:0, height:50, skin:assets.skins.back, active:true, Behavior:View.BackBehavior  }),
				Label($, { top:5, style:assets.styles.title, string:"Settings" }),
			]
		}),
	]
}));

const DSTSettingItemRow = Row.template($ => ({
	left:10, right:10, height:42, skin:assets.skins.setting, style:assets.styles.setting,
	contents: [
		Label($, { left:0, right:0, style:assets.styles.LEFT, string:$.name }),
		Container($, {
			width:50, height:24, active:true, Behavior:DSTToggleBehavior,
			contents: [
				Content($, { left:0, width:50, height:24, skin:assets.skins.toggleBar }),
				Content($, { left:0, width:24, height:22, skin:assets.skins.toggleButton }),
			]
		}),
	]
}));

const SettingItemRow = Row.template($ => ({
	left:10, right:10, height:42, skin:assets.skins.setting, style:assets.styles.setting, active:true, Behavior:View.ItemBehavior,
	contents: [
		Label($, { left:0, right:0, style:assets.styles.LEFT, string:$.name }),
		$.id ? Label($, { left:0, right:0, style:assets.styles.itemValue, string:controller[$.id] }) : null,
	]
}));

class SettingsTimeline extends Timeline {
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
	static get DSTSettingItem() { return DSTSettingItemRow }
	static get SettingItem() { return SettingItemRow }
	constructor(data) {
		super(data);
	}
	get Template() { return SettingsContainer }
	get Timeline() { return SettingsTimeline }
};
