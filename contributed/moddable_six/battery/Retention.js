import BIN from "BinaryMessage";
import model from "model";
import Preference from "preference";

const preferenceDomain = "selection";

class Retention {
	static defaultParams(modelParams) {
		let selectionParams = { VERSION:model.VERSION };
		for (let name in modelParams)
			selectionParams[name] = modelParams[name].value;
		return selectionParams;
	}
	static deletePreference(key) {
		return Preference.delete(preferenceDomain, key);
	}
	static filterParams(retention, modelParams, selectionParams) {
		let changed = false;
		for (let name in modelParams) {
			let param = modelParams[name];
			if (param.retention <= retention) {
				let value = param.value;
				if (selectionParams[name] != value) {
					selectionParams[name] = value;
					changed = true;
				}
			}
		}
		return changed;
	}
	static readParams(modelParams, key, format) {
		let selectionParams = this.defaultParams(modelParams);
		try {
			let buffer = Preference.get(preferenceDomain, key);
			if (buffer) {
				let parsedParams = BIN.parse(format, buffer, false);
				if (parsedParams) {
					if (parsedParams.VERSION == selectionParams.VERSION) {
						for (let name in modelParams) {
							let param = modelParams[name];
							if (name in parsedParams) {
								let value = parsedParams[name];
								if (param.range) {
									if (param.range.indexOf(value) >= 0)
										selectionParams[name] = value;
								}
								else
									selectionParams[name] = value;
							}
						}
					}
				}
			}
		}
		catch {
			this.writeParams(format, key, selectionParams);
		}
		return selectionParams;
	}
	static readPreference(key) {
		return Preference.get(preferenceDomain, key);
	}
	static testParams(modelParams, key, format) {
		let selectionParams = null;
		try {
			let buffer = Preference.get(preferenceDomain, key);
			if (buffer) {
				selectionParams = this.defaultParams(modelParams);
				let parsedParams = BIN.parse(format, buffer, false);
				if (parsedParams) {
					if (parsedParams.VERSION == selectionParams.VERSION) {
						for (let name in modelParams) {
							let param = modelParams[name];
							if (name in parsedParams) {
								let value = parsedParams[name];
								if (param.range) {
									if (param.range.indexOf(value) >= 0)
										selectionParams[name] = value;
								}
								else
									selectionParams[name] = value;
							}
						}
					}
				}
			}
		}
		catch {
		}
		return selectionParams;
	}
	static writeParams(format, key, params) {
		let buffer = BIN.serialize(format, params, false);
		Preference.set(preferenceDomain, key, buffer);
	}
	static writePreference(key, value) {
		return Preference.set(preferenceDomain, key, value);
	}
}
Retention.NEVER = 0;
Retention.UNTIL_RESET = 1;
Retention.UNTIL_FACTORY_RESET = 2;
Retention.FOREVER = 3;
export default Retention;