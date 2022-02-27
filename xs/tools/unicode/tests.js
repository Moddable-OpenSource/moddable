import {TOOL, FILE} from "tool";

function cps(field) {
	return "[" + field.split(" ").map(part => "0x" + part).join(",") + "]";
}

export default class extends TOOL {
	constructor(argv) {
		super(argv);
	}
	run() {
		const path = this.resolveFilePath("./data/NormalizationTest.txt");
		const string = this.readFileString(path);
		const lines = string.split("\n");
		for (let line of lines) {
			if (line.length == 0)
				continue;
			if (line[0] == "#")
				continue;
			if (line[0] == "@") {
				const d = line.split(" # ");
				this.report(`// ${d[1]}`);
				continue;
			}
			const d = line.split(" # ");
			const f = d[0].split(";");
			
			this.report(`{ s:${cps(f[0])}, NFC:${cps(f[1])}, NFD:${cps(f[2])}, NFKC:${cps(f[3])}, NFKD:${cps(f[4])}, comment:"${d[1].replaceAll('"', '\\"')}" },`);
		}
	}
}
