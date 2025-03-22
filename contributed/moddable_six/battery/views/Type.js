import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

class TypeBehavior extends View.Behavior {
	onCreate(container, view) {
		super.onCreate(container, view);
		const grid = view.GRID;
		const range = view.data.range;
		const length = range.length;
		for (let index = 0; index < length; index++) {
			grid.add(new TypeItemContainer(range[index], { left:120 * (index & 1), top:120 * (index >> 1) }));
		}
		container.distribute("onSelected", controller.type);
	}
	onSelect(container, type) {
		controller.type = type;
		container.distribute("onSelected", type);
	}
}

class TypeItemBehavior extends View.ButtonBehavior {
	changeState(container, state) {
		super.changeState(container, state);
		container.first.state = state;
	}
	onSelected(container, type) {
		container.first.state = this.data == type ? 1 : 0;
		container.last.visible = this.data == type;
	}
	onTap(container) {
		super.onTap(container);
		container.state = 0;
		container.bubble("onSelect", this.data);
	}
}

const TypeContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, style:assets.styles.screen, Behavior:TypeBehavior,
	contents: [
		new $.constructor.TitleBar($),
		Scroller($, {
			left:0, width:240, top:50, bottom:0, clip:true, active:true, backgroundTouch:true,
			Behavior:View.VerticalScrollerBehavior,
			contents: [
				Container($, { anchor:"GRID", left:0, right:0, top:0 } )
			]
		}),
	]
}));

const TypeItemContainer = Container.template($ => ({
	width:120, height:120, active:true, Behavior:TypeItemBehavior,
	contents: [
		Content($, { top:22, skin:assets.skins.type }),
		Label($, { bottom:5, string:$ }),
		Content($, { skin:assets.skins.icons, variant:5, visible:false }),
	],
}));

export default class extends View {
	constructor(data) {
		super(data);
	}
	get Template() { return TypeContainer }
};
