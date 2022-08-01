/*---
description: 
flags: [onlyStrict, async]
---*/

class SampleBehavior extends $TESTMC.Behavior {
    onDisplaying(die) {
        die.fill()
            .and(10, 10, 50, 50)
            .cut();
        screen.checkImage("2223c4764c1d80eca259e7a5854a40af");
       
        die.and(10, 10, 25, 25)
            .cut();
        screen.checkImage("084b090aae60ca17fc4cb4fcbf2f5322");

        die.and(100, 100, 25, 25)
            .cut();
        screen.checkImage("c9d0aacc2c08f82343962fec6a71c2ab");

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