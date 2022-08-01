/*---
description: 
flags: [onlyStrict, async]
---*/

class SampleBehavior extends $TESTMC.Behavior {
    onDisplaying(content) {
        die.attach(content);
        die.or(0, 0, die.width, die.height/4)
        die.or(0, die.height/2, die.width, die.height/4)
            .cut();
        screen.checkImage("de4040e4b2f78fa903907f2e25675b9d");

        die.empty()
            .cut();
        screen.checkImage("9ce5b7c7f139fa44f841923a8f6945c3");

        die.detach();
        die.or(0, 0, die.width, die.height/4)
        die.or(0, die.height/2, die.width, die.height/4)
            .cut();
         screen.checkImage(undefined);

        $DONE();
    }
}

const content = new Content(null, { 
    left: 0, right: 0, top: 0, bottom: 0, 
    skin: new Skin({ fill: "white" }),
    Behavior: SampleBehavior
});

const die = new Die(null, {
    left: 0, right: 0, top: 0, bottom: 0
});

new Application(null, {
    skin: new Skin({  fill: "blue" }),
    contents: [ content ]
});

$TESTMC.timeout(200, "`onDisplaying` should have been triggered");