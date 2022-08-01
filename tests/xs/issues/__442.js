/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/442
flags: [onlyStrict]
---*/

const source = `
// This file was procedurally generated from the following sources:
// - src/dstr-assignment/obj-rest-same-name.case
// - src/dstr-assignment/default/for-of.template
/*---
description: Proper setting in the values for rest name equal to a property name. (For..of statement)
esid: sec-for-in-and-for-of-statements-runtime-semantics-labelledevaluation
features: [object-rest, destructuring-binding]
flags: [generated]
info: |
    IterationStatement :
      for ( LeftHandSideExpression of AssignmentExpression ) Statement

    1. Let keyResult be the result of performing ? ForIn/OfHeadEvaluation(« »,
       AssignmentExpression, iterate).
    2. Return ? ForIn/OfBodyEvaluation(LeftHandSideExpression, Statement,
       keyResult, assignment, labelSet).

    13.7.5.13 Runtime Semantics: ForIn/OfBodyEvaluation

    [...]
    4. If destructuring is true and if lhsKind is assignment, then
       a. Assert: lhs is a LeftHandSideExpression.
       b. Let assignmentPattern be the parse of the source text corresponding to
          lhs using AssignmentPattern as the goal symbol.
    [...]
---*/
var o = {
    x: 42,
    y: 39,
    z: 'cheeseburger'
};

var x, y, z;

var counter = 0;

for ({ x, ...{
      break
      mainloop
    } } of [o]) {

  var keys = Object.getOwnPropertyNames(z);
  counter += 1;
}

`;

assert.throws(SyntaxError, () => eval(source));


