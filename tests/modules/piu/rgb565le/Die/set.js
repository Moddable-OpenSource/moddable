/*---
description: 
flags: [onlyStrict, async]
---*/

class SampleBehavior extends $TESTMC.Behavior {
    onDisplaying(die) {
        die.set(10, 10, 220, 300)
            .cut();
        screen.checkImage("db7d9d64b455792cf369d672d54169fd");

        die.set(-10, -10, 10, 10)
            .cut();
        screen.checkImage("b983f4c33d26bc546c2b7647d9b7cc3b");
        
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