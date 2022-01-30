/*
 * Copyright (c) 2016-2021 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import {} from "piu/MC";
import config from "mc/config";
import CARDS from "cards";

const WHITE = "white";
const BLACK = "black";
const backgroundSkin = new Skin({ fill: [WHITE, BLACK] });
const openSans35 = new Style({ font: "35px Open Sans", color: [BLACK, WHITE], bottom: 10 });

class AppBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
		this.index = -1;
		this.questions = Object.keys(CARDS);
		const thisBehavior = this;
		new device.peripheral.button.B({
			onPush() {
				if (this.pressed) {
					thisBehavior.showNextCard(application, -1);
				}
			}
		});
		new device.peripheral.button.C({
			onPush() {
				if (this.pressed) {
					thisBehavior.showNextCard(application, 1);
				}
			}
		});
	}
	onDisplaying(application) {
		screen.refresh?.();
		screen.configure?.({updateMode: config.firstDrawMode ?? config.updateMode});
		if (config.firstDrawMode)
			application.defer("onFinishedFirstDraw", config.updateMode);
		this.showNextCard(application, 1);
	}
	onFinishedFirstDraw(application, mode) {
		screen.configure({updateMode: mode});
	}
	showNextCard(application, delta) {
		let data = this.data;
		let questions = this.questions;
		this.index += delta;
		if (this.index >= questions.length)
			this.index = 0;
		else if (this.index < 0)
			this.index = questions.length-1;
		let question = questions[this.index];
		data["QUESTION"].string = question;
		data["ANSWER"].string = CARDS[question];
	}
	onTouchBegan(application, id, x, y, ticks) {
		this.startX = x;
	}
	onTouchMoved(application, id, x, y, ticks) {
		if (this.startX - x > 150) {
			this.startX = x;
			this.showNextCard(application, 1);
		}
		else if (this.startX - x < -150) {
			this.startX = x;
			this.showNextCard(application, -1);
		}
	}
}

class AnswerBehavior extends Behavior {
	onCreate(row) {
		new device.peripheral.button.A({
			onPush() {
				row.first.visible = this.pressed;
			}
		})
	}
	onTouchBegan(row) {
		row.first.visible = true;
	}
	onTouchEnded(row) {
		row.first.visible = false;
	}
	onTouchCancelled(row) {
		row.first.visible = false;
	}
}

const FlashcardApp = Application.template($ => ({
	displayListLength: 5000,
	left: 0, right: 0, top: 0, bottom: 0,
	contents: [
		Column($, {
			top: 0, bottom: 0, left: 0, right: 0, skin: backgroundSkin, state: 1,
			contents: [
				Row($, {
					top: 0, bottom: 0, left: 0, right: 0, skin: backgroundSkin, state: 0,
					contents: [
						Text($, {
							anchor: "QUESTION", left: 30, right: 30, state: 0, style: openSans35
						})
					]
				}),	
				Row($, {
					top: 0, bottom: 0, left: 0, right: 0, skin: backgroundSkin, state: 1,
					contents: [
						Text($, {
							anchor: "ANSWER", left: 30, right: 30, style: openSans35, state: 1, visible: 0
						})
					],
					active: true, Behavior: AnswerBehavior
				})
			]
		})
	],
	active: true, Behavior: AppBehavior
}));

export default new FlashcardApp({});
