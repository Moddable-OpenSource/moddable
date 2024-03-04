import Modules from "modules";
import * as MCNamespace from "piu/MC";
import * as WipeTransitionNamespace from "piu/WipeTransition";
import * as CombTransitionNamespace from "piu/CombTransition";
const globals = {
	Date, Application, Behavior, CLUT, Column, Container, Content, DeferLink, Die, Label, Layout, Link,
	Locals, Port, Row, Scroller, Skin, Style, template, Template, Text, Texture, TouchLink,
	Transition, blendColors, hsl, hsla, rgb, rgba,
};
const modules = {
	"piu/MC": { namespace:MCNamespace },
	"piu/WipeTransition": { namespace: WipeTransitionNamespace },
	"piu/CombTransition": { namespace: CombTransitionNamespace },
};

let ErrorApplication = Application.template($ => ({
	displayListLength:8192,
	skin:{ fill:"#192eab" },
	contents: [
		Column($, { 
			left:0, right:0,
			contents: [
				Content($, { skin:{ texture:{ path:"main.png" }, color:"white", width:64, height:64 } }),
				Text($, { left:10, right:10, style:{ font:"semibold 18px Open Sans", color:"white" }, string:$ } ),
			]
		}),
	]
}));

export default function () {
	const specifiers = Modules.archive;
	const path = specifiers.find(it => it.endsWith("/mod"));
	const archive = globalThis.archive;
	let application;
	if (archive) {
		const compartment = new Compartment({
			globals: { 
				...globals, 
				archive
			},
			modules: {
				...modules, 
				mod: { archive, path }
			},
			resolveHook(specifier) {
				return specifier;
			}
		});
		application = compartment.importNow("mod").default;
		if (application instanceof Function)
			application = application();
	}
	else 
		application = new ErrorApplication("No mod installed");
	return application;
}
