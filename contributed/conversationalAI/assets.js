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

import config from "mc/config"

const BLACK = "black";
const DARK = "#404040";
const GRAY = "#808080";
const LITE = "#C0C0C0";
const WHITE = "white";
const TRANSPARENT = "transparent";

const assets = {
	colors: {
		BLACK,
		DARK,
		GRAY,
		LITE,
		WHITE,
		TRANSPARENT,
	},
	skins: {
		screen: { fill:"#011430" },
		
		logo: { texture:{ path: "mod-logo.png" }, width:144, height:56, color:WHITE },
		microphone: { texture:{ path: "microphone.png" }, width:100, height:100, variants:100, color:[WHITE,GRAY] },
		microphoneSmall: { texture:{ path: "microphone-small.png" }, width:24, height:24, variants:24, color:[BLACK,GRAY] },
		scrollbarY: { texture:{ path: "scrollbarY.png" }, width:20, height:60, top:10, bottom:10, color:GRAY },
		star2: { texture:{ path: "star-4x4.png" }, width:4, height:4, variants:4 },
		star3: { texture:{ path: "star-5x5.png" }, width:5, height:5, variants:5 },
		star6: { texture:{ path: "star-8x8.png" }, width:8, height:8, variants:8 },
		star9: { texture:{ path: "star-11x11.png" }, width:11, height:11, variants:11 },
		star2_2: { texture:{ path: "star2-4x4.png" }, width:4, height:4, variants:4 },
		star2_3: { texture:{ path: "star2-5x5.png" }, width:5, height:5, variants:5 },
		star2_6: { texture:{ path: "star2-8x8.png" }, width:8, height:8, variants:8 },
		star2_9: { texture:{ path: "star2-11x11.png" }, width:11, height:11, variants:11 },

		personaRow: { fill:[WHITE,LITE], stroke:LITE, bottom:1 },
		homeTitle: { stroke:GRAY, bottom:1, }, 
		
		back: { texture:{ path: "back-arrow-mask.png" }, width:12, height:20, color:[WHITE,LITE] },
		
		error: { fill:WHITE },
	},
	styles: {
		screen: { font:"18px Roboto", color:WHITE },
		
		personaTitle: { font:"medium 16px Roboto", color:BLACK, horizontal:"left" },
		personaSubtitle: { font:"light 14px Roboto", color:BLACK, horizontal:"left" },
		
		homeTitle: { font:"medium 16px Roboto", color:WHITE, horizontal:"left" },
		homePrompt: { font:"medium 14px Roboto", color:WHITE, horizontal:"left" },
	
		error: { font:"light 14px Roboto", color:BLACK, horizontal:"left" },
	},
	textures: {
		bubble: { path: "bubble.png" },
		button: { path: "button.png" },
		glow: { path: "glow.png" },
		level: { path: "level.png" },
		wait: { path: "wait.png" },
	},
	
	services: {
		gemini: {
			color: "#4e87d3",
			icon:  { texture:{ path: "gemini-logo.png" }, width:64, height:64 },
			iconSmall: { texture:{ path: "gemini-logo-small.png" }, width:24, height:24 },
			key: config.geminiAPIKey,
			title: "Google Gemini Live",
			specifier: "googleGeminiLive",
			transcript: false,
		},
		hume: {
			color: "#9a8dea",
			icon:  { texture:{ path: "hume-logo.png" }, width:64, height:64 },
			iconSmall: { texture:{ path: "hume-logo-small.png" }, width:24, height:24 },
			key: config.humeAIKey,
			title: "Hume AI EVI",
			specifier: "humeAIEVI",
			transcript: true,
		},
		openai: {
			color: "#00afba",
			icon:  { texture:{ path: "openai-logo.png" }, width:64, height:64 },
			iconSmall: { texture:{ path: "openai-logo-small.png" }, width:24, height:24 },
			key: config.openAIKey,
			title: "OpenAI Realtime",
			specifier: "openAIRealtime",
			transcript: true,
		}
	}
};

export default Object.freeze(assets, true);
