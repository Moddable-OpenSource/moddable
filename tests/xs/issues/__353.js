/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/353
flags: [onlyStrict]
---*/

const source = `
function main() {
    const v2 = [
        13.37,
        13.37, 13.37,
        13.37,
        13.37
    ];
    const v3 = [];
    let v4 = v3;
    function v5(v6, v7) {
        let v10 = 0;
        while (v10 < 10) {
            const v17 = [1337];
            const v18 = [v17];
            const v19 = {
                c: -2589926580,
                __proto__: v18,
                a: -2589926580
            };
            const v21 = new Set('__proto__');
            const v22 = v21.add(v19);
            const v23 = v22.clear();
            for (let v24 = 0; v24 < 8; v24++) {
                for (let v28 = 0; v28 < 5; v28++) {
                }
            }
            const v29 = v10 + 1;
            v10 = v29;
        }
    }
    const v30 = [];
    let v31 = v30;
    const v32 = v5(...v31, v4, ...v2, 10, 13.37);
}while();
`;

assert.throws(SyntaxError, () => eval(source));
