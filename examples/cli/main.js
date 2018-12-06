trace("top-level executes\n");

export default function main(argv) {
	let three = 1 + 2;
	let message = "Hello, world";
	trace("Hello world, 1+2=" + three + "\n");
	trace(`1+2=${1+2}`);
	if (argv.length) {
		trace("argv[0]: " + argv[0] + "\n");
	}
}
