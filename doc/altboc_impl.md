# Galileo E5 AltBOC Implementation Notes

This note summarizes the current PocketSDR implementation for Galileo E5
wideband AltBOC tracking.

## Scope

The implemented signal is `E5ABQ`.

`E5ABQ` is a pilot-only, sign-only AltBOC approximation that combines the
Galileo E5aQ and E5bQ pilot components at the E5 center frequency:

- RF frequency: `1191.795 MHz`
- Code period: `1 ms`
- Base code length: `10230 chips`
- Internal complex replica length: `40920 samples/chips`
- RINEX observation code mapping: `CODE_L8Q`

The design intentionally does not implement ICD-perfect full E5 AltBOC with
data-assisted product-term wipeoff.

## Signal Model

The receiver uses a complex spreading-code replica. Each E5 code chip is
expanded by four AltBOC subchips. The E5aQ and E5bQ pilot codes are modulated by
periodic sign-only subcarrier tables:

- E5aQ complex subcarrier: `ALTBOC_E5A_I`, `ALTBOC_E5A_Q`
- E5bQ complex subcarrier: `ALTBOC_E5B_I`, `ALTBOC_E5B_Q`

After secondary-code synchronization, the tracking replica is:

```text
E5ABQ = E5aQ_altboc + rel * E5bQ_altboc
```

where:

```text
rel = E5aQ_secondary[current_ms] * E5bQ_secondary[current_ms]
```

The combined I/Q values are sign-limited to `{-1, 0, +1}` so the existing
8-bit complex correlator path can be reused.

## Acquisition

Acquisition uses the E5aQ AltBOC complex replica only. This avoids requiring
secondary-code alignment before signal detection.

Once the signal is acquired, the channel starts tracking with the same E5aQ
AltBOC replica until the E5aQ secondary code is synchronized.

## Tracking

Tracking uses three precomputed code banks:

1. Bank 0: E5aQ AltBOC replica, used before secondary-code sync.
2. Bank 1: E5aQ + E5bQ combined replica for `rel = +1`.
3. Bank 2: E5aQ + E5bQ combined replica for `rel = -1`.

At each 1 ms tracking epoch, if the E5aQ secondary code is synchronized, the
receiver selects bank 1 or bank 2 from the current E5aQ/E5bQ secondary-code
relative sign.

The correlator uses a complex-code dot product:

```text
corr = data * conj(code)
```

This is required because AltBOC replicas have distinct I and Q code values.

## Navigation Decoding

`E5ABQ` is routed through the existing E5aQ pilot navigation path. It is used as
a tracking and observation signal, not as an E5aI/E5bI data-channel decoder.

Data-channel demodulation remains the responsibility of the existing `E5AI` and
`E5BI` channels.

## Rejected Variants

### `E5IQ`

An earlier Tier 2 signal name, `E5IQ`, represented E5aQ-only AltBOC tracking.
It was useful for bring-up, but was removed from the public signal list after
`E5ABQ` became stable.

The E5aQ-only AltBOC replica still exists internally as the acquisition and
pre-secondary-sync fallback for `E5ABQ`.

### `E5ABQF`

An experimental full AltBOC-Q signal, `E5ABQF`, was also tested. It included
the E5 product terms and used four E5aI/E5bI data-sign hypotheses.

This was rejected because full AltBOC product terms require reliable wipeoff of
the E5aI and E5bI navigation data signs. Without data-assisted wipeoff, the
tracking loop sees unstable phase changes and split correlation peaks.

Supporting full AltBOC robustly would require sharing data-sign state from
separate E5AI/E5BI channels or adding a dedicated data-assisted wideband
tracking architecture. That is intentionally out of scope for the current
implementation.

## Expected Behavior

Compared with E5aQ or E5aI alone, `E5ABQ` can use both E5aQ and E5bQ pilot
power. The ideal pilot-power gain is about `3 dB` over a single pilot component.

In practice, the measured gain can be smaller because of:

- frontend passband ripple and roll-off,
- E5a/E5b gain imbalance,
- sign-only replica approximation,
- receiver C/N0 estimator assumptions,
- finite front-end bandwidth.

The implementation should therefore be judged primarily by stable lock,
consistent code offset versus E5a/E5b sideband channels, and a clean single
correlation peak.

