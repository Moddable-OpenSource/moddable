/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/449
flags: [onlyStrict]
---*/

// This file was procedurally generated from the following sources:
// - src/dstr-assignment-for-await/array-elem-init-fn-name-gen.case
// - src/dstr-assignment-for-await/default/async-func-decl.template
/*---
description: Assignment of function `name` attribute (GeneratorExpression) (for-await-of statement in an async function declaration)
esid: sec-for-in-and-for-of-statements-runtime-semantics-labelledevaluation
features: [generators, destructuring-binding, async-iteration]
flags: [generated, async]
includes: [propertyHelper.js]
info: |
    IterationStatement :
      for await ( LeftHandSideExpression of AssignmentExpression ) Statement

    1. Let keyResult be the result of performing ? ForIn/OfHeadEvaluation(« »,
       AssignmentExpression, iterate).
    2. Return ? ForIn/OfBodyEvaluation(LeftHandSideExpression, Statement,
       keyResult, assignment, labelSet).

    13.7.5.13 Runtime Semantics: ForIn/OfBodyEvaluation

    [...]
    5. If destructuring is true and if lhsKind is assignment, then
       a. Assert: lhs is a LeftHandSideExpression.
       b. Let assignmentPattern be the parse of the source text corresponding to
          lhs using AssignmentPattern as the goal symbol.
    [...]

    AssignmentElement[Yield] : DestructuringAssignmentTarget Initializeropt
    [...] 7. If Initializer is present and value is undefined and
       IsAnonymousFunctionDefinition(Initializer) and IsIdentifierRef of
       DestructuringAssignmentTarget are both true, then
       a. Let hasNameProperty be ? HasOwnProperty(v, "name").
       b. If hasNameProperty is false, perform SetFunctionName(v, GetReferencedName(lref)).

---*/

const source = `
let xGen, gen;

let iterCount = 0;
async function fn() {
  for await ([ (), gen = function*() {} ] of [[]]) {

      enumerable: false,

    iterCount += 1;
  }
}

let promise = fn();

promise
`;

assert.throws(SyntaxError, () => eval(source));
