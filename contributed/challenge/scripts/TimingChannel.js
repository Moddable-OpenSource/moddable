function* timingChannel() {
  const toChar = (c) => c.toString(36).toUpperCase();

  const fastestChar = (delays) => {
    const pairs = Array.from(delays.entries());
    pairs.sort((a, b) => b[1] - a[1]);
    return pairs[0][0];
  };

  /**
   * @param {string} into
   * @param {number} offset
   * @param {string} char
   * @returns {string}
   */
  const insert = (into, offset, char) =>
    into.slice(0, offset) + char + into.slice(offset + 1, into.length);

  /**
   * @param {string} base
   * @param {number} offset
   * @param {number} c
   * @returns {string}
   */
  const buildCode = (base, offset, c) => {
    // keep the first 'offset' chars of base, set [offset] to c, fill the
    // rest with random-looking junk to make the demo look cool
    // (random-looking, not truly random, because we're deterministic)
    let code = insert(base, offset, toChar(c));
    for (let off2 = offset + 1; off2 < 10; off2++) {
      code = insert(code, off2, toChar((off2 * 3 + c * 7) % 36));
    }
    return code;
  };

  const dateNow = () => {
    try {
      return Date.now();
    } catch (err) {
      if (err instanceof TypeError) {
        // assume we cannot measure time because of err
        return NaN;
      }
      throw err;
    }
  };

  let base = "0000000000";
  while (true) {
    for (let offset = 0; offset < 10; offset++) {
      const delays = new Map();
      for (let c = 0; c < 36; c++) {
        const guessedCode = buildCode(base, offset, c);
        const start = dateNow();
        guess(guessedCode);
        const elapsed = dateNow() - start;
        delays.set(toChar(c), elapsed);
        yield; // allow UI to refresh
        // if our guess was right, then on the last character
        // (offset===9) we never actually reach here, since we guessed
        // correctly earlier, and when the attacker guesses correctly,
        // the defender stops calling go()
      }
//       delays.forEach((value, key) => {
//       	trace(`${key}=${value} `);
//       });
//       trace(`\n`);
      const nextChar = fastestChar(delays);
      base = insert(base, offset, nextChar);
//       trace(`Setting code[${offset}]=${nextChar} -> ${base}\n`);
    }
    trace("we must have measured the timings wrong, try again\n");
  }
}

