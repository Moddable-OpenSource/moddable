/*---
description: Math.irandom is spread uniformly over its range
info: |
  These are statistical checks on the platform random number generator behind
  Math.irandom, which is where conformance tends to be weakest. The thresholds are
  chosen so that a uniform generator fails with probability below 1e-12, while a
  generator that is stuck, band limited, skewed, or short of period fails by orders
  of magnitude. They are deliberately loose: this test is meant to catch a broken
  generator, not to grade a good one.
flags: [onlyStrict]
---*/

const SPAN = 2147483647;		// the range of Math.irandom() with no arguments
const BUCKETS = 16;
const SAMPLES = 16000;			// 1000 per bucket
const CHI2_LIMIT = 90;			// chi-square, 15 degrees of freedom, p < 1e-12

function chiSquare(counts, samples) {
	const expected = samples / counts.length;
	let total = 0;
	for (let i = 0; i < counts.length; i++) {
		const difference = counts[i] - expected;
		total += (difference * difference) / expected;
	}
	return total;
}

function tally(draw) {
	const counts = new Array(BUCKETS).fill(0);
	for (let i = 0; i < SAMPLES; i++)
		counts[draw()] += 1;
	return counts;
}

// A small range, which exercises the top of the generator's value.
{
	const counts = tally(function () { return Math.irandom(BUCKETS); });
	for (let i = 0; i < BUCKETS; i++)
		assert(counts[i] > 0, "irandom(" + BUCKETS + ") never returned " + i);

	const chi2 = chiSquare(counts, SAMPLES);
	assert(chi2 < CHI2_LIMIT,
		"irandom(" + BUCKETS + ") is not uniform: chi-square " + chi2 + " over " + SAMPLES +
		" samples exceeds " + CHI2_LIMIT + ", counts " + counts.join(","));
}

// The full default range, bucketed, which exercises the whole of the generator.
{
	const width = SPAN / BUCKETS;
	const counts = tally(function () { return Math.floor(Math.irandom() / width); });
	for (let i = 0; i < BUCKETS; i++)
		assert(counts[i] > 0, "irandom() never landed in bucket " + i + " of " + BUCKETS);

	const chi2 = chiSquare(counts, SAMPLES);
	assert(chi2 < CHI2_LIMIT,
		"irandom() is not uniform: chi-square " + chi2 + " over " + SAMPLES +
		" samples exceeds " + CHI2_LIMIT + ", counts " + counts.join(","));
}

// A generator stuck in a band reaches neither end of the range.
{
	let low = SPAN, high = 0;
	for (let i = 0; i < 1000; i++) {
		const value = Math.irandom();
		if (value < low) low = value;
		if (value > high) high = value;
	}
	assert(low < SPAN / BUCKETS,
		"irandom() never reached the bottom of its range in 1000 samples: lowest was " + low);
	assert(high > SPAN - (SPAN / BUCKETS),
		"irandom() never reached the top of its range in 1000 samples: highest was " + high);
}

// A generator with a short period repeats itself long before 256 draws from a
// range of two billion should collide.
{
	const seen = new Set();
	for (let i = 0; i < 256; i++)
		seen.add(Math.irandom());
	assert(seen.size >= 200,
		"irandom() returned only " + seen.size + " distinct values in 256 draws, which suggests a short period");
}
