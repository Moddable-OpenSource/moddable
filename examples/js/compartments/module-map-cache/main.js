import * as increment from "increment";
import * as mod1 from "mod";
import * as mod2 from "mod";
let counts = {
	increment: 0,
	mod1: 0,
	mod2: 0,
};
let proxy = new Proxy({ 
	increment, 
	mod1, 
	mod2
}, {
	get(target, key) {
		counts[key]++;
		return { namespace: target[key] };
	}
});
let compartment = new Compartment({
	modules: proxy,
	resolveHook(specifier, refererSpecifier) {
		return specifier;
	},
	loadNowHook(specifier) {
	}
});
compartment.importNow("mod1");
compartment.importNow("mod2");
trace(counts.increment + " " + counts.mod1 + " " + counts.mod2 + "\n"); // 1 1 1
