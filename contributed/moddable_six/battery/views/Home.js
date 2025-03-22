import {} from "piu/shape";
import {Outline} from "commodetto/outline";
import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

class HomeBehavior extends View.Behavior {
}

class PaneBehavior extends View.ButtonBehavior {
	onCreate(container, view, it) {
		this.view = view;
		this.data = it.data;
	}
	onDisplaying(container) {
		this.onUpdate(container);
	}
	onTap(container) {
		super.onTap(container);
		this.view.itemData = this.data;
		controller.goWith(this.data);
	}
	onUpdate(container) {
	}
}

class StatusPaneBehavior extends PaneBehavior {
	onUpdate(container) {
    	const { capacity, fullCharge, index, input, level, output, sign, zeroCharge } = controller;
    	const view = this.view;
		const label = view.LEVEL;
    	const shape = view.GAUGE;
		
		shape.state = level < 50 ? 0 : sign > 0 ? 1 : 2;
		
		if (shape.strokeOutline == null) {
			const path = new Outline.CanvasPath;
			path.arc(80, 80, 79, 0, 2 * Math.PI);
			shape.strokeOutline = Outline.stroke(path, 2);
    	}
		const path = new Outline.CanvasPath;
		path.arc(80, 80, 68, 0 - (Math.PI / 2), (level / 100 * 2 * Math.PI) - (Math.PI / 2));
		shape.fillOutline = Outline.stroke(path, 10, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
		
		view.CHARGE.string = (zeroCharge + ((fullCharge - zeroCharge) * level / 100)).toFixed(1) + "V";

		view.LEVEL.string = level;
		view.LEVEL_STATE.state = level < 50 ? 1 : 2;
		view.LEVEL_CLIP.height = Math.round((100 - level) * 36 / 100);
		view.LEVEL_BLACK.string = view.LEVEL_WHITE.string = Math.round(level * capacity / 100);
		view.HOURS.string = ((capacity * level / 100) / output).toFixed(1) + " hrs";
	}
}

class InputPaneBehavior extends PaneBehavior {
	onUpdate(container) {
    	const { index, input, output } = controller;
    	const view = this.view;
    	const delta = input - output;
  		view.INPUT.string = input.toFixed(1);
   		view.NET.string = delta.toFixed(1);
   		view.SIGN.variant = delta > 0 ? 0 : 1;
	}
}

class SettingsPaneBehavior extends PaneBehavior {
	changeState(container, state) {
		super.changeState(container, state);
		container.last.state = state;
	}
	onUpdate(container) {
		const { time } = controller;
		container.first.string = time;
	}
}

const StatusPaneContainer = Container.template($ => ({
	left:0, width:240, top:0, height:180, active:true, Behavior:StatusPaneBehavior,
	contents: [
		Container($, { 
			left:0, width:240, top:0, height:180, 
			contents: [
				Container($, { 
					left:10, width:160, top:10, height:160,
					contents: [
						Shape($, { anchor:"GAUGE", left:0, width:160, top:0, height:160, skin:assets.skins.level } ),
						Row($, {
							top:100, style:assets.styles.L,
							contents:[
								Label($, { anchor:"CHARGE", style:assets.styles.BOLD }),
								Label($, { string:"V" }),
							], 
						}),
					],
				}),
				Content($, { right:5, top:100, skin:assets.skins.icons, variant:0 }),
				Row($, {
					right:10, top:145,
					contents:[
						Label($, { anchor:"HOURS", style:assets.styles.BOLD }),
						Label($, { string:" hrs" }),
					], 
				}),
			], 
		}),
		Container($, { 
			left:10, width:160, top:10, height:160,
			contents: [
				Row($, { 
					top:21, height:90,
					contents: [
						Label($, { anchor:"LEVEL", top:0, style:assets.styles.XXXL }),
						Label($, { top:12, style:assets.styles.XL, string:"%" }),
					],
				}),
			], 
		}),
		Container($, { 
			width:86, right:10, height:57, top:10, 
			contents: [
				Container($, { 
					left:8, width:70, top:11, height:36, clip:true,
					contents: [
						Content($, { anchor:"LEVEL_STATE", left:0, width:70, top:0, height:36, skin:assets.skins.fill }),
						Row($, { 
							top:4,
							contents: [
								Label($, { anchor:"LEVEL_BLACK", style:assets.styles.BOLD, state:1 }),
								Label($, { state:1, string:"Ah" }),
							],
						}),
					],
				}),
				Container($, { 
					anchor:"LEVEL_CLIP", left:8, width:70, top:11, height:36, clip:true,
					contents: [
						Content($, { left:0, width:70, top:0, height:36, skin:assets.skins.fill }),
						Row($, { 
							top:4,
							contents: [
								Label($, { anchor:"LEVEL_WHITE", style:assets.styles.BOLD }),
								Label($, { string:"Ah" }),
							],
						}),
					],
				}),
				Content($, { left:0, top:0, skin:assets.skins.home, state:1 }),
			],
		}),
	], 
}));

const InputOutputPaneContainer = Container.template($ => ({
	left:0, width:240, top:180, height:45, active:true, Behavior:InputPaneBehavior,
	contents: [
		Container($, {
			left:0, width:240, top:0, height:45,
			contents: [
				Row($, {
					left:45,
					contents:[
						Label($, { anchor:"INPUT", style:assets.styles.BOLD }),
						Label($, { string:"A" }),
					], 
				}),
				Row($, {
					right:10,
					contents:[
						Content($, { anchor:"SIGN", width:30, skin:assets.skins.arrows, state:1 }),
						Label($, { anchor:"NET", style:assets.styles.BOLD }),
						Label($, { string:"A" }),
					], 
				}),
			]
		}),
		Content($, { left:5, top:2, skin:assets.skins.sun }),
	], 
}));

const StateOfChargePaneContainer = Container.template($ => ({
	left:0, width:240, top:225, height:55, active:true, Behavior:PaneBehavior,
	contents: [
		Port($, { left:33, right:37, top:5, bottom:5, Behavior: View.StateOfChargePortBehavior }),
	], 
}));

const SettingsPaneContainer = Container.template($ => ({
	left:0, width:240, top:280, height:40, active:true, Behavior:SettingsPaneBehavior,
	contents: [
		Label($, { left:0, height:50, style:assets.styles.time }),
		Content($, { right:5, bottom:-5, skin:assets.skins.icons, variant:2 }),
	], 
}));

const HomeContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, /*skin:assets.skins.screen,*/ style:assets.styles.screen, Behavior:HomeBehavior,
	contents: [
		StatusPaneContainer($, { data:$.data.items[0] }),
		Content($, { anchor:"SEPARATOR_1", left:10, width:220, height:1, top:179, skin:assets.skins.separator }),
		InputOutputPaneContainer($, { data:$.data.items[1] }),
		Content($, { anchor:"SEPARATOR_2", left:10, width:220, height:1, top:224, skin:assets.skins.separator }),
		StateOfChargePaneContainer($, { data:$.data.items[2] }),
		Content($, { anchor:"SEPARATOR_3", left:10, width:220, height:1, top:280, skin:assets.skins.separator }),
		SettingsPaneContainer($, { data:$.data.items[3] }),
	],
}));

class HomeTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		const status = screen.first;
		const separator1 = status.next;
		const io = separator1.next;
		const separator2 = io.next;
		const charge = separator2.next;
		const separator3 = charge.next;
		const settings = separator3.next;
		const { x, y, width, height } = screen;
		const { duration, delay } = assets.transitions;
	
		if (other.id == "Status") {
			const rest = status.first;
			const percent = rest.next;
			const battery = percent.next;
			
			let x0 = 77, y0 = 126, x1 = 144, y1 = 10;
			let cx = x + x0, cy = y + y1;
			let dx = x1 - x0, dy = y0 - y1, a = 0, da = Math.PI / 8;
			let i = 0;
			let xs = new Array(5);
			let ys = new Array(5);
			while (i < 5) {
				xs[i] = cx + Math.round(Math.sin(a) * dx);
				ys[i] = cy + Math.round(Math.cos(a) * dy);
				a += da;
				i++;
			}
			this.from(percent, { x:x + 40, y:y + 21 }, duration, Math.quadEaseOut, 0);
			this.on(battery, { x:xs, y:ys }, duration, Math.quadEaseOut, -duration);

			this.from(rest, { y:y - rest.height }, duration, Math.quadEaseOut, -duration);
			this.from(separator1, { y:other.SEPARATOR_1.y  }, duration, Math.quadEaseOut, -duration);
			this.from(io, { y:y + height }, duration, Math.quadEaseOut, -duration);
			this.from(separator2, { y:other.SEPARATOR_2.y  }, duration, Math.quadEaseOut, -duration);
			this.from(charge, { y:y + height + io.height }, duration, Math.quadEaseOut, -duration);
			this.from(separator3, { y:y + height + io.height + charge.height - 1  }, duration, Math.quadEaseOut, -duration);
			this.from(settings, { y:y + height + io.height + charge.height }, duration, Math.quadEaseOut, -duration);
		
			return;
		}
		if (other.id == "InputOutput") {
			const rest = io.first;
			const sun = rest.next;
			
			this.from(sun, { x:other.SUN.x, y:other.SUN.y }, duration, Math.quadEaseOut, 0);
			
			this.from(separator1, { y:other.SEPARATOR_1.y  }, duration, Math.quadEaseOut, -duration);
			this.from(separator2, { y:other.SEPARATOR_2.y  }, duration, Math.quadEaseOut, -duration);
			this.from(separator3, { y:other.SEPARATOR_3.y  }, duration, Math.quadEaseOut, -duration);
			
			this.from(status, { y:y - status.height - rest.height }, duration, Math.quadEaseOut, -duration);
			this.from(rest, { y:y - rest.height }, duration, Math.quadEaseOut, -duration);
			this.from(charge, { y:y + height }, duration, Math.quadEaseOut, -duration);
			this.from(settings, { y:y + height+ charge.height }, duration, Math.quadEaseOut, -duration);
			
			return;
		}
		if (other.id == "StateOfCharge") {
			this.from(charge, { y:y + 60, height:210 }, duration, Math.quadEaseOut, 0);

			this.from(status, { y:y - status.height - io.height  }, duration, Math.quadEaseOut, -duration);
			this.from(separator1, { y:y - status.height - io.height  }, duration, Math.quadEaseOut, -duration);
			this.from(io, { y:y - io.height}, duration, Math.quadEaseOut, -duration);
			
			this.from(separator2, { y:y + 49  }, duration, Math.quadEaseOut,-duration);
			
			this.from(separator3, { y:y + height  }, duration, Math.quadEaseOut, -duration);
			this.from(settings, { y:y + height + separator3.height }, duration, Math.quadEaseOut, -duration);
			return;
		}
		if (other.id == "Settings") {
			let content = screen.first;
			const contentDuration = delay + duration;
			let contentDelay = 0;
			while (content) {
				this.from(content, { x:screen.x - screen.width }, contentDuration, Math.quadEaseOut, contentDelay);
				content = content.next;
				contentDelay = -contentDuration;
			}
			this.simultaneous = true;
			return;
		}
	}
}

export default class extends View {
	static get Status() { return StatusPaneContainer }
	static get InputOutput() { return InputOutputPaneContainer }
	static get StateOfCharge() { return StateOfChargePaneContainer }
	static get Settings() { return SettingsPaneContainer }

	constructor(data) {
		super(data);
		this.itemData = null;
	}
	get Template() { return HomeContainer }
	get Timeline() { return HomeTimeline }
};
