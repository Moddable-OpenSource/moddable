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
import Preference from "preference";

class Controller extends Behavior {
	constructor() {
		super();
		this.forgottenHistory = 0;
		this.going = 0;
		this.history = [];
		this.view = null;
	}
	display(container, view, backwards) {
		if (this.view) {
			this.onScreenUndisplaying(container, this.view);
			if (backwards)
				this.view.runTransitionBackwards(view);
			else {
				if (this.view.historical)
					this.history.push(this.view);
				view.runTransitionForwards(this.view);
			}
		}
		else {
			this.container.add(new view.Template(view));
			this.going = 0;
			this.onScreenDisplayed(container, view);
		}
		this.view = view;
		if (this.forgottenHistory) {
			this.history.length = this.forgottenHistory;
			this.forgottenHistory = 0;
		}
		trace("HISTORY:");
		this.history.forEach(view => trace(" " + view.id));
		trace("\n");
	}
	forgetHistory(forgottenHistory = 1) {
		this.forgottenHistory = forgottenHistory;
	}
	goBack(count = 1) {
		if (this.history.length) {
			let view;
			while (count) {
				view = this.history.pop();
				count--;
			}
			this.going = -1;
			this.container.defer("display", view, true);
		}
	}
	goHome() {
		if (this.history.length) {
			let view = this.history[0];
			this.history.length = 0;
			this.going = -1;
			this.container.defer("display", view, true);
		}
	}
	goTo(id) {
		let View = importNow(id);
		if (View) {
			let view = new View();
			view.id = id;
			this.going = 1;
			this.container.defer("display", view, false);
		}
	}
	goWith(data) {
		let id = data.View;
		let View = importNow(id);
		if (View) {
			let view = new View(data);
			view.id = id;
			this.going = 1;
			this.container.defer("display", view, false);
		}
	}
	onCreate(container, $) {
		this.container = container;
		this.options = JSON.parse(Preference.get("model", "options") ?? "[]");
		this.personas = $.model.map(persona => {
			persona = { ...persona };
			const service = assets.services[persona.service];
			const voiceName = persona.voiceName;
			delete persona.voiceName;
			const voice = service.voices.find(voice => voice.name == voiceName);
			persona.voiceID = voice ? voice.id : service.defaultVoiceID;
			persona.providerID = service.defaultProviderID;
			persona.modelID = service.defaultModelID;
			controller.readOption(persona);
			return persona;
		});
	}
	onDisplaying(container) {
		const id = "Splash";
		const View = importNow(id);
		const view = new View();
		view.id = id;
		this.history = [];
		this.display(container, view, false);
	}
	onScreenDisplayed(container, view) {
		container.first.distribute("onDisplayed");
	}
	onScreenUndisplaying(container, view) {
		container.first.distribute("onUndisplaying");
	}
	onTransitionEnded(container) {
		application.purge();
		this.going = 0;
		this.onScreenDisplayed(container, this.view);
		this.updateScreen();
	}
	readOption(persona) {
		const option = this.options.find(option => option.title == persona.title);
		if (option) {
			if (option.service in assets.services) {
				persona.service = option.service;
				const service = assets.services[option.service];
				const voiceName = option.voiceName;
				const voiceID = option.voiceID;
				const providerID = option.providerID;
				const modelID = option.modelID;
				let voice, provider, model;
				if (voiceName) // compatibility
					voice = service.voices.find(voice => voice.id == voiceName);
				else if (voiceID)
					voice = service.voices.find(voice => voice.id == voiceID);
				persona.voiceID = (voice) ? voice.id : service.defaultVoiceID;
				if (service.providers) {
					if (providerID && modelID) {
						provider = service.providers.find(provider => provider.id == providerID);
						if (provider)
							model = provider.models.find(model => model.id == modelID);
					}
				}
				if (provider && model) {
					persona.providerID = provider.id;
					persona.modelID = model.id;
				}
				else {
					persona.providerID = service.defaultProviderID;
					persona.modelID = service.defaultModelID;
				}
			}
		}
	}
	redisplay() {
		this.onScreenUndisplaying(application, this.view);
		this.container.replace(this.container.first, new this.view.Template(this.view));
		this.onScreenDisplayed(application, this.view);
	}
	writeOption(persona) {
		let option = this.options.find(option => option.title == persona.title);
		if (option) {
			delete option.voiceName;
			option.service = persona.service;
			option.voiceID = persona.voiceID;
			option.providerID = persona.providerID;
			option.modelID = persona.modelID;
		}
		else {
			option = {
				title: persona.title,
				service: persona.service,
				voiceID: persona.voiceID,
				providerID: persona.providerID,
				modelID: persona.modelID,
			};
			this.options.push(option);
		}
		Preference.set("model", "options", JSON.stringify(this.options));
	}
	
	updateScreen() {
		if (this.going)
			return;
		let container = application.last;
		if (container)
			container.distribute("onUpdate");
	}
}
Object.freeze(Controller.prototype);

export default Controller;
