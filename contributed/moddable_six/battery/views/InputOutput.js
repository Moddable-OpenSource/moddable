import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

class InputOutputBehavior extends View.Behavior {
	onDisplaying(container) {
		this.onUpdate(container);
	}
	onUpdate(container) {
    	const { index, input, output } = controller;
    	const view = this.view;
   		view.INPUT.string = input.toFixed(1);
   		view.OUTPUT.string = "-" + output.toFixed(1);
   		const delta = input - output;
   		view.NET.string = delta.toFixed(1);
	}
}

const InputOutputContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, style:assets.styles.screen, Behavior:InputOutputBehavior,
	contents: [
		new $.constructor.TitleBar($, { state:1 }),
		Container($, {
			left:0, width:120, top:50, height:270,
			contents:[
				Container($, {
					left:0, width:120, top:0, height:100,
					contents:[
						Label($, { left:10, top:5, string:"Solar" }),
						Content($, { left:10, top:30, skin:assets.skins.input}),
					], 
				}),
				Container($, {
					left:0, width:120, top:100, height:100,
					contents:[
						Label($, { left:10, top:5, string:"Loads" }),
						Content($, { left:10, top:35, skin:assets.skins.output }),
					], 
				}),
				Container($, {
					left:0, width:120, top:200, height:70,
					contents:[
						Label($, { left:10, string:"Net" }),
					], 
				}),
			], 
		}),
		Container($, {
			right:0, width:120, top:50, height:270,
			contents:[
			Row($, {
				right:10, top:0, height:100, style:assets.styles.XL,
				contents:[
					Label($, { anchor:"INPUT", style:assets.styles.BOLD }),
					Label($, { string:"A" }),
				], 
			}),
			Row($, {
				right:10, top:100, height:100, style:assets.styles.XL,
				contents:[
					Label($, { anchor:"OUTPUT", style:assets.styles.BOLD }),
					Label($, { string:"A" }),
				], 
			}),
			Row($, {
				right:10, top:200, height:70, style:assets.styles.XL,
				contents:[
					Label($, { anchor:"NET", style:assets.styles.BOLD }),
					Label($, { string:"A" }),
				], 
			}),
			], 
		}),
		Content($, { anchor:"SEPARATOR_1", left:10, right:10, top:49, height:1, skin:assets.skins.separator }),
		Content($, { anchor:"SEPARATOR_2", left:10, right:10, top:149, height:1, skin:assets.skins.separator }),
		Content($, { anchor:"SEPARATOR_3", left:10, right:10, top:249, height:1, skin:assets.skins.separator }),
		Content($, { anchor:"SUN", left:5, top:80, skin:assets.skins.sun }),
	]
}));

class InputOutputTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		const { delay, duration } = assets.transitions;
		const header = screen.first;
		const left = header.next;
		const right = left.next;
		this.from(left, { x:screen.x - left.width }, duration, Math.quadEaseOut, 0);
		this.from(right, { x:screen.x + screen.width }, duration, Math.quadEaseOut, -duration);
		this.from(header, { x:screen.x + screen.width }, duration, Math.quadEaseOut, -delay);
	}
}
export default class extends View {
	constructor(data) {
		super(data);
	}
	get Template() { return InputOutputContainer }
	get Timeline() { return InputOutputTimeline }
};
