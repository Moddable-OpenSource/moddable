/*---
description: 
flags: [onlyStrict]
---*/

const content = new Content(null, { 
    left: 0, right: 0, top: 0, bottom: 0
});

const content2 = new Content(null, { 
    left: 0, right: 0, top: 0, bottom: 0
});

const die = new Die(null, {
    left: 0, right: 0, top: 0, bottom: 0
});

new Application(null, {
    contents: [ content ]
});

assert.sameValue(die.length, 0, `die should have no contents`);
assert.sameValue(die.container, undefined, `die should have no container`);
assert.sameValue(content.container, application, `content container should be application`);

die.attach(content);
assert.sameValue(die.length, 1, `die should have 1 content`);
assert.sameValue(die.container, application, `die container should be application`);
assert.sameValue(content.container, die, `content container should be die`);

die.detach();
assert.sameValue(die.length, 0, `die should have no contents`);
assert.sameValue(die.container, undefined, `die should have no container`);
assert.sameValue(content.container, application, `content container should be application`);

// TO DO: figure out what this should do (right now it crashes)
// die.attach(content2);