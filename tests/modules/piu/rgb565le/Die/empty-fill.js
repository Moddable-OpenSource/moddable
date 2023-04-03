/*---
description: 
flags: [onlyStrict, async]
---*/

class SampleBehavior extends $TESTMC.Behavior {
    onDisplaying(die) {
        die.fill()
            .cut();
        screen.checkImage("f29d3d7c7bace5943c2e44d85267b5d3");
       
        die.empty()
            .cut();
        screen.checkImage("73abe5bc0e6f03166f8810a15b95416f");

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

$TESTMC.timeout(275, "`onDisplaying` should have been triggered");