/*---
description: 
flags: [onlyStrict]
---*/

new Application(null, {
	displayListLength: 3000,
	skin: new Skin({ fill: "white" })
});

const CircleTexture = Texture.template({ path: "circleish.png" });

const circleMaskSkin = new Skin({
	Texture: CircleTexture,
	height: 40, width: 40,
});

const CircleMaskSkinTemplate = Skin.template({
	Texture: CircleTexture,
	height: 40, width: 40,
});

application.add(new Content(null, { skin: circleMaskSkin, top: 0, left: 0 }));
application.add(new Content(null, { Skin: CircleMaskSkinTemplate, top: 40, left: 0 }));

screen.checkImage("8f657445c9b5dda381d2f9726e7e30b6");
