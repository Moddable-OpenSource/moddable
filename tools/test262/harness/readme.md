# test262 harness files

Copies of the [test262](https://github.com/tc39/test262) harness files that tests in `$MODDABLE/tests` name in their `includes:` front matter. They are used by the xsdb test runner when no test262 repository is configured (`set test262 <path>`). When a test262 repository is configured, its `harness` directory is used instead, so these copies are a fallback only.

`assert.js`, `sta.js`, and `compareArray.js` are not here because the test apps build them in (see `../harness.js`); the runner never sends them.

Copied from test262 commit `45e740928f` (2024-02-16). These files are governed by the test262 license found in that repository.

To refresh, copy the files from the `harness` directory of a current test262 checkout.
