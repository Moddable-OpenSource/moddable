import assets from "assets";
import GIFImage from "piu/GIFImage";
import Timeline from "piu/Timeline";
import View from "View";

class SplashBehavior extends View.Behavior {
	onJogDialReleased(container) {
		container.first.stop();
		controller.goTo("Home");
		return true;
	}
}

class GIFImageBehavior extends Behavior {
	onDisplaying(image) {
		image.start();
	}
	onFinished(image) {
		controller.goTo("Home");
	}
};

const SplashContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:SplashBehavior,
	contents: [
		GIFImage($, { left:0, top:0, width:128, height:128, path:"splash.gif", Behavior:GIFImageBehavior }),
	]
}));

class SplashTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super();
		let image = screen.first;
		this.from(image, { x:screen.x - image.width }, 250, Math.quadEaseOut, 0);
	}
}

export default class extends View {
	static get Behavior() { return SplashBehavior }
	
	constructor(data) {
		super(data);
	}
	get Template() { return SplashContainer }
	get Timeline() { return SplashTimeline }
	get historical() { return false }
};
