/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/328
flags: [onlyStrict]
---*/

(async () => {
  await (async () => {});
})();

(async () => {
  await 0;
  (async () => {});
})();

(async () => {
  await 0;
  {
    (async () => {});
  }
})();
