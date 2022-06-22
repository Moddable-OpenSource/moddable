/*---
description: 
flags: [onlyStrict, async]
---*/

class SampleBehavior extends $TESTMC.Behavior {
    onDisplaying(die) {
        die.fill()
            .sub(0, 0, 50, 50)
            .cut();
        screen.checkImage("550ec4111f6e15b7cc0952039bfa581a");
        
        die.sub(100, 0, 50, 50)
            .sub(200, 0, 50, 50)
            .cut();
        screen.checkImage("68565931177a8b408df1850ce8833b8d");

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