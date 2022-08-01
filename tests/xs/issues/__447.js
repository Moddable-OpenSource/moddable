/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/447
flags: [onlyStrict]
---*/

const source = `
function main() {
    const v0 = [];
    let v1 = v0;
    function v2({, v4, v5) {
        'use strict';
        const v8 = [
            13.37,
            13.37,
            13.37,
            13.37,
            13.37
        ];
        const v9 = [];
        let v10 = v9;
        function v11(v12, v13) {
            const v16 = [
                1337,
                1337
            ];
            const v17 = [1337];
            const v20 = NaN.toLocaleString();
            const v21 = eval(v20);
            const v22 = {
                e: 1337,
                __proto__: v16,
                valueOf: Proxy,
                d: v17
            };
        }
        const v23 = [];
        let v24 = v23;
        const v25 = v11(...v24, v10, ...v8, 10, 13.37);
    }
    for (let v29 = 0; v29 < 10; v29++) {
        const v30 = v2(v29, 0);
    }
}
main();
`;

assert.throws(SyntaxError, () => eval(source));
