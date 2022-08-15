/*---
description: 
flags: [onlyStrict]
---*/

const source = new ModuleSource(`
	export var v0;	
	export default 0
	export { v0 as w0 };	

	import v1 from "mod";
	import * as ns1 from "mod";	
	import { x1 } from "mod";	
	import { v1 as w1 } from "mod";	

	export { x2 } from "mod";
	export { v2 as w2 } from "mod";
	export * from "mod";
	export * as ns2 from "mod";
`);
const sourceBindingsString = JSON.stringify(source.bindings);

const bindings = [
	{
		export: "v0",
		as: "w0"
	},
	{
		export: "v0"
	},
	{
		export: "default"
	},
	{
		import: "default",
		as: "v1",
		from: "mod"
	},
	{
		importAllFrom: "mod",
		as: "ns1"
	},
	{
		import: "x1",
		from: "mod"
	},
	{
		import: "v1",
		as: "w1",
		from: "mod"
	},
	{
		export: "x2",
		from: "mod"
	},
	{
		export: "v2",
		as: "w2",
		from: "mod"
	},
	{
		exportAllFrom: "mod"
	},
	{
		exportAllFrom: "mod",
		as: "ns2"
	}
];
const bindingsString = JSON.stringify(bindings);

assert.sameValue(sourceBindingsString, bindingsString);
