import assets from "assets";
import Timeline from "piu/Timeline";
import View from "View";

class ImageContainerBehavior extends View.Behavior {
	onCreate(container, view) {
		super.onCreate(container, view);
	}
}

class ImageBehavior extends Behavior {
	onDisplaying(image) {
		image.start();
	}
	onFinished(image) {
		image.bubble("onStep");
	}
};

const ImageContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:ImageContainerBehavior,
	contents: [
		Image($, { duration:1000, path:"robby.cs", Behavior:ImageBehavior }),
	]
}));

class ImageTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
	}
}

export default class extends View {
	static get Behavior() { return ImageBehavior }
	
	constructor(data) {
		super(data);
	}
	get Template() { return ImageContainer }
	get Timeline() { return ImageTimeline }
};
