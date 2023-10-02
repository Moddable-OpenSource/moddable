import wow from "./wow.js";

console.log(wow);

await new Promise(resolve => setTimeout(resolve, 500));
console.log("done waiting");
