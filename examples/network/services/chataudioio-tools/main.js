/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

import ChatAudioIO from "ChatAudioIO";
import Timer from "timer";

const rooms = new Map([
	["outside", {temperature: 15}],
	["kitchen", {temperature: 25}],
	["living room", {temperature: 20}],
	["basement", {temperature: 17}],
	["garage", {temperature: 16}],
	["dining room", {temperature: 20}]
]);

const chat = new ChatAudioIO({
	specifier: "googleGeminiLive",
	voiceName: "Orus",
	instructions:  `You are a helpful home assistant.
					You provide temperature for locations in and around the house.
					You don't engage on any topic except temperatures.
					Your answers are focused and terse, typically no more than seven words.
					Temperatures in conversation are in Fahrenheit, unless the user asks to use Celsius.
					You can control both the furnace and the air conditioner.
					The furnace and air conditioner are both off now.
					The furnace cannot warm the house beyond 25 degrees, and the air conditioner cannot cool it below 15 degrees.
					Never run both the furnace and air conditioner at the same time. If turning one on, make sure to turn the other off first.
					Never say that you are checking the temperature.`,
	functions: [
		{
			name: "get_temperature",
			description: "Current temperatures in degrees Celsius.",
			parameters: {
				type: "object",
				properties: {
					locations: {
						type: "array",
						items: {
							type: "string",
							enum: Array.from(rooms.keys()),
						},
						description: "names of locations to get the temperature for"
					}
				},
				required: ["locations"]
			}
		},
		{
			name: "control_furnace",
			description: "Turn furnace on and off",
			parameters: {
				type: "object",
				properties: {
					on: {
						type: "boolean",
						description: "True to turn furnace on; false to turn fornace off."
					}
				},
				required: ["on"]
			}
		},
		{
			name: "control_airconditioner",
			description: "Turn air conditioner on and off",
			parameters: {
				type: "object",
				properties: {
					on: {
						type: "boolean",
						description: "True to turn air conditioner on; false to turn air conditioner off."
					}
				},
				required: ["on"]
			}
		}
	],
	onStateChanged(state) {
		trace(`State: ${ChatAudioIO.states[state]} ${this.error ?? ""}\n`);
	},
	onInputTranscript(text) {
		trace(`User: ${text}\n`);
	},
	onOutputTranscript(text) {
		trace(`Agent: ${text}\n`);
	},
	onFunctionCall(call, name, parameters) {
		switch (name) {
			case "get_temperature": {
				trace(`Call: get_temperature(locations: ${parameters.locations})\n`);
				const result = {};
				parameters.locations.forEach(location => {
					result[location] = {temperature: rooms.get(location).temperature + "℃"};
				});
				this.sendFunctionResult(call, name, result);
				} break;
			case "control_furnace": {
				trace(`Call: control_furnace(on: ${parameters.on})\n`);
				if (parameters.on)
					this.furnace ??= Timer.repeat(() => adjustTemperature(+0.5), 5_000); 
				else {
					Timer.clear(this.furnace);
					delete this.furnace;
				}
				this.sendFunctionResult(call, name, `{"furnace": ${this.airconditioner ? "true" : "false"}}`);
			} break;
			case "control_airconditioner": {
				trace(`Call: control_airconditioner(on: ${parameters.on})\n`);
				if (parameters.on)
					this.airconditioner ??= Timer.repeat(() => adjustTemperature(-0.5), 5_000); 
				else {
					Timer.clear(this.airconditioner);
					delete this.airconditioner;
				}
				this.sendFunctionResult(call, name, `{"airconditioner": ${this.airconditioner ? "true" : "false"}}`);
			} break;
		}
	}
});
chat.connect();

function adjustTemperature(delta) {
	trace(`adjust inside temperature by ${delta} degrees\n`);
	rooms.forEach((value, location) => {
		if ("outside" === location)
			return;
		rooms.set(location, {temperature: Math.min(25, Math.max(15, value.temperature - 0.5))});
	});
}
