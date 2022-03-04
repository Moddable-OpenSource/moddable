import Modules from "modules";
import {Outline} from "commodetto/outline";
import {} from "piu/MC";
import {} from "piu/shape";

const shapeSkin = new Skin({ fill:rgba(255, 255, 255, 0.5), stroke:"white" });

let ShapeApplication = Application.template($ => ({
	skin:new Skin({ fill:"#192eab" }),
	style:new Style({ font:"semibold 18px Open Sans", color:"white", horizontal:"center" }),
	contents: [
		Shape($, { left:0, right:0, top:0, bottom:0, skin:shapeSkin } ),
		Label($, { left:0, right:0, bottom:0, height:40, string:"no mods" } ),
	],
	behavior: {
		addMod(application) {
			if (Modules.has("mod")) {
				try {
					let Behavior = Modules.importNow("mod");
					let shape = application.first;
					shape.behavior = new Behavior();
					shape.behavior.onCreate(shape);
				}
				catch(e) {
					trace(`${e.stack}\n`);
					application.last.string = "no mods";
				}
			}
		},
		onCreate(application) {
			this.addMod(application)
		},
		onDisplaying(application) {
			application.first.start();
		},
		onLabel(application, string) {
			application.last.string = string;
		},
	},
}));

export default new ShapeApplication(null, { displayListLength:4096, touchCount:1, pixels: 240 * 64 });
