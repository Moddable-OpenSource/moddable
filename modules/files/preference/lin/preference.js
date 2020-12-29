import {File} from "file";
import config from "mc/config";

export default class Preference {
    static _prefCache;

    static set(domain, key, value) {
        Preference._load();
        if (Preference._prefCache[domain] == undefined)
            Preference._prefCache[domain] = {};
		Preference._prefCache[domain][key] = value;
		Preference._save();
	}

    static get(domain, key) {
        Preference._load();
		return Preference._prefCache[domain] ? Preference._prefCache[domain][key] : undefined;
	}

    static delete(domain, key) {
        Preference._load();
		if ( (Preference._prefCache[domain] !== undefined) && (Preference._prefCache[domain][key] !== undefined) ) {
			delete Preference._prefCache[domain][key];
			if (Object.keys(Preference._prefCache[domain]).length == 0)
				delete Preference._prefCache[domain];
			Preference._save();
		}
	}

    static keys(domain) {
        Preference._load();
		return Preference._prefCache[domain] ? Object.keys(Preference._prefCache[domain]) : [];
	}

	static get _filePath() {
		return config.file.root + "preferences.json";
	}

	static _load() {
        if (Preference._prefCache === undefined) {
            try {
                let file = new File(Preference._filePath, false);
                Preference._prefCache = JSON.parse(file.read(String));
                file.close();
            } catch {
                // ignore errors
                Preference._prefCache = {};
            }
        }
	}

	static _save() {
        try {
            File.delete(Preference._filePath);
            let file = new File(Preference._filePath, true);
            file.write(JSON.stringify(Preference._prefCache));
            file.close();
        } catch {
            // ignore errors
        }
	}
}

