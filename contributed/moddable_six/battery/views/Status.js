import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

class StatusBehavior extends View.Behavior {
	get fade() {
		return 0;
	}
	set fade(it) {
		const view = this.view;
		let content = view.INFOS.first;
		while (content) {
			content.state = it;
			content = content.next;	
		}
		content = view.NET;
		content.next.state = content.state = it;
		content = view.CHARGE;
		content.next.state = content.state = it;
		content = view.TEMPERATURE;
		content.next.state = content.state = it;
	}
	onDisplaying(container) {
		this.onUpdate(container);
	}
	onUpdate(container) {
    	const { capacity, fullCharge, index, input, level, output, sign, zeroCharge } = controller;
    	const view = this.view;
    	const delta = input - output;
    	
    	view.LEVEL.string = level;
    	view.SIGN.variant = delta > 0 ? 0 : 1;

		view.LEVEL_STATE.state = level < 50 ? 1 : 2;
		view.LEVEL_CLIP.height = Math.round((100 - level) * 58 / 100);
		view.LEVEL_BLACK.string = view.LEVEL_WHITE.string = Math.round(level * capacity / 100);
		
   		view.NET.string = delta.toFixed(1);
		view.CHARGE.string = (zeroCharge + ((fullCharge - zeroCharge) * level / 100)).toFixed(1);
		view.TEMPERATURE.string = 77;
		
		view.HOURS.string = ((capacity * level / 100) / output).toFixed(1);
	}
}

class StatusSignBehavior extends Behavior {
	onDisplaying(container) {
		container.duration = 1000;
		container.start();
	}
	onFinished(container) {
		container.time = 0;
		container.start();
	}
	onTimeChanged(container) {
		let content = container.first;
		let fraction = Math.quadEaseOut(container.fraction);
		content.state = fraction;
		if (content.variant == 0)
			fraction = 1 - fraction;
		content.y = container.y + ((container.height - content.height) * fraction);
	}
}


const StatusContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, style:assets.styles.screen, Behavior:StatusBehavior,
	contents: [
		new $.constructor.TitleBar($, { state:1 }),
		Content($, { anchor:"SEPARATOR_1", left:10, right:10, height:1, top:49, skin:assets.skins.separator }),
		Container($, { 
			left:40, width:160, top:21, height:160,
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
			left:184, width:20, top:70, height:40, Behavior: StatusSignBehavior,
			contents: [
				Content($, { anchor:"SIGN", top:0, skin:assets.skins.arrows }),
			],
		}),
		Container($, {
			top:126, height:76,
			contents: [
				Container($, { 
					left:3, width:108, top:14, height:58, clip:true,
					contents: [
						Content($, { anchor:"LEVEL_STATE", left:0, right:0, top:0, bottom:0, skin:assets.skins.fill }),
						Row($, { 
							top:12, style:assets.styles.L, 
							contents: [
								Label($, { anchor:"LEVEL_BLACK", style:assets.styles.BOLD, state:1 }),
								Label($, { state:1, string:"Ah" }),
							],
						}),
					],
				}),
				Container($, { 
					anchor:"LEVEL_CLIP", left:3, width:108, top:14, height:58, clip:true,
					contents: [
						Content($, { left:0, right:0, top:0, bottom:0, skin:assets.skins.fill }),
						Row($, { 
							top:12, style:assets.styles.L,
							contents: [
								Label($, { anchor:"LEVEL_WHITE", style:assets.styles.BOLD }),
								Label($, { string:"Ah" }),
							],
						}),
					],
				}),
				Content($, { left:0, top:0, skin:assets.skins.status, state:1 }),
			],
		}),
		Container($, {
			anchor:"INFOS", left:0, width:240, height:73, bottom:60,
			contents: [
				Content($, { left:32, width:27, top:0, height:2, skin:assets.skins.separator, state:0 }),
				Content($, { right:32, width:27, top:0, height:2, skin:assets.skins.separator, state:0 }),
				Content($, { left:32, width:2, top:0, height:33, skin:assets.skins.separator, state:0 }),
				Content($, { left:119, width:2, top:23, height:10, skin:assets.skins.separator, state:0 }),
				Content($, { right:32, width:2, top:0, height:33, skin:assets.skins.separator, state:0 }),
				
				Container($, {
					left:0, right:0, height:40, bottom:0,
					contents: [
						Row($, {
							left:10,
							contents:[
								Label($, { anchor:"NET", style:assets.styles.BOLD }),
								Label($, { string:"A" }),
							], 
						}),
						Row($, {
							contents:[
								Label($, { anchor:"CHARGE", style:assets.styles.BOLD }),
								Label($, { string:"V" }),
							], 
						}),
						Row($, {
							right:10,
							contents:[
								Label($, { anchor:"TEMPERATURE", style:assets.styles.BOLD }),
								Label($, { string:"°F" }),
							], 
						}),
					],
				}),
			],
		}),
		Content($, { anchor:"SEPARATOR_2", left:10, right:10, height:1, bottom:60, skin:assets.skins.separator }),
		Row($, {
			height:60, bottom:0,
			contents: [
				Content($, { skin:assets.skins.icons, variant:0 }),
				Column($, {
					contents:[
						Row($, {
							left:0, height:22, 
							contents:[
								Label($, { anchor:"HOURS", style:assets.styles.BOLD }),
								Label($, { string:" hrs" }),
							], 
						}),
						Label($, { left:0, height:22, string:"@ current load" }),
					], 
				}),
			],
		}),
	]
}));

class StatusTimeline extends Timeline {
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
	}
	get Template() { return StatusContainer }
	get Timeline() { return StatusTimeline }
};
