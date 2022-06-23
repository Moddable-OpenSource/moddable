/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/322
flags: [onlyStrict]
---*/

const out = typeof trace === 'undefined' ? console.log : trace;

try {
  const r = (() => {
    function Foo() { }
    Foo.prototype.froze = () => 'is frozen'
    Object.freeze(Foo.prototype)
    const f = new Foo;
    f.froze = () => 'should never happen due to override mistake';
    return f.froze();
  })();
  out(`unexpected sync success: ${r}\n`);
} catch (e) {
  out(`expected sync failure ${e}\n`);
}

(async () => {
  function Foo() { }
  Foo.prototype.froze = () => 'froze'
  Object.freeze(Foo.prototype)
  const f = new Foo;
  f.froze = () => 'nofroze';
  return f.froze();
})().then(
  r => out(`unexpected async success: ${r}\n`),
  r => out(`expected async failure: ${r}\n`)
);
