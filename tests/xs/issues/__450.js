/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/450
flags: [onlyStrict]
---*/

function main() {
    let v2 = 0;
    function v3(v4, v5) {
        let v8 = 0;
        const v9 = v8 + 1;
        v8 = v9;
        const v13 = [
            13.37,
            13.37,
            13.37,
            13.37,
            13.37
        ];
        const v15 = [
            1337,
            1337,
            1337,
            1337,
            1337
        ];
        const v16 = {
            e: v15,
            length: 13.37,
            d: v15,
            __proto__: Symbol,
            valueOf: v13,
            c: 'p76QI.ipnu'
        };
        const v18 = [];
        Symbol.__proto__ = v18;
        const v22 = [
            Set,
            Set,
            1337,
            '0MS*nEwLSL'
        ];
        const v26 = new Proxy(Promise, Reflect);
        const v30 = v3 * v16;
        const v31 = v26.bind(1, 0, v3, v30);
    }
    const v32 = v3();
    const v33 = v2 + 1;
    v2 = v33;
}
main();
