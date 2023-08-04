/*
 * Copyright (c) 2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import assets from "assets";
import Timeline from "piu/Timeline";
import View from "View";

class HomeBehavior extends View.Behavior {
	onDisplayed(container) {
		this.offset = 0;
		container.duration = 1000;
		container.start() ;
	}
	onFinished(container) {
		const scroller = container.last.first;
		this.offset = scroller.scroll.y;
		container.time = 0;
		container.start() ;
	}
	onJogDialReleased(container) {
		controller.goWith(controller.model.menu);
		return true;
	}
	onTimeChanged(container) {
		const scroller = container.last.first;
		scroller.scrollTo(0, this.offset - (Math.quadEaseOut(container.fraction) * 64));
	}
	onUndisplaying(container) {
		container.stop();
		container.time = container.duration;
	}
}

const HomeContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:HomeBehavior,
	contents: [
		Content($, { left:0, width:50, top:7, height:50, skin:assets.skins.logo }),
		Content($, { left:0, width:128, top:7, height:50, skin:assets.skins.moddable }),
		Container($, {
			left:0, width:128, top:64, height:64, clip:true, skin:assets.skins.focus, state:1,
			contents: [
				Scroller($, {
					left:0, right:0, top:0, bottom:0, looping:true,
					contents: [
						Column($, {
							left:0, right:0, top:0, 
							contents: [
								Label($, { width:128, height:64, string:"Hello", style:{ font:"40px Arial Narrow", color:"black" } }),
								Label($, { width:128, height:64, string:"你好", style:{ font:"36px NotoSansTC" , color:"black"} }),
								Label($, { width:128, height:64, string:"Bonjour", style:{ font:"italic 32px Arial Narrow", color:"black" } }),
								Label($, { width:128, height:64, string:"Hallo", style:{ font:"bold 32px Arial Narrow", color:"black" } }),
								Label($, { width:128, height:64, string:"Ciao", style:{ font:"italic 40px Arial Narrow", color:"black" } }),
								Label($, { width:128, height:64, string:"こんにちは", style:{ font:"24px NotoSansJP", color:"black" } }),
								Label($, { width:128, height:64, string:"안녕하세요", style:{ font:"26px NotoSansKR", color:"black" } }),
								Label($, { width:128, height:64, string:"Hola", style:{ font:"bold 36px Arial Narrow", color:"black" } }),
								Label($, { width:128, height:64, string:"Привіт", style:{ font:"italic 36px Arial Narrow", color:"black" } }),
							]
						}),
					]
				}),
			]
		}),
	]
}));

class HomeTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super();
		let logo = screen.first;
		let moddable = logo.next;
		let hello = moddable.next;
		if (controller.going != direction) {
			this.from(logo, { x:screen.x - logo.width }, 250, Math.quadEaseOut, 0);
			this.from(moddable, { x:screen.x - moddable.width }, 250, Math.quadEaseOut, -125);
			this.from(hello, { x:screen.x - hello.width }, 250, Math.quadEaseOut, -125);
		}
		else {
			this.from(logo, { x:screen.x + screen.width }, 250, Math.quadEaseOut, 0);
			this.from(moddable, { x:screen.x + screen.width }, 250, Math.quadEaseOut, -125);
			this.from(hello, { x:screen.x + screen.width }, 250, Math.quadEaseOut, -125);
		}
	}
}

export default class extends View {
	static get Behavior() { return HomeBehavior }
	
	constructor(data) {
		super(data);
	}
	get Template() { return HomeContainer }
	get Timeline() { return HomeTimeline }
};
