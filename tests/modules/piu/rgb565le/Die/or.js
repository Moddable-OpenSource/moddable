/*---
description: 
flags: [onlyStrict, async]
---*/

class SampleBehavior extends $TESTMC.Behavior {
    onDisplaying(die) {
        die.or(10, 10, 50, 50)
            .cut();
        screen.checkImage("2223c4764c1d80eca259e7a5854a40af");

        die.or(70, 70, 100, 100)
            .cut();
        screen.checkImage("213f0088905c22817cb0fc4ef3f7ee0a");
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