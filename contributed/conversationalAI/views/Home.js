/*
 * Copyright (c) 2024-2025 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

import ChatAudioIO from "ChatAudioIO";
const halfPI = Math.PI / 2;
const twoPI = 2 * Math.PI;

const interval = 20;

class HomeBehavior extends View.Behavior {
	onCreate(container, view) {
		super.onCreate(container, view);
		this.chat = new ChatAudioIO({
			specifier: view.specifier,
			instructions: view.instructions,
			voiceID: view.voiceID,
			providerID: view.providerID,
			modelID: view.modelID,
			onStateChanged: (state) => {
				this.onStateChanged(container, state);
			},
			onInputLevelChanged: level => {
				container.distribute("onInputLevelChanged", level);
				if (this.silence) {
					if (level > 1000) {
						this.view.SPEAKING.start();
					}
				}
			},
			onOutputLevelChanged: level => {
				container.distribute("onOutputLevelChanged", level);
			},
			onInputTranscript: (text, more) => {
				this.onInputTranscript(container, text, more);
			},
			onOutputTranscript: (text, more) => {
				this.onOutputTranscript(container, text, more);
			},
		})
		this.closing = false;
		this.silence = false;
		if (view.transcript) {
			this.leftData = null;
			this.rightData = null;
			this.microphoneRow = new MicrophoneRow(view);
		}
	}
	onChangeMicrophone(container, microphone) {
		this.chat.changeMicrophone(microphone);
	}
	onConnect(container) {
		this.chat.connect();
	}
	onClose(container) {
		const state = this.chat.state;
		if ((state != ChatAudioIO.FAILED) && (state != ChatAudioIO.DISCONNECTED)) {
			this.closing = true;
			this.chat.disconnect();
		}
		else {
			this.chat.close();
			controller.goBack();
		}
	}
	onDisconnect(container) {
		this.chat.disconnect();
	}
	onDisplaying(container) {
		const view = this.view;
		if (view.transcript)
			this.onTranscript(container, LeftBubbleRow, this.leftData, view.prompt);
	}
	onDisplayed(container) {
		if (this.view.service.key)
			this.chat.connect();
		else
			this.chat.failed({ string:`${ this.view.service.title }: no API key!` });
	}
	onFinished(container) {
		const state = this.chat.state;
		if ((state != ChatAudioIO.CONNECTING) && (state != ChatAudioIO.DISCONNECTING))
			this.onStateChanged(container, state);
	}
	onInputTranscript(container, text, more) {
		this.rightData = this.onTranscript(container, RightBubbleRow, this.rightData, text, more)
	}
	onOutputTranscript(container, text, more) {
		this.leftData = this.onTranscript(container, LeftBubbleRow, this.leftData, text, more)
	}
	onStateChanged(container, state) {
		if (container.running)
			return;
		const view = this.view;
		const listen = view.LISTEN;
		const speak = view.SPEAKING;
		const wait = view.WAIT;
		let content = container.last.first;
		while (content) {
			content.visible = false;
			content = content.next;
		}
		if (!view.transcript) {
			listen.stop();
			if (speak.running) {
				speak.time = speak.duration;
			}
		}
		wait.stop();
		switch (state) {
		case ChatAudioIO.DISCONNECTED:
			view.DISCONNECTED.visible = true;
			if (this.closing) {
				view.DISCONNECTED_BUTTON.visible = false;
				this.chat.close();
				controller.goBack();
			}
			else
				view.DISCONNECTED_BUTTON.visible = true;
			break;
		case ChatAudioIO.FAILED:
			view.FAILED.visible = true;
			view.FAILED_TEXT.string = this.chat.error;
			break;
		case ChatAudioIO.CONNECTING:
		case ChatAudioIO.DISCONNECTING:
			view.WAITING.visible = true;
			view.WAITING_LABEL.string = (state == ChatAudioIO.CONNECTING) ? "Connecting..." : "Disconnecting...";
			container.duration = wait.duration - 1000;
			container.time = 0;
			container.start();
			wait.time = 1000;
			wait.start();
			break;
		case ChatAudioIO.SPEAKING:
			if (view.transcript) {
				view.TRANSCRIPT.visible = true;
				view.TRANSCRIPT_COLUMN.add(this.microphoneRow);
				view.TRANSCRIPT_COLUMN.container.scrollTo(0, 0x7fff);
			}
			else {
				this.silence = view.SPEAKING.first.visible;
				view.SPEAKING.visible = true;
			}
			break;
		case ChatAudioIO.LISTENING:
			if (view.transcript) {
				view.TRANSCRIPT.visible = true;
				view.TRANSCRIPT_COLUMN.remove(this.microphoneRow);
			}
			else {
				view.LISTENING.visible = true;
				listen.start();
			}
			break;
		}
	}
	onTranscript(container, Template, data, text, more) {
		const view = this.view;
		const column = view.TRANSCRIPT_COLUMN;
		if (column) {
			if (data) {
				data.string = data.string.slice(0, -3);
				data.string += text;
				if (more)
					data.string += "...";
				if (data.string.length)
					data.ROW.behavior.onUpdate(data.ROW);
				else
					column.remove(data.ROW);
			}
			else {
				data = { string:text, skin:this.view.skins.bubble, style:this.view.styles.bubble };
				if (more)
					data.string += "...";
				if (data.string.length)
					column.add(new Template(data));
			}
		}
		if (more)
			return data;
		return null;
	}
}

class BackButtonBehavior extends View.ButtonBehavior {
	onTap(content) {
		content.active = false;
		content.container.container.last.bubble("onClose");
	}
}

class ConnectButtonBehavior extends View.ButtonBehavior {
	onTap(container) {
		container.bubble("onConnect");
		this.changeState(container, 0);
	}
}

class DisconnectButtonBehavior extends View.ButtonBehavior {
	onTap(container) {
		container.bubble("onDisconnect");
		this.changeState(container, 0);
	}
}

class LevelPortBehavior extends Behavior {
	onCreate(port, data, it) {
		this.data = data;
		this.direction = it.direction;
		this.limit = (this.direction < 0) ? port.width : 0;
    }
    onDraw(port) {
    	const width = port.width;
        const levelSkin = this.data.skins.level;
        const limit = this.limit;
        if (this.direction > 0) {
			let x = 0;
			while (x < limit) {
				 port.drawSkin(levelSkin, x, 0, 10, 30, 0, 1);
				 x += 10;
			}
			while (x < width) {
				 port.drawSkin(levelSkin, x, 0, 10, 30, 0, 0);
				 x += 10;
			}
        }
        else {
			let x = width - 10;
			while (x >= limit) {
				 port.drawSkin(levelSkin, x, 0, 10, 30, 0, 1);
				 x -= 10;
			}
			while (x >= 0) {
				 port.drawSkin(levelSkin, x, 0, 10, 30, 0, 0);
				 x -= 10;
			}
        }
    }
    onInputLevelChanged(port, level) {
    	const width = port.width;
//     	let limit = 10 * Math.idiv(level - 1000, 3000);

		let silence = 8;
		let fraction = Math.max(0, Math.log2(level) - silence) / (15 - silence);
	   	let limit = 10 * Math.round(fraction * 10);
	   	
   		if (limit > width)
    		limit = width;
        if (this.direction < 0)
        	limit = width - limit
    	if (this.limit != limit) {
    		this.limit = limit;
    		port.invalidate();
    	}
    }
}
    
class MicrophoneButtonBehavior extends View.ButtonBehavior {
	onTap(content) {
		if (content.variant == 0) {
			content.variant = 1;
			content.bubble("onChangeMicrophone", 0);
		}
		else {
			content.variant = 0;
			content.bubble("onChangeMicrophone", 1);
		}
		this.changeState(content, 0);
	}
}


class ListenContentBehavior extends Behavior {
	onCreate(content, data) {
		content.duration = 2000;
		content.interval = interval;
		this.variant = content.variant = 9;
    }
    onChange(content, variant) {
			content.stop();
			this.variant = content.variant = variant;
			content.time = 0;
			content.start();
    }
    onTimeChanged(content) {
    	const variant = this.variant;
    	content.variant = variant + Math.round((9 - variant) * content.fraction)
    }
}

class ListenContainerBehavior extends Behavior {
	onCreate(content, data) {
		content.duration = 4000;
		content.interval = interval;
		this.variant = 9;
    }
	onDisplaying(container) {
		this.cx = container.x + (container.width >> 1);
		this.cy = container.y + (container.height >> 1);
		this.radius = 80;
    	this.onTimeChanged(container);
	}
	onDisplayed(container) {
    	this.onFinished(container);
	}
	onFinished(container) {
		container.time = 0;
		container.start();
	}
    onTimeChanged(container) {
    	const { cx, cy, radius, variant } = this;
    	const fraction = container.fraction;
    	const length = container.length;
    	let which = Math.round((1 - fraction) * (length - 1));
    	let index = 0;
		let angle = (twoPI * fraction) - halfPI;
		let delta = twoPI / length;
    	let content = container.first;
    	while (content) {
			content.x = cx + Math.round(radius * Math.cos(angle)) - 20;
			content.y = cy + Math.round(radius * Math.sin(angle)) - 20;
			if (index == which)
    			content.behavior.onChange(content, variant);
			index++;
			angle += delta
    		content = content.next;
    	}
    }
    onOutputLevelChanged(container, level) {
    	level = 10 * level / 32768;
    	if (level > 1)
    		level = 1;
    	this.variant = Math.round(9 * (1 - level));
    }
}

class SpeakContainerBehavior extends Behavior {
	onDisplaying(container) {
		const timeline = new Timeline();
		const bubble = container.first;
		const microphone = bubble.next;
		const level = microphone.next;
		const y = container.y + (container.height - (microphone.height + 10 + level.height)) >> 1;
		const duration = 250;
		timeline.to(bubble, { y: container.y - bubble.height }, duration, Math.quadEaseOut, 0);
		timeline.to(microphone, { y }, duration, Math.quadEaseOut, -(duration >> 1));
		timeline.to(level, { y: y + microphone.height + 10 }, duration, Math.quadEaseOut, -(duration >> 1));
		this.timeline = timeline;
		container.duration = timeline.duration;
    }
    onFinished(container) {
		container.first.visible = false;
    }
    onTimeChanged(container) {
		this.timeline.fraction = container.fraction;
    }
}    

class TranscriptRowBehavior extends Behavior {
	onCreate(row, data) {
		this.data = data;
    }
	onDisplaying(row) {
		this.onUpdate(row);
    }
	onUpdate(row) {
		const data = this.data;
		const size = data.style.measure(data.string);
		let width = size.width + 36;
		if (width > 208)
			width = 208;
		else if (width < 48)
			width = 48;
		row.width = width;	
		data.TEXT.string = data.string;
		row.container.container.scrollTo(0, 0x7fff);
    }
}

class WaitContainerBehavior extends Behavior {
	onCreate(container, view) {
		this.timeline = new Timeline();
	}
	onDisplaying(container) {
		this.cx = container.x + (container.width >> 1);
		this.cy = container.y + (container.height >> 1);
		let content = container.first;
		let delay = 1000;
		while (content) {
			this.timeline.on(content.behavior, { angle:[- halfPI, twoPI - halfPI] }, 1500, Math.quadEaseInOut, delay); 
			content = content.next;
			delay = -1400;
		}
		container.duration = this.timeline.duration;
		container.interval = interval;
	}
	onFinished(container) {
		container.time = 0;
		container.start();
	}
	onTimeChanged(container) {
		this.timeline.seekTo(container.time);
		let content = container.first;
		while (content) {
			const behavior = content.behavior;
			const angle = behavior.angle;
			const radius = behavior.radius;
			content.x = this.cx + Math.round(radius * Math.cos(angle)) - behavior.dx;
			content.y = this.cy + Math.round(radius * Math.sin(angle)) - behavior.dy;
			content = content.next;
		}
	}
}

class WaitContentBehavior extends Behavior {
	onCreate(content) {
		this.angle = 0;
		this.radius = 0;
	}
	onDisplaying(content) {
		this.angle = 0;
		this.radius = ((content.container.height - content.height) >> 1) - content.coordinates.top;
		this.dx = content.width >> 1;
		this.dy = content.height >> 1;
	}
}

const DisconnectedContainer = Container.template($ => ({
	anchor:"DISCONNECTED", visible:true, left:0, right:0, top:50, bottom:50,
	contents: [
		Content($, { skin:$.service.icon }),
		Container($, {
			anchor:"DISCONNECTED_BUTTON", visible:false, width:120, height:50, bottom:-50, skin:$.skins.button, active:true, Behavior:ConnectButtonBehavior,
			contents: [
				Label($, { string:"Connect" }),
			],
		}),
	],
}));

const FailedContainer = Container.template($ => ({
	anchor:"FAILED", visible:false, left:0, right:0, top:50, bottom:50,
	contents: [
		Scroller($, { 
			left:0, right:0, top:0, bottom:0, skin:assets.skins.error, clip:true, active:true, backgroundTouch:true, Behavior:View.VerticalScrollerBehavior,
			contents: [
				Text($, { anchor:"FAILED_TEXT", left:10, right:10, style:assets.styles.error }),
			],
		}),
		Container($, {
			width:120, height:50, bottom:-50, skin:$.skins.button, active:true, Behavior:ConnectButtonBehavior,
			contents: [
				Label($, { string:"Connect" }),
			],
		}),
	],
}));

const ListeningContainer = Container.template($ => ({
	anchor:"LISTENING", visible:false, left:0, right:0, top:50, bottom:50,
	contents: [
		Container($, { 
			anchor:"LISTEN", width:140, height:140, Behavior:ListenContainerBehavior,
			contents: [
				Content($, { left:0, top:0, skin:$.skins.wait, Behavior:ListenContentBehavior }),
				Content($, { left:0, top:0, skin:$.skins.wait, Behavior:ListenContentBehavior }),
				Content($, { left:0, top:0, skin:$.skins.wait, Behavior:ListenContentBehavior }),
				Content($, { left:0, top:0, skin:$.skins.wait, Behavior:ListenContentBehavior }),
				Content($, { left:0, top:0, skin:$.skins.wait, Behavior:ListenContentBehavior }),
				Content($, { left:0, top:0, skin:$.skins.wait, Behavior:ListenContentBehavior }),
				Content($, { left:0, top:0, skin:$.skins.wait, Behavior:ListenContentBehavior }),
				Content($, { left:0, top:0, skin:$.skins.wait, Behavior:ListenContentBehavior }),
			]
		}),
		Content($, { skin:$.service.icon }),
	],
}));

const SpeakingContainer = Container.template($ => ({
	anchor:"SPEAKING", visible:false, left:0, right:0, top:50, bottom:0, clip:true, Behavior:SpeakContainerBehavior,
	contents: [
		Container($, {
			left:0, width:208, top:0, clip:true,
			contents: [
				Content($, { left:0, right:0, top:2, bottom:2, skin:$.skins.bubble } ), 
				Column($, { left:18, right:12, clip:true,
					contents:[
						Content($, { height:10 }),
						Text($, { anchor:"TEXT", left:0, right:0, top:0, style:$.styles.bubble, string:$.prompt }),
						Content($, { height:10 }),
					]
				}),
			],
		}),
		Content($, { bottom:60, skin:assets.skins.microphone, active:true, Behavior:MicrophoneButtonBehavior }),
		Port($, { anchor:"LEVEL", width:100, height:30, bottom:20, Behavior:LevelPortBehavior, direction:1 }),
	],
}));

const TranscriptContainer = Container.template($ => ({
	anchor:"TRANSCRIPT", visible:false, left:0, right:0, top:50, bottom:0,
	contents: [
		Scroller($, {
			left:0, width:240, top:0, bottom:0, clip:true, active:true, backgroundTouch:true, Behavior:View.VerticalScrollerBehavior,
			contents: [
				Column($, { 
					anchor:"TRANSCRIPT_COLUMN", left:0, right:0, top:0, 
					contents: [
					],
				}),
				View.VerticalScrollbar($, {}),
			]
		}),
	],
}));

const LeftBubbleRow = Container.template($ => ({
	anchor:"ROW", left:0, width:208, clip:true, Behavior: TranscriptRowBehavior,
	contents: [
		Content($, { left:0, right:0, top:2, bottom:2, skin:$.skin } ), 
		Column($, { left:18, right:12, clip:true,
			contents:[
				Content($, { height:10 }),
				Text($, { anchor:"TEXT", left:0, right:0, top:0, style:$.style }),
				Content($, { height:10 }),
			]
		}),
	],
}));

const RightBubbleRow = Layout.template($ => ({
	anchor:"ROW", right:20, width:208, clip:true, Behavior: TranscriptRowBehavior,
	contents: [
		Content($, { left:0, right:0, top:2, bottom:2, skin:$.skin, state:1, variant:1 } ), 
		Column($, { left:12, right:18, clip:true,
			contents:[
				Content($, { height:10 }),
				Text($, { anchor:"TEXT", left:0, right:0, top:0, style:$.style, state:1 }),
				Content($, { height:10 }),
			]
		}),
	],
}));

const MicrophoneRow = Layout.template($ => ({
	anchor:"ROW", right:20, width:148, height:40, clip:true,
	contents: [
		Content($, { left:0, right:0, top:2, bottom:2, skin:$.skins.bubble, state:1, variant:1 } ), 
		Port($, { anchor:"LEVEL", left:12, width:100, height:30, Behavior:LevelPortBehavior, direction:-1 }),
		Content($, { right:12, skin:assets.skins.microphoneSmall, active:true, Behavior:MicrophoneButtonBehavior }),
	],
}));

const WaitingContainer = Container.template($ => ({
	anchor:"WAITING", visible:false, left:0, right:0, top:50, bottom:50,
	contents: [
		Container($, { 
			anchor:"WAIT", width:180, height:180, Behavior:WaitContainerBehavior,
			contents: [
				Content($, { left:80, top:0, skin:$.skins.wait, variant:3, Behavior:WaitContentBehavior }),
				Content($, { left:80, top:-2, skin:$.skins.wait, variant:4, Behavior:WaitContentBehavior }),
				Content($, { left:80, top:-4, skin:$.skins.wait, variant:5, Behavior:WaitContentBehavior }),
				Content($, { left:80, top:-6, skin:$.skins.wait, variant:6, Behavior:WaitContentBehavior }),
			]
		}),
		Content($, { skin:$.service.icon }),
		Label($, { anchor:"WAITING_LABEL", left:0, right:0, height:50, bottom:-50 }),
	],
}));

const HomeContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:HomeBehavior,
	contents: [
		$.constructor.StarsContainer($, {}),
		Row($, {
			left:0, width:240, top:0, height:50, skin:assets.skins.homeTitle,
			contents: [
				Content($, { width:50, height:50, skin:assets.skins.back, active:true, Behavior:BackButtonBehavior }),
				Text($, { left:0, right:0, style:assets.styles.homeTitle, string:$.title }),
			]
		}),
		Container($, {
			left:0, width:240, top:0, bottom:0, 
			contents: [
				DisconnectedContainer($, {}),
				FailedContainer($, {}),
				WaitingContainer($, {}),
				$.transcript ? [
					TranscriptContainer($, {}),
				] : [
					SpeakingContainer($, {}),
					ListeningContainer($, {}),
				]
			]
		}),
	],
}));

class HomeTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		let header = screen.first.next;
		let body = screen.last;
		const duration = 250;
		this.from(header, { y:screen.y - header.height }, duration, Math.quadEaseOut, 0);
		this.from(body, { x:screen.x + body.width }, duration, Math.quadEaseOut, -duration);
	}
}

export default class extends View {
	constructor(persona) {
		super();
		const service = this.service = assets.services[persona.service];
		this.instructions = persona.instructions;
		this.prompt = persona.prompt;
		this.specifier = service.specifier;
		this.title = persona.title;
		this.transcript = service.transcript;
		this.voiceID = persona.voiceID;
		this.providerID = persona.providerID;
		this.modelID = persona.modelID;
		this.skins = {
			bubble:  new Skin({ texture: assets.textures.bubble, color:[service.color,assets.colors.WHITE], x:0, y:0, width:204, height:332, variants:204, left:24, right:24, top:12, bottom:12 }),
			button:  new Skin({ texture:assets.textures.button, width:50, height:50, left:15, right:15, color: [assets.colors.GRAY,service.color] }),
			glow:  new Skin({ texture:assets.textures.glow, width:32, height:32, variants:32, color: [assets.colors.TRANSPARENT,service.color] }),
			level: new Skin({ texture:assets.textures.level, x:0, y:0, width:10, height:30, color:this.transcript ? [assets.colors.LITE,assets.colors.DARK] : [assets.colors.GRAY,assets.colors.WHITE] }),
			wait: new Skin({ texture: assets.textures.wait, width:40, height:40, variants:40, color:service.color }),
		};
		this.styles = {
			bubble: new Style({ font:"medium 14px Roboto", color:[assets.colors.WHITE,assets.colors.BLACK], horizontal:"left" })
		};
	}
	get Template() { return HomeContainer }
	get Timeline() { return HomeTimeline }
};
