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
screen.checkImage("14e6edc6291901a47e0268a966b4d99e");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	height: 20, width: 40
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, left: 0 }));
screen.checkImage("053273c5e82f6ce6437bef73c975e8d4");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	height: 40, width: 40
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, left: 0 }));
screen.checkImage("f82011683f40c81820b7e7315bb6a466");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	x: 10, y: 10, height: 30, width: 30
});
application.add(new Content(null, { skin: circleMaskSkin, top: 10, left:10 }));
screen.checkImage("b49fae1704559deccbb62721dcdd2011");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	color: "red",
	height: 40, width: 40
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, left: 0 }));
screen.checkImage("dc719985005822afa42e3076b6d25190");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	color: ["red", "blue"],
	height: 40, width: 40
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, left: 0 }));
screen.checkImage("dc719985005822afa42e3076b6d25190");
application.first.state = 0.5;
screen.checkImage("5fd7a8f99e8f17511c5fca6a290b7f98");
application.first.state = 1;
screen.checkImage("9ca0338769a305a425c42527d4ee5085");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	height: 20, width: 20,
	states: 20, variants: 20
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, left: 0 }));
screen.checkImage("c4c6fcc43f4a502fd67879e391b9cc31");
application.first.state = 1;
screen.checkImage("22c14ffe6fb19231faef579d81b7f918");
application.first.variant = 1;
screen.checkImage("1689e1e3549be21a23996da95eda9e5f");
application.first.state = 0;
screen.checkImage("bc4f348a5e4141de64cc8696fa44fdea");
application.remove(application.first);

circleMaskSkin = new Skin({
	texture: circleTexture,
	height: 40, width: 40,
	tiles: { top: 0, bottom: 0, left: 0, right: 0 }
});
application.add(new Content(null, { skin: circleMaskSkin, top: 0, bottom: 0, left: 0, right: 0 }));
screen.checkImage("b7b9001e722cadb0107694675ed9668d");
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