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

import AudioIn from "embedded:io/audio/in";
import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

function computeLevel(buffer) { return native("xs_computeLevel").call(this, buffer); };

class LevelsBehavior extends View.Behavior {
	onCreate(container, view) {
		super.onCreate(container, view);
		this.inputBufferSize = 512 * 1024;
		this.inputBuffer = new SharedArrayBuffer(this.inputBufferSize);
		this.input = new AudioIn({
			onReadable: (size) => {
				const samples = new Uint8Array(this.inputBuffer, 0, size);
				this.input.read(samples);
				const level = computeLevel(samples);
				if (this.level != level) {
					this.level = level;
					container.distribute("onInputLevelChanged", level);
				}
			}
		});
		this.level = 0;
	}
	onDisplaying(container) {
		this.input.start(); 
	}
	onUndisplaying(container) {
		this.input?.close();
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
        const levelSkin = this.data.levelSkin;
        const limit = this.limit;
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
    onInputLevelChanged(port, level) {
    	const width = port.width;
		let silence = 8;
		
		let fraction = Math.max(0, Math.log2(level) - silence) / (15 - silence);
	   	let limit = 10 * Math.round(fraction * 10);
    	trace(`${ level } ${ limit }\n`);
    	
//     	let limit = 10 * Math.idiv(level - 1000, 3000);
    	if (limit > width)
    		limit = width;
    	if (this.limit != limit) {
    		this.limit = limit;
    		port.invalidate();
    	}
    }
}
const LevelsContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:LevelsBehavior,
	contents: [
		$.constructor.StarsContainer($, {}),
		Row($, {
			left:screen.corner, width:screen.width - (screen.corner << 1), top:0, height:50, skin:assets.skins.homeTitle,
			contents: [
				Content($, { width:50, height:50, skin:assets.skins.back, active:true, Behavior:View.BackButtonBehavior }),
				Text($, { left:0, right:0, style:assets.styles.homeTitle, string:"Levels" }),
			]
		}),
		Container($, {
			left:0, width:screen.width, top:0, bottom:0, 
			contents: [
				Content($, { bottom:60, skin:assets.skins.microphone }),
				Port($, { anchor:"LEVEL", width:100, height:30, bottom:20, Behavior:LevelPortBehavior }),
			]
		}),
	],
}));

class LevelsTimeline extends Timeline {
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
	constructor() {
		super();
		this.levelSkin = new Skin({ texture:assets.textures.level, x:0, y:0, width:10, height:30, color:[assets.colors.GRAY,assets.colors.WHITE] });

	}
	get Template() { return LevelsContainer }
	get Timeline() { return LevelsTimeline }
};
