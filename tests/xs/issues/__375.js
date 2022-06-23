/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/375
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
    let v2 = 0;
    var Semm = new SharedArrayBuffer(null);
    var adte = new Boolean();
    do {
        const v5 = [
            13.37,
            13.37,
            13.37,
            13.37,
            13.37
        ];
        const v6 = [];
        let v7 = v6;
        function v8(v9, v10) {
            function v11(v12, v13) {
                let v15 = arguments;
                return v15;
            }
            const v16 = v11();
            const v18 = [
                13.37,
                13.37,
                13.37,
                13.37,
                13.37
            ];
            const v20 = 1919903432 === 1919903432;
            const v21 = [v20];
            var AMrP = DataView;
            const v24 = 1919903432 << 6;
            var EWch = Date;
            const v25 = v21.indexOf(v24, 0, 6, v24, v24);
            const v28 = new Uint32Array(11105);
            var rhbY = v8(null, null, null, null, null);
            var fJwj = main();
            var TMJa = Reflect;
            var GfCt = v8(v28['448'], v6, v6, v28[''], v28['']);
            var RdYi = JSON.stringify('(,#');
            var jcrn = main();
            var SBcE = Reflect;
            const v29 = v16[1337];
            for (const v30 of v28) {
            }
        }
        const v31 = [];
        let v32 = v31;
        const v33 = v8(...v32, v7, v5, 10, 13.37);
        const v34 = v2 + 1;
        v32.length = v34;
    } while (v2 < 10);
}
main();
