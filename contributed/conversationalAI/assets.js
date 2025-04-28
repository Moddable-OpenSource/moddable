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
		check: { texture:{ path: "check-mask.png" }, width:24, height:20, color:DARK },
		forward: { texture:{ path: "forward-arrow-mask.png" }, width:12, height:20, color:GRAY },
		gear: { texture:{ path: "gear-mask.png" }, width:23, height:23, color:[GRAY,BLACK] },
		
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
		elevenLabs: {
			color: "#959595",
			icon:  { texture:{ path: "elevenLabs-logo.png" }, width:64, height:64 },
			iconSmall: { texture:{ path: "elevenLabs-logo-small.png" }, width:24, height:24 },
			key: config.elevenLabsKey,
			title: "ElevenLabs Agent",
			specifier: "elevenLabsAgent",
			transcript: true,
			voices: [
				{ name:"Adam", gender:"Male", age:"Middle aged", accent:"American", usage:"Narration", description:"Deep" },
				{ name:"Alice", gender:"Female", age:"Middle aged", accent:"British", usage:"News", description:"Confident" },
				{ name:"Antoni", gender:"Male", age:"Young", accent:"American", usage:"Narration", description:"Well-rounded" },
				{ name:"Aria", gender:"Female", age:"Middle aged", accent:"American", usage:"Social media", description:"Expressive" },
				{ name:"Arnold", gender:"Male", age:"Middle aged", accent:"American", usage:"Narration", description:"Crisp" },
				{ name:"Arnold", gender:"Male", age:"Middle aged", accent:"American", usage:"Narration", description:"Crisp" },
				{ name:"Bill", gender:"Male", age:"Old", accent:"American", usage:"Narration", description:"Trustworthy" },
				{ name:"Brian", gender:"Male", age:"Middle aged", accent:"American", usage:"Narration", description:"Deep" },
				{ name:"Callum", gender:"Male", age:"Middle aged", accent:"American", usage:"Characters", description:"Intense" },
				{ name:"Charlie", gender:"Male", age:"Middle aged", accent:"Australian", usage:"Conversational", description:"Natural" },
				{ name:"Charlotte", gender:"Female", age:"Young", accent:"Swedish", usage:"Characters", description:"Seductive" },
				{ name:"Chris", gender:"Male", age:"Middle aged", accent:"American", usage:"Conversational", description:"Casual" },
				{ name:"Clyde", gender:"Male", age:"Middle aged", accent:"American", usage:"Characters", description:"War veteran" },
				{ name:"Daniel", gender:"Male", age:"Middle aged", accent:"British", usage:"News", description:"Authoritative" },
				{ name:"Dave", gender:"Male", age:"Young", accent:"British", usage:"Characters", description:"Conversational" },
				{ name:"Domi", gender:"Female", age:"Young", accent:"American", usage:"Narration", description:"Strong" },
				{ name:"Dorothy", gender:"Female", age:"Young", accent:"British", usage:"Narration", description:"Pleasant" },
				{ name:"Drew", gender:"Male", age:"Middle aged", accent:"American", usage:"News", description:"Well-rounded" },
				{ name:"Elli", gender:"Female", age:"Young", accent:"American", usage:"Narration", description:"Emotional" },
				{ name:"Emily", gender:"Female", age:"Young", accent:"American", usage:"Meditation", description:"Calm" },
				{ name:"Eric", gender:"Male", age:"Middle aged", accent:"American", usage:"Conversational", description:"Friendly" },
				{ name:"Ethan", gender:"Male", age:"Young", accent:"American", usage:"ASMR", description:"Soft" },
				{ name:"Fin", gender:"Male", age:"Old", accent:"Irish", usage:"Characters", description:"Sailor" },
				{ name:"Freya", gender:"Female", age:"Young", accent:"American", usage:"Characters", description:"Expressive" },
				{ name:"George", gender:"Male", age:"Middle aged", accent:"British", usage:"Narration", description:"Warm" },
				{ name:"George", gender:"Male", age:"Middle aged", accent:"British", usage:"Audiobook", description:"" },
				{ name:"Gigi", gender:"Female", age:"Young", accent:"American", usage:"Animation", description:"Childlish" },
				{ name:"Giovanni", gender:"Male", age:"Young", accent:"Italian", usage:"Narration", description:"Foreigner" },
				{ name:"Glinda", gender:"Female", age:"Middle aged", accent:"American", usage:"Characters", description:"Witch" },
				{ name:"Grace", gender:"Female", age:"Young", accent:"Us-southern", usage:"Narration", description:"Pleasant" },
				{ name:"Harry", gender:"Male", age:"Young", accent:"American", usage:"Characters", description:"Anxious" },
				{ name:"James", gender:"Male", age:"Old", accent:"Australian", usage:"News", description:"Calm" },
				{ name:"Jeremy", gender:"Male", age:"Young", accent:"Irish", usage:"Narration", description:"Excited" },
				{ name:"Jessica", gender:"Female", age:"Young", accent:"American", usage:"Conversational", description:"Expressive" },
				{ name:"Jessie", gender:"Male", age:"Old", accent:"American", usage:"Characters", description:"Raspy" },
				{ name:"Joseph", gender:"Male", age:"Middle aged", accent:"British", usage:"News", description:"Articulate" },
				{ name:"Josh", gender:"Male", age:"Young", accent:"American", usage:"Narration", description:"Deep" },
				{ name:"Laura", gender:"Female", age:"Young", accent:"American", usage:"Social media", description:"Upbeat" },
				{ name:"Liam", gender:"Male", age:"Young", accent:"American", usage:"Narration", description:"Articulate" },
				{ name:"Lily", gender:"Female", age:"Middle aged", accent:"British", usage:"Narration", description:"Warm" },
				{ name:"Matilda", gender:"Female", age:"Middle aged", accent:"American", usage:"Narration", description:"Friendly" },
				{ name:"Michael", gender:"Male", age:"Old", accent:"American", usage:"Narration", description:"Calm" },
				{ name:"Mimi", gender:"Female", age:"Young", accent:"Swedish", usage:"Animation", description:"Childish" },
				{ name:"Nicole", gender:"Female", age:"Young", accent:"American", usage:"ASMR", description:"Soft" },
				{ name:"Patrick", gender:"Male", age:"Middle aged", accent:"American", usage:"Characters", description:"Shouty" },
				{ name:"Paul", gender:"Male", age:"Middle aged", accent:"American", usage:"News", description:"Authoritative" },
				{ name:"Rachel", gender:"Female", age:"Young", accent:"American", usage:"Narration", description:"Calm" },
				{ name:"River", gender:"Non-binary", age:"Middle aged", accent:"American", usage:"Social media", description:"Confident" },
				{ name:"Roger", gender:"Male", age:"Middle aged", accent:"American", usage:"Social media", description:"Confident" },
				{ name:"Sam", gender:"Male", age:"Young", accent:"American", usage:"Narration", description:"Raspy" },
				{ name:"Santa Claus", gender:"Male", age:"Old", accent:"American", usage:"Characters", description:"Cheerful" },
				{ name:"Sarah", gender:"Female", age:"Young", accent:"American", usage:"News", description:"Soft" },
				{ name:"Serena", gender:"Female", age:"Middle aged", accent:"American", usage:"Narration", description:"Pleasant" },
				{ name:"Thomas", gender:"Male", age:"Young", accent:"American", usage:"Meditation", description:"Calm" },
				{ name:"Will", gender:"Male", age:"Young", accent:"American", usage:"Social media", description:"Friendly" },
			],
			defaultVoiceName: "Rachel"
		},
		gemini: {
			color: "#4e87d3",
			icon:  { texture:{ path: "gemini-logo.png" }, width:64, height:64 },
			iconSmall: { texture:{ path: "gemini-logo-small.png" }, width:24, height:24 },
			key: config.geminiAPIKey,
			title: "Google Gemini Live",
			specifier: "googleGeminiLive",
			transcript: false,
			voices: [
				{ name:"aoede", labels:[ "Female" ] },
				{ name:"charon", labels:[ "Male" ] },
				{ name:"fenrir", labels:[ "Male" ] },
				{ name:"kore", labels:[ "Female" ] },
				{ name:"leda", labels:[ "Female" ] },
				{ name:"orus", labels:[ "Male" ] },
				{ name:"puck", labels:[ "Male" ] },
				{ name:"zephyr", labels:[ "Female" ] },
			],
			defaultVoiceName: "puck"
		},
		hume: {
			color: "#9a8dea",
			icon:  { texture:{ path: "hume-logo.png" }, width:64, height:64 },
			iconSmall: { texture:{ path: "hume-logo-small.png" }, width:24, height:24 },
			key: config.humeAIKey,
			title: "Hume AI EVI",
			specifier: "humeAIEVI",
			transcript: true,
			voices: [
				{ name:"AURA", labels:[] },
				{ name:"DACHER", labels:[] },
				{ name:"FINN", labels:[] },
				{ name:"ITO", labels:[] },
				{ name:"KORA", labels:[] },
				{ name:"STELLA", labels:[] },
				{ name:"SUNNY", labels:[] },
				{ name:"WHIMSY", labels:[] },
			],
			defaultVoiceName: "ITO"
		},
		openai: {
			color: "#00afba",
			icon:  { texture:{ path: "openai-logo.png" }, width:64, height:64 },
			iconSmall: { texture:{ path: "openai-logo-small.png" }, width:24, height:24 },
			key: config.openAIKey,
			title: "OpenAI Realtime",
			specifier: "openAIRealtime",
			transcript: true,
			voices: [
				{ name:"alloy", labels:[] },
				{ name:"ash", labels:[] },
				{ name:"ballad", labels:[] },
				{ name:"coral", labels:[] },
				{ name:"echo", labels:[] },
				{ name:"sage", labels:[] },
				{ name:"shimmer", labels:[] },
				{ name:"verse", labels:[] },
			],
			defaultVoiceName: "alloy"
		}
	}
};

export default Object.freeze(assets, true);
