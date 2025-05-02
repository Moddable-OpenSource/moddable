/*---
description: 
flags: [module]
---*/

import storage from "./storage_FIXTURE.js";

const path = "test";
let store = storage.open({path}); 
store.close();
store.close();
store.close();
