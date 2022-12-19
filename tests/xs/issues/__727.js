/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/727
flags: [onlyStrict]
---*/

class Foo {}
class Bar extends Foo {}
delete Bar.prototype.constructor;
new Bar(); 
