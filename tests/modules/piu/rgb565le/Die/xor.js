/*---
description: 
flags: [onlyStrict, async]
---*/

class SampleBehavior extends $TESTMC.Behavior {
    onDisplaying(die) {
        die.xor(50, 50, die.width-100, die.height-100)
            .cut();
        screen.checkImage("d043160ad14456ea53c2dbb23cbbf4b9");
        
        die.set(0, 0, die.width/2, die.height)
            .xor(50, 50, die.width-100, die.height-100)
            .cut();
        screen.checkImage("f78f8d5d5b94c1510449a41c0e7dee24");

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