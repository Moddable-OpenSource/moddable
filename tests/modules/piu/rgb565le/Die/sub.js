/*---
description: 
flags: [onlyStrict, async]
---*/

class SampleBehavior extends $TESTMC.Behavior {
    onDisplaying(die) {
        die.fill()
            .sub(0, 0, 50, 50)
            .cut();
        screen.checkImage("8e4f7bde403e94148a35744e08b4b123");
        
        die.sub(100, 0, 50, 50)
            .sub(200, 0, 50, 50)
            .cut();
        screen.checkImage("841ed8760dfc834e964d1cfdef0b202c");

        $DONE();
    }
}

const content = new Content(null, { 
    left: 0, right: 0, top: 0, bottom: 0, 
    skin: new Skin({ fill: "white" }),
});

const sampleDie = new Die(null, {
    left: 0, right: 0, top: 0, bottom: 0,
    contents: [ content ],
    Behavior: SampleBehavior
});

new Application(null, {
    skin: new Skin({  fill: "blue" }),
    contents: [ sampleDie ]
});

$TESTMC.timeout(200, "`onDisplaying` should have been triggered");