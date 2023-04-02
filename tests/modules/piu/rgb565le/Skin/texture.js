/*---
description: 
flags: [onlyStrict]
---*/

new Application(null, {
	displayListLength: 3000,
	skin: new Skin({ fill: "white" })
});

const circleTexture = new Texture({ path: "circleish.png" });

let circleMaskSkin = new Skin({
	texture: circleTexture
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, left: 0 }));
screen.checkImage("f29d3d7c7bace5943c2e44d85267b5d3");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	height: 20, width: 40
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, left: 0 }));
screen.checkImage("2b447bb260d1a2a23a95d7a0def9806d");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	height: 40, width: 40
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, left: 0 }));
screen.checkImage("bf43a12c63d48cee705c607a56025cae");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	x: 10, y: 10, height: 30, width: 30
});
application.add(new Content(null, { skin: circleMaskSkin, top: 10, left:10 }));
screen.checkImage("c9ae47ab978f6ae8c8641054332f5996");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	color: "red",
	height: 40, width: 40
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, left: 0 }));
screen.checkImage("bc8accb3366e6aab57215450bc72bcae");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	color: ["red", "blue"],
	height: 40, width: 40
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, left: 0 }));
screen.checkImage("bc8accb3366e6aab57215450bc72bcae");
application.first.state = 0.5;
screen.checkImage("1a4da9d8c6fcc4b070afdac962cdd7c7");
application.first.state = 1;
screen.checkImage("48aac1265ae65c331568605ba3509968");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	height: 20, width: 20,
	states: 20, variants: 20
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, left: 0 }));
screen.checkImage("af42ad3ecfa355a4033bd95382381cd3");
application.first.state = 1;
screen.checkImage("abff2e0d1c6df40efbf4fa797932d147");
application.first.variant = 1;
screen.checkImage("db313e942bc46f28534c7e5a04f746c9");
application.first.state = 0;
screen.checkImage("fa37c292ce6d28b2ac7ce5a16ab54009");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	height: 40, width: 40,
	tiles: { top: 0, bottom: 0, left: 0, right: 0 }
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, bottom: 0, left: 0, right: 0 }));
screen.checkImage("684790fa6a173c7d57509a2a5c50a7e4");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	color: ["black", "red"],
	height: 40, width: 40,
	tiles: { top: 10, bottom: 10, left: 10, right: 10 }
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, height:200, left: 0, width: 200 }));
screen.checkImage("8b684395af0e96af7a906a406d2e97ab");
application.first.state = 1;
screen.checkImage("8ed49b081ed09dcee56ba2efb5248060");