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

import AudioOut from "pins/audioout"
import MP3Streamer from "mp3streamer";

import config from "mc/config";
import {} from "piu/MC";
import { VerticalScrollerBehavior } from "ScrollerBehaviors";
import Timeline from "piu/Timeline";
import channels from "channels";

const BLACK = "black"
const GRAY = "#808080"
const RED = "#ff3333"
const WHITE = "#ffffff";

const backgroundSkin = new Skin({ fill: BLACK });
const headerSkin = new Skin({ fill:"#000000C0" });
const footerSkin = new Skin({ fill:"#000000C0" });
const iconsSkin = new Skin({ texture: new Texture("icons.png"), width:40, height:40, variants:40, color:[WHITE,RED] });
const volumeTexture =  new Texture("volume.png");
const volumeSkin = new Skin({ texture: volumeTexture, width:40, height:40, variants:40, color:WHITE });
const sliderSkin = new Skin({ texture: volumeTexture, x:80, width:40, height:40, left:10, right:10, color:[GRAY,WHITE] });

const logoStyle = new Style({ font:"40px FiraSansCondensed", color:RED });
const listTitleStyle = new Style({ font:"bold 20px FiraSansCondensed", color:WHITE, horizontal:"left", left:5 });
const listGenreStyle = new Style({ font:"16px FiraSansCondensed", color:WHITE, horizontal:"left", left:5 });
let paneTitleStyle;
let paneDescriptionStyle;

let ChannelPane;

const model = {
	channels,
	current: null,
	home: true,
	scroll: { x:0, y:0 },
	streamer: null,
};

const audio = new AudioOut({});

class ApplicationBehavior {
	goToList(application) {
		const pane = application.first;
		const list = new ChannelList(model);
		application.distribute("onUndisplaying");
		model.home = true;
		application.run(new TimelineTransition(ChannelPaneTimeline, ChannelListTimeline), pane, list);
	}
	goToPane(application, channel) {
		if (model.current != channel) {
			this.onPause(application);
			model.current = channel;
		}
		const list = application.first;
		const pane = new ChannelPane(channel);
		application.distribute("onUndisplaying");
		model.home = false;
		application.run(new TimelineTransition(ChannelListTimeline, ChannelPaneTimeline), list, pane);
	}
	onCreate(container, $) {
		this.going = false;
		application.replace(application.first, new ChannelList(model));
	}
	onDisplayed(container) {
		this.going = false;
	}
	onGoToList(application) {
		if (this.going)
			return;
		this.going = true;
		application.defer("goToList");
	}
	onGoToPane(application, channel) {
		if (this.going)
			return;
		this.going = true;
		application.defer("goToPane", channel);
	}
	onPlay(application) {
		model.streamer = new MP3Streamer({
			http: device.network.http,
			host: "ice2.somafm.com",
			path: `/${model.current.id}-128-mp3`,
			audio: {
				out: audio,
				stream: 0
			},
			onReady(state) {
				trace(`MP3 Ready: ${state}\n`);
				if (state)
					audio.start();
				else
					audio.stop();
			},
			onError(e) {
				trace("MP3 ERROR: ", e, "\n");
			},
			onDone() {
				trace("MP3 Done\n");
			}
		});
		application.distribute("onModelChanged");
	}
	onPause(application) {
		if (model.streamer) {
			model.streamer.close();
			model.streamer = null;
			application.distribute("onModelChanged");
		}
		audio.enqueue(0, AudioOut.Flush);
		audio.stop();
	}
	onUndisplaying(content) {
	}
	onVolumeChanged(application, fraction) {
		if (audio.length(0))
			audio.enqueue(0, AudioOut.Volume, 256 * fraction);
//		else
//			trace("audio queue full on set volume\n");
	}
}

class ChannelItemBehavior {
	changeState(container, from, to, duration) {
		this.from = from;
		this.to = to;
		container.duration = duration;
		container.time = 0;
		container.start();
	}
	onCreate(container, $) {
		this.channel = $;
	}
	onDisplaying(container) {
		container.state = (model.current == this.channel) ? 1 : 0;
	}
	onDisplayed(container) {
		container.active = true;
		if (container.state == 1)
			this.changeState(container, 1, 0, 250);
	}
	onTimeChanged(container) {
		container.state = this.from + (container.fraction * (this.to - this.from));
	}
	onTouchBegan(container, id, x, y) {
		this.changeState(container, 0, 1, 250);
	}
	onTouchCancelled(container, id, x, y) {
		container.stop();
		this.changeState(container, container.fraction, 0, container.time);
	}
	onTouchEnded(container, id, x, y) {
		container.bubble("onGoToPane", this.channel);
	}
	onUndisplaying(content) {
		content.active = false;
	}
}

class ChannelPaneBehavior {
	onDisplayed(container) {
		if (model.streamer == null)
			container.bubble("onPlay");
	}
}

class NowPlayingBehavior {
	onCreate(content) {
		content.interval = 250;
	}
	onDisplayed(content) {
		if (model.home) {
			if (model.current) {
				content.active = true;
				content.state = 0;
				content.variant = 0;
				content.visible = true;
				content.start();
			}
			else {
				content.active = false;
				content.state = 0;
				content.variant = 7;
				content.visible = false;
			}
		}
		else {
			content.active = true;
			content.state = 0;
			content.variant = 6;
			content.visible = true;
		}
	}
	onTimeChanged(content) {
		let variant = content.variant + 1;
		if (variant > 3)
			variant = 0;
		content.variant = variant;	
	}
	onTouchBegan(content, id, x, y) {
		content.state = 1;
	}
	onTouchEnded(content, id, x, y) {
		if (content.variant == 6)
			content.bubble("onGoToList");
		else if (content.variant != 7)
			content.bubble("onGoToPane", model.current);
	}
	onUndisplaying(content) {
		content.stop();
	}
}

class PlayPauseBehavior {
	onModelChanged(content) {
		if (model.streamer) {
			content.active = true;
			content.state = 0;
			content.variant = 5;
			content.visible = true;
		}
		else if (model.current) {
			content.active = true;
			content.state = 0;
			content.variant = 4;
			content.visible = true;
		}
		else {
			content.active = false;
			content.state = 0;
			content.variant = 4;
			content.visible = false;
		}
	}
	onTouchBegan(content, id, x, y) {
		content.state = 1;
	}
	onTouchEnded(content, id, x, y) {
		if (content.variant == 4) {
			content.bubble("onPlay");
		}
		else {
			content.bubble("onPause");
		}
	}
}

class VolumeBehavior {
	onDisplaying(container) {
		const bar = container.first.next;
		const thumb = bar.first;
		thumb.width = bar.width;
	}
	onTouchBegan(container, id, x, y) {
		this.onTouchMoved(container, id, x, y);
	}
	onTouchEnded(container, id, x, y) {
		this.onTouchMoved(container, id, x, y);
	}
	onTouchMoved(container, id, x, y) {
		const bar = container.first.next;
		const thumb = bar.first;
		const width = bar.width - 20;
		x -= bar.x + 10;
		if (x < 0)
			x = 0;
		else if (x > width)
			x = width;
		thumb.width = 20 + x;
		container.bubble("onVolumeChanged", x / width);
	}
}

const ChannelList = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:backgroundSkin,
	contents: [
		Scroller($, {
			left:0, right:0, top:40, bottom:40, Behavior:VerticalScrollerBehavior, active:true, backgroundTouch:true,
			contents: [
				Column($, {
					left:0, right:0, top:0, 
					contents: $.channels.map($$ => new ChannelItem($$)),
				}),
			]
		}),
	]
}));

const ChannelItem = Container.template($ => ({
	left:0, width:application.width, height:80, skin:$.color, Behavior:ChannelItemBehavior, active:true,
	contents: [
		Content($, { left:0, top:0, skin:$.image }),
		Column($, {
			left:80, right:0, 
			contents: [
				Text($, { left:0, right:0, style:listTitleStyle, string:$.title }),
				Label($, { left:0, right:0, style:listGenreStyle, string:$.genre }),
			]
		})
	]
}));

const HorizontalChannelPane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:$.color, Behavior:ChannelPaneBehavior,
	contents: [
		Container($, {
			left:0, right:0, top:0, bottom:0, 
			contents: [
				Content($, { left:10, skin:$.image }),
				Column($, {
					left:100, right:0, 
					contents: [
						Text($, { left:0, width:application.width - 100, style:paneTitleStyle, string:$.title }),
						Text($, { left:0, width:application.width - 100, top:10, style:paneDescriptionStyle, string:$.description }),
					]
				})
			]
		})
	]
}));

const VerticalChannelPane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:$.color, Behavior:ChannelPaneBehavior,
	contents: [
		Column($, {
			left:0, right:0, 
			contents: [
				Content($, { left:(application.width - 80) >> 1, skin:$.image }),
				Column($, {
					left:0, right:0, top:10, 
					contents: [
						Text($, { left:0, width:application.width, style:paneTitleStyle, string:$.title }),
						Text($, { left:0, width:application.width, top:10, style:paneDescriptionStyle, string:$.description }),
					]
				})
			]
		})
	]
}));

class TimelineTransition extends Transition {
	constructor(FromTimeline, ToTimeline) {
		super(0);
		this.FromTimeline = FromTimeline;
		this.ToTimeline = ToTimeline;
	}
	onBegin(container, from, to) {
		container.insert(to, from);
		this.fromContent = from;
		this.toContent = to;
		this.fromTimeline = new this.FromTimeline(container, from, to, -1);
		this.toTimeline = new this.ToTimeline(container, to, from, 1);
		const fromDuration = this.fromTimeline.duration;
		const toDuration = this.toTimeline.duration;
		const duration = fromDuration + toDuration;
		this.duration = duration;
		this.fromFraction = fromDuration / duration;
		this.toFraction = toDuration / duration;
	}
	onEnd(container, from, to) {
		this.toTimeline = null;
		this.fromTimeline = null;
		container.remove(from);
		application.purge();
		container.distribute("onDisplayed");
	}
	onStep(fraction) {
		if (fraction < this.fromFraction) {
			this.fromContent.visible = true
			this.fromTimeline.fraction = (this.fromFraction - fraction) / this.fromFraction;
			this.toContent.visible = false
			this.toTimeline.fraction = 0;
		}
		else {
			this.fromContent.visible = false
			this.fromTimeline.fraction = 0;
			this.toContent.visible = true
			this.toTimeline.fraction = (fraction - this.fromFraction) / this.toFraction;
		}
	}
};

class ChannelListTimeline extends Timeline {
	constructor(container, list, pane, direction) {
		super();
		list.skin = pane.skin;
		let content = list.first.first.first;
		const top = container.y - content.height;
		const bottom = container.y + container.height;
		while (content) {
			if (content.y > top)
				break;
			content = content.next;
		}
		while (content) {
			if (content.y > bottom)
				break;
			this.from(content, { x:container.x - content.width }, 100, Math.quadEaseOut, 0);
			content = content.next;
		}
	}
}

class ChannelPaneTimeline extends Timeline {
	constructor(container, pane, list, direction) {
		super();
		const image = pane.first.first;
		const title = image.next.first;
		const description = title.next;
		this.from(image, { x:container.x + container.width }, 100, Math.quadEaseOut, 0);
		this.from(title, { x:container.x + container.width }, 100, Math.quadEaseOut, 0);
		this.from(description, { x:container.x + container.width }, 100, Math.quadEaseOut, 0);
	}
}

if ((config.rotation == 0) || (config.rotation == 180)) {
	paneTitleStyle = new Style({ font:"bold 20px FiraSansCondensed", color:WHITE, left:10, right:10 });
	paneDescriptionStyle = new Style({ font:"18px FiraSansCondensed", color:WHITE, left:10, right:10 });
	ChannelPane = VerticalChannelPane;
}
else {
	paneTitleStyle = new Style({ font:"bold 20px FiraSansCondensed", color:WHITE, horizontal:"left", right:10 });
	paneDescriptionStyle = new Style({ font:"18px FiraSansCondensed", color:WHITE, horizontal:"left", right:10 });
	ChannelPane = HorizontalChannelPane;
}

const SomaFMApplication = Application.template($ => ({
	skin:backgroundSkin, Behavior:ApplicationBehavior,
	contents: [
		Content($, {}),
		Container($, {
			left:0, right:0, top:0, height:40, skin:headerSkin,
			contents: [
				Content($, { anchor:"NOW_PLAYING", left:0, top:0, skin:iconsSkin, variant:7, visible:false, Behavior:NowPlayingBehavior }),
				Label($, { left:40, right:40, top:-5, style:logoStyle, string:"soma fm" }),
				Content($, { anchor:"PLAY_PAUSE", right:0, top:0, skin:iconsSkin, variant:4, visible:false, Behavior:PlayPauseBehavior }),
			]
		}),
		Container($, {
			left:0, right:0, height:40, bottom:0, skin:footerSkin, active:true, Behavior:VolumeBehavior,
			contents: [
				Content($, { left:0, top:0, skin:volumeSkin, variant:0 }),
				Container($, {
					left:40, right:40, top:0, bottom:0, skin:sliderSkin,
					contents:[
						Content($, { left:0, width:20, top:0, skin:sliderSkin, state:1 }),
					]
				}),
				Content($, { right:0, top:0, skin:volumeSkin, variant:1 }),
			]
		}),
	]
}));

export default new SomaFMApplication(model, { commandListLength:1536, displayListLength:4192, touchCount:1, pixels: 240 * 16 });

