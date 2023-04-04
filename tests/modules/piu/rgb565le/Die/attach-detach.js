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
        screen.checkImage("8777bd1191fcf96fc52297010ca7cdc9");

        die.empty()
            .cut();
        screen.checkImage("5fd1cbf7e9d4a8f48652c91815270d6e");

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