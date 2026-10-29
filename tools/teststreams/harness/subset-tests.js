// WPT common/subset-tests.js; teststreams supplies globalThis.location = {search: ""} (every subtest runs) and a test file may set a range. See MODDABLE below.
(function() {
  // MODDABLE: parsed on first use rather than at load, so a test file may assign location.search = "?1-1000"
  // (one imported file per WPT META: variant) before defining its tests.
  var subTestStart, subTestEnd;
  function parseRange() {
    subTestStart = 0;
    subTestEnd = Infinity;
    var match = /(?:^\?|&)(\d+)-(\d+|last)(?:&|$)/.exec(location.search);
    if (match) {
      subTestStart = parseInt(match[1], 10);
      if (match[2] !== "last") {
        subTestEnd = parseInt(match[2], 10);
      }
    }
  }
  function shouldRunSubTest(currentSubTest) {
    if (subTestStart === undefined) parseRange();
    return currentSubTest >= subTestStart && currentSubTest <= subTestEnd;
  }
  var currentSubTest = 0;
  /**
   * Only test a subset of tests with, e.g., `?1-10` in the URL.
   * Can be used together with `<meta name="variant" content="...">`
   * Sample usage:
   * for (const test of tests) {
   *   subsetTest(async_test, test.fn, test.name);
   * }
   */
  function subsetTest(testFunc, ...args) {
    currentSubTest++;
    if (shouldRunSubTest(currentSubTest)) {
      return testFunc(...args);
    }
    return null;
  }
  self.shouldRunSubTest = shouldRunSubTest;
  self.subsetTest = subsetTest;
})();
