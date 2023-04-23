/*---
description: 
flags: [onlyStrict, async]
---*/

class SampleBehavior extends $TESTMC.Behavior {
    onDisplaying(die) {
        die.fill()
            .and(10, 10, 50, 50)
            .cut();
        screen.checkImage("ebaa18162e0319186ca4cf04864c8d88");
       
        die.and(10, 10, 25, 25)
            .cut();
        screen.checkImage("b65f1a920565012534b7cac6ca001e2d");

        die.and(100, 100, 25, 25)
            .cut();
        screen.checkImage("7734e0d9fa986c3e446e15f6e5c7701b");

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

$TESTMC.timeout(150, "`onDisplaying` should have been triggered");