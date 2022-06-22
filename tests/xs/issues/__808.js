/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/808
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function main() {
const v2 = "function";
const v3 = 9419;
const v5 = new Compartment();
const v6 = v5.module();
const v7 = 0;
const v11 = 36251;
const v14 = [26167,536870912n,-2647986534,-2647986534];
const v16 = TypeError();
const v17 = {"apply":Object,"construct":v14,"getOwnPropertyDescriptor":Function,"isExtensible":TypeError,"preventExtensions":Object};
const v18 = Proxy;
const v19 = 2;
const v20 = 1;
function* v21(v22,v23) {
}
const v26 = new Int32Array(36251);
try {
    const v27 = v26.reduce(v21);
} catch(v28) {
}
const v29 = [-2432330610];
const v30 = {};
const v31 = [v30,v30];
const v33 = [690473.6494891534,690473.6494891534];
const v35 = new Set();
const v38 = new Uint8Array();
const v39 = v38.includes;
const v40 = Date();
function v41(v42,v43) {
    const v45 = [];
    const v46 = Reflect.apply(v42,v40,v45);
    const v48 = Symbol.species;
    const v49 = v45[v48];
    const v50 = ArrayBuffer;
    const v51 = [v45];
    const v52 = "boolean";
    const v53 = Int8Array;
    const v54 = Array;
    async function v55(v56,v57,v58,v59,v60) {
    }
    const v61 = v55 in v31;
    const v62 = [64,64];
    const v63 = -9007199254740991;
    const v64 = {"set":TypeError};
    const v65 = v35.delete;
    const v66 = Reflect.apply();
}
const v68 = new Promise(v41);
const v69 = 2157069075;
const v70 = -536870912;
const v71 = {};
const v72 = /4\Dp+/igs;
}
main();
