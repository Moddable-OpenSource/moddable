let counts = {"mod":0, "increment":0 };
let proxy = new Proxy({ "mod":"mod", "increment":"increment" }, {
	get(target, key) {
		counts[key]++;
		return target[key];
	}
});
let mod = new Compartment({}, { "*": proxy });
trace(counts.mod + " " + counts.increment + "\n"); // 1 1
