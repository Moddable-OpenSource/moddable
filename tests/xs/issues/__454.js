/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/454
flags: [onlyStrict]
---*/

function main() {
    const v4 = [
        1337,
        1337,
        1337,
        1337
    ];
    const v5 = [-2];
    const v6 = {
        d: 13.37,
        c: v5,
        toString: 1337,
        constructor: v4,
        __proto__: isFinite,
        length: v4
    };
    for (let v12 = 0; v12 < 127; v12++) {
        for (let v16 = 2; v16 < 100; v16 = v16 + 9) {
            try {
                let v18 = String;
                const v19 = v18.fromCharCode(v16, String, 4192820042, v12, v16);
                const v20 = eval(v19);
                const v24 = v19.split(v20, v6);
            } catch (v25) {
            }
        }
    }
    let v31 = 0;
    const v32 = v31 + 1;
    v31 = v32;
    let v35 = 0;
    const v36 = v35 + 1;
    v35 = v36;
}
main();
