import Resource from "Resource";

export default function(x, y) {
	let a = new Array(2);
	a[0] = x;
	a[1] = y;
	let r = new Resource("data.txt", import.meta.archive);
	debugger
	return a;
}
