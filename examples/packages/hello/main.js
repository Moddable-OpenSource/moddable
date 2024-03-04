import test from "#test";

import module0 from "./module0.js";
import module1 from "@moddable/example-hello1/mod.js";
import main2 from "@moddable/example-hello2";

console.log(test);
console.log(module0);
console.log(module1);
console.log(main2);

import Mustache from "mustache";

const view = {
  title: "Joe",
  calc: function () {
    return 2 + 4;
  }
};

const output = Mustache.render("{{title}} spends {{calc}}", view);
console.log(`${output}\n`);
