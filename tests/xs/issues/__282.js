/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/282
flags: [onlyStrict]
---*/

class Thing {  
  constructor(myName) {
    return (async () => {
      this.name = myName;
      return this;
    })();
  }
}

(async () => {
  let thing1 = await new Thing("Hartman");
  let thing2 = await new Thing("Lovitz");
})();
