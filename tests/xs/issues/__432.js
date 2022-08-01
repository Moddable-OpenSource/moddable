/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/432
flags: [onlyStrict]
---*/

const source = `
function main() {
    try {
        const v2 = [
            -1.7976931348623157e+308,
            -1.7976931348623157e+308,
            -1.7976931348623157e+308,
            -1.7976931348623157e+308,
            -1.7976931348623157e+308
        ];
        const v3 = [];
        let v4 = v3;
        function v4 = ()(v6, v7) {
            const v10 = [
                1,
                1,
                1,
                1,
                1
            ];
            let v11 = v10;
            function v12(v13, v14) {
                try {
                    const v15 = v12();
                } catch (v16) {
                    const v18 = [1337];
                    function v19(v20, v21) {
                        'use strict';
                        const v22 = [];
                        let v23 = v22;
                        const v27 = new Int8Array(512);
                        try {
                            const v31 = [
                                -441746.4139016614,
                                -441746.4139016614,
                                -441746.4139016614,
                                -441746.4139016614,
                                -441746.4139016614
                            ];
                            let v32 = v23;
                            function v33(v34, v35) {
                                v13[2] = eval;
                            }
                            const v36 = [];
                            let v37 = v36;
                            const v38 = v33(...v37, ...v32, ...v31, 1337, -441746.4139016614);
                        } catch (v39) {
                            const v42 = [
                                -441746.4139016614,
                                -441746.4139016614,
                                -441746.4139016614,
                                -441746.4139016614,
                                -441746.4139016614
                            ];
                            const v43 = [];
                            let v44 = v43;
                            function v45(v46, v47) {
                                const v50 = [
                                    719904.8518018327,
                                    719904.8518018327,
                                    719904.8518018327,
                                    719904.8518018327,
                                    719904.8518018327
                                ];
                                const v53 = [];
                                let v54 = v53;
                                function v55(v56, v57) {
                                    const v59 = [
                                        13.37,
                                        13.37,
                                        13.37,
                                        13.37,
                                        13.37
                                    ];
                                    for (const v62 of v56) {
                                        let v66 = 0;
                                        do {
                                            break;
                                            const v67 = v66 + 1;
                                            v66 = v67;
                                        } while (v66 < 10);
                                        v23 = v55;
                                        let v68 = v66;
                                        let v69 = 0;
                                        const v70 = ~v69;
                                        v69 = v70;
                                    }
                                    const v71 = [
                                        1000,
                                        1000,
                                        v55,
                                        'OQc32+vRhR'
                                    ];
                                    const v73 = [
                                        -2.220446049250313e-16,
                                        v71
                                    ];
                                    const v74 = v73.toLocaleString();
                                    const v75 = v74.substring(1000, 128);
                                    const v76 = eval(v75);
                                }
                                const v77 = [];
                                let v78 = v77;
                                const v79 = v55(...v78, v54, ...v50, 10, 719904.8518018327);
                            }
                            const v80 = [];
                            let v81 = v80;
                            const v82 = v45(...v81, ...v44, ...v42, 1337, -441746.4139016614);
                        }
                        const v83 = {
                            __proto__: 13.37,
                            d: v27
                        };
                        const v86 = [
                            v83,
                            v23,
                            178987211
                        ];
                        const v87 = {
                            toString: v86,
                            c: 'symbol'
                        };
                        const v88 = [v87];
                        const v89 = [v88];
                        const v90 = [v89];
                        const v92 = JSON.stringify(v90, JSON, v89);
                        const v93 = eval(v92);
                    }
                    const v94 = v19(v18, v18);
                }
            }
            let v95 = v11;
            const v96 = v12(v95, v11, ...v10, -258611160, 1);
        }
        const v97 = [];
        let v98 = v97;
        const v99 = v5(...v98, v4, ...v2, -3477252921, -1.7976931348623157e+308);
    } catch (v100) {
    }
}
main();
`;

assert.throws(SyntaxError, () => eval(source));
