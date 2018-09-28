/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
import Timeline from "piu/Timeline";

const WHITE = "#FFFFFF";
const BLACK = "#000000";
const GRAY = "#999999";
const DARK_GRAY = "#666666";
const MOZILLA_BLUE = "#4C9ACA";

const WhiteSkin = Skin.template({ fill: WHITE });
const GraySkin = Skin.template({ fill: GRAY });
const DarkGraySkin = Skin.template({ fill: DARK_GRAY });
const MozillaBlueSkin = Skin.template({ fill: MOZILLA_BLUE });

const OpenSans20 = Style.template({ color: WHITE, font: "open-sans-reg-20-degree", horizontal: "center", vertical: "middle" });
const OpenSans36 = Style.template({ color: WHITE, font: "OpenSans-36-numbers", vertical: "middle" });
const OpenSans100 = Style.template({ font: "open-sans-light-num-100", color: ["white", "transparent"] });
const WhiteStyle = Style.template({ color: [WHITE, MOZILLA_BLUE] });
const CenterStyle = Style.template({ horizontal: "center" });

const HeatTexture = Texture.template("flame.png");
const HeatSkin = Skin.template({ 
	Texture: HeatTexture,
	x:0, y:0, width:30, height:38
});
const FanTexture = Texture.template("fan.png");
const FanSkin = Skin.template({ 
	Texture: FanTexture,
	x:0, y:0, width:38, height:38
});
const UpTexture = Texture.template("up-arrow-mask.png");
const UpSkin = Skin.template({ 
	Texture: UpTexture,
	color: WHITE,
	x:0, y:0, width:36, height:18
});
const DownTexture = Texture.template("down-arrow-mask.png");
const DownSkin = Skin.template({ 
	Texture: DownTexture,
	color: WHITE,
	x:0, y:0, width:36, height:18
});

class ArrowBehavior extends Behavior {
	onCreate(arrow, data) {
		this.data = data;
	}
	onTouchBegan(content) {
		this.increment(content);
		content.interval = 400;
		content.time = 0;
		content.start();
	}
	onTouchEnded(content) {
		content.stop();
		application.distribute("updateMaster");
	}
	onTimeChanged(content) {
		if ( content.time > 2000 ) content.interval = 100;
		else if ( content.time > 1000 ) content.interval = 200;
		this.increment(content);
	}
	increment(content) {
		application.distribute("incrementTarget", this.delta);
	}
}

class UnderlineBehavior extends Behavior {
	onCreate(underline, data) {
		this.data = data;
	}
	onDisplaying(underline) {
		this.coolX = underline.x;
		let heatX = this.heatX = underline.previous.x;
		let timeline = this.timeline = new Timeline();
		timeline.to(underline, { x: heatX }, 200, Math.quadEaseOut, 0);
		underline.duration = timeline.duration;
		this.update(underline, 1);
	}
	update(underline, isHeating) {
		if (isHeating) {
			if (underline.x != this.coolX) return;
			this.reverse = false;
		} else {
			if (underline.x != this.heatX) return;
			this.reverse = true;
		}
		underline.time = 0;
		underline.start();
	}
	onTimeChanged(underline) {
		let time = underline.time;
		if (this.reverse) time = underline.duration - time;
		this.timeline.seekTo(time);
	}
}

const ThermostatScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0, Style: OpenSans20,
	contents: [
		Label($, {
			anchor: "HOSTNAME", Skin: GraySkin, top: 0, height: 40, left: 0, right: 0, string: $.hostname,
		}),
		Row($, {
			top: 40, bottom: 0, left: 0, right: 0, Style: OpenSans20,
			contents: [
				Column($, {
					top: 0, bottom: 0, left: 0, width: 200, Skin: MozillaBlueSkin,
					contents: [
						Label($, {
							anchor: "ACTUAL", top: 0, bottom: 0, Style: OpenSans100,
							string: "65°",
						}),
						Container($, {
							left: 0, right: 0, height: 73, Skin: WhiteSkin,
							contents: [
								Content($, { Skin: FanSkin, left: 50, top: 13, width: 40, height: 40,}),
								Content($, { Skin: HeatSkin, right: 50, top: 13, width: 40, height: 40,}),
								Content($, { 
									anchor: "UNDERLINE", width: 48, height: 6, bottom: 10, left: 45, 
									Skin: DarkGraySkin, Behavior: UnderlineBehavior
								})
							]
						})
					]
				}),
				Column($, {
					top: 0, bottom: 0, left: 0, right: 0, Skin: DarkGraySkin, Style: OpenSans20,
					contents: [
						Content($, {
							active: true, top: 35, Skin: UpSkin,
							Behavior: class extends ArrowBehavior {
								onCreate(arrow, data) {
									this.delta = 1;
									super.onCreate(arrow, data);
								}
							}
						}),
						Label($, {
							top: 10, bottom: 0, Style: OpenSans20,
							string: "Target:"
						}),
						Label($, {
							anchor: "TARGET", top: 0, bottom: 10, Style: OpenSans36,
							string: "70°"
						}),
						Content($, {
							active: true, bottom: 35, Skin: DownSkin,
							Behavior: class extends ArrowBehavior {
								onCreate(arrow, data) {
									this.delta = -1;
									super.onCreate(arrow, data);
								}
							}
						}),
					]
				}),
			],
		})
	]
}))
export default {
	MozillaBlueSkin,
	OpenSans20,
	ThermostatScreen,
}


