/*---
description: 
flags: [onlyStrict, async]
---*/

class SampleBehavior extends $TESTMC.Behavior {
    onDisplaying(die) {
        die.set(10, 10, 220, 300)
            .cut();
        screen.checkImage("6345899f70604e2d3fe49fc3937db544");

        die.set(-10, -10, 10, 10)
            .cut();
        screen.checkImage("2b80347b64944280e25bc111f52c3e6d");
        
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

$TESTMC.timeout(300, "`onDisplaying` should have been triggered");