import {File} from "file";
import config from "mc/config";

export default class Preference {
    static #filePath = config.file.root + "modpreferences.json";
    static #cache = Preference.#load();

    static set(domain, key, value) {
		if (value instanceof ArrayBuffer)
			value = {buffer: (new Uint8Array(value).toString())};

        Preference.#cache[domain] ??= {};
		Preference.#cache[domain][key] = value;
		Preference.#save();
	}

    static get(domain, key) {
		let result = Preference.#cache[domain]?.[key];

		if (result && ("object" === typeof result))
			result = Uint8Array.from(result.buffer.split(",")).buffer;

		return result;
	}

    static delete(domain, key) {
		if (Preference.#cache[domain]?.[key] !== undefined) {
			delete Preference.#cache[domain][key];
			if (Object.keys(Preference.#cache[domain]).length === 0)
				delete Preference.#cache[domain];
			Preference.#save();
		}
	}

    static keys(domain) {
		return Preference.#cache[domain] ? Object.keys(Preference.#cache[domain]) : [];
	}

	static #load() {
		let result = {};

		try {
			if (File.exists(Preference.#filePath)) {
				let file = new File(Preference.#filePath, false);
				result = JSON.parse(file.read(String));
				file.close();
			}
		} catch {
			// ignore errors
		}

		return result;
	}

	static #save() {
		try {
			File.delete(Preference.#filePath);
			const file = new File(Preference.#filePath, true);
			file.write(JSON.stringify(Preference.#cache));
			file.close();
		} catch {
			// ignore errors
		}
	}
}
