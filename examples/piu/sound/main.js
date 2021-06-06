/*
 * Copyright (c) 2021 Moddable Tech, Inc.
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

import {} from "piu/MC";
import Sound from "piu/Sound"

const WHITE = "white";

const backgroundSkin = new Skin({ fill: WHITE });
const buttonSkin = new Skin({ fill: ["#0033cc", "#668cff"] });
const volumeBarSkin = new Skin({ fill: ["#efefef", "#202020"] });

const volumeDownTexture = new Texture({ path: "volume-down.png" });
const volumeDownSkin = new Skin({
	texture: volumeDownTexture, color: WHITE,
	height: 40, width: 40
});

const volumeUpTexture = new Texture({ path: "volume-up.png" });
const volumeUpSkin = new Skin({
	texture: volumeUpTexture, color: WHITE,
	height: 55, width: 55
});

const playTexture = new Texture({ path: "play.png" });
const playSkin = new Skin({
	texture: playTexture, color: WHITE,
	height: 55, width: 55
});

class ButtonBehavior extends Behavior {
	onCreate(button, data) {
		this.data = data;
	}
	onTouchBegan(button) {
		button.state = 1;
	}
	onTouchEnded(button) {
		button.state = 0;
	}
}

class SoundAppBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
		Sound.volume = 0.5;
	}
	onDisplaying(application) {
		let data = this.data;
		if (undefined !== global.button) { // M5Stack
			let button = global.button;
			button.a.onChanged = function() {
				if (this.read()) {
					application.delegate("volumeDown");
				}
			}
			button.b.onChanged = function() {
				if (this.read()) {
					application.delegate("volumeUp");
				}
			}
			button.c.onChanged = function() {
				if (this.read()) {
					application.delegate("playSound");
				}
			}
		}
		
		const Host = global.Host;
		if (undefined !== Host?.Button?.C && undefined !== Host?.Button?.E && undefined !== Host?.Button?.F) { // Kaluga buttons
			new Host.Button.C({
				onPush(value){
					if (value) application.delegate("playSound");
				}
			})
			new Host.Button.E({
				onPush(value){
					if (value) application.delegate("volumeDown");
				}
			})
			new Host.Button.F({
				onPush(value){
					if (value) application.delegate("volumeUp");
				}
			})
		}
		if (undefined !== Host?.Touchpad?.B && undefined !== Host?.Touchpad?.E && undefined !== Host?.Touchpad?.F ) { // Kaluga touchpad
			new Host.Touchpad.B({
				onPush(value){
					if (value) application.delegate("playSound");
				}
			})
			new Host.Touchpad.E({
				onPush(value){
					if (value) application.delegate("volumeUp");
				}
			})
			new Host.Touchpad.F({
				onPush(value){
					if (value) application.delegate("volumeDown");
				}
			})
		}
	}
	volumeDown(application) {
		if (Sound.volume >= 0.1) {
			Sound.volume -= 0.1;
		} else {
			Sound.volume = 0;
		}
		this.data["VOLUME"].delegate("onVolumeChanged", Sound.volume);
	}
	volumeUp(application) {
		if (Sound.volume < 1) {
			Sound.volume += 0.1;
			this.data["VOLUME"].delegate("onVolumeChanged", Sound.volume);
		}
	}
	playSound(application) {
		this.data.sound.play();
	}
}

const VolumeBar = Content.template($ => ({
	top: 0, bottom: 0, left: 0, right: 3, skin: volumeBarSkin
}));

const SoundApp = Application.template($ => ({
	skin: backgroundSkin,
	contents: [
		Row($, {
			anchor: "VOLUME", top: 10, height: 50, left: 20, right: 23,
			contents: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9].map(item => new VolumeBar($, { state: (item < 5)? 1 : 0 })),
			Behavior: class extends Behavior {
				onCreate(row, data) {
					this.data = data;
				}
				onVolumeChanged(row, volume) {
					if (volume) {
						volume *= 10;
						volume = Math.round(volume) - 1;
						row.content(volume).state = 1;
						if (row.content(volume).next) row.content(volume).next.state = 0;
					} else {
						row.first.state = 0;
					}
				}
			}
		}),
		Container($, {
			active: true, height: 70, bottom: 0, left: 28, width: 70, skin: buttonSkin,
			contents: [
				Content($, { skin: volumeDownSkin })
			],
			Behavior: class extends ButtonBehavior {
				onTouchEnded(button) {
					super.onTouchEnded(button);
					application.delegate("volumeDown");
				}
			}
		}),
		Container($, {
			active: true, height: 70, bottom: 0, left: 125, width: 70, skin: buttonSkin,
			contents: [
				Content($, { skin: volumeUpSkin })
			],
			Behavior: class extends ButtonBehavior {
				onTouchEnded(button) {
					super.onTouchEnded(button);
					application.delegate("volumeUp");
				}
			}
		}),
		Container($, {
			active: true, height: 70, bottom: 0, right: 28, width: 70, skin: buttonSkin,
			contents: [
				Content($, { skin: playSkin })
			],
			Behavior: class extends ButtonBehavior {
				onTouchEnded(button) {
					super.onTouchEnded(button);
					application.delegate("playSound");
				}
			}
		}),
	],
	Behavior: SoundAppBehavior
}));

export default new SoundApp({ sound: new Sound({ path: "bflatmajor.wav" }) }, { displayListLength: 1024, touchCount: 1 });

