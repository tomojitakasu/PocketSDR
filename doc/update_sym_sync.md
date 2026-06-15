# Bit/Symbol Synchronization Update

Date: 2026-06-14

----

## Summary

The bit/symbol synchronization in `sdr_ch_t` navigation decoding was changed
from an absolute-amplitude threshold on a single epoch to the bit-energy
maximum-likelihood (ML) method of Kokkonen and Pietila. The goal is to make
the synchronized symbol boundary (and therefore `ch->tow`) deterministic,
addressing GitHub issue #83.

The change is contained entirely in `sync_symb()` in `src/sdr_nav.c`. No data
structures, initialization, or per-signal decoders were modified.

## Problem (issue #83)

`sync_symb()` declared symbol sync on the first epoch where:

1. the last 200 ms of IP correlations split into N-ms groups all had a uniform
   sign (`R[i] == +/-N`),
2. the two most recent groups had opposite sign, and
3. the mean IP magnitude of the latest group exceeded `THRES_SYNC = 0.05`.

`THRES_SYNC` is an absolute amplitude. Its effective value scales with front-end
bit width, AGC, and signal level, so it is not a fixed operating point:

- At strong signal (around C/N0 50 dB-Hz) the amplitude gate passes trivially
  and no longer suppresses residual pull-in transients. A half-cycle-slip or
  PLL pull-in transient left in the IP history can mimic a bit transition at an
  arbitrary 1 ms position, so the declared boundary can shift by +/-1 ms between
  runs. For L1C/A this is a silent error: 19 of 20 samples still carry the
  correct sign, so frame sync and parity still pass while `ch->tow` is off by
  1 ms, producing a pseudorange error.
- At weak signal the same fixed threshold can block sync entirely.

Steady-state symbol errors do not explain the strong-signal randomness; at
50 dB-Hz the 1 ms coherent SNR is high and the sign sequence is effectively
error-free. The randomness comes from transients in the evaluation window, not
from the data.

The *silent* failure mode is specific to the 20 ms cases (L1C/A, NavIC I5S): a
1 ms boundary error leaves 19 of 20 samples correct, so frame sync and parity
still pass while `ch->tow` is wrong. The 2 ms cases (SBAS, BDS B1I D2) instead
self-recover through downstream FEC/CRC, because a 1 ms boundary error destroys
the symbol stream rather than shifting it silently. The replacement sync itself
is general and applies to all `sync_symb()` callers (see Properties).

## Method

`sync_symb()` now uses the bit-energy ML method (Kokkonen and Pietila, IEEE
PLANS 2002, pp. 85-90; see also Van Dierendonck, ch. 8 in Parkinson and Spilker,
*Global Positioning System: Theory and Applications*, Vol. I, AIAA, 1996 for the
classical histogram method this replaces).

For each of the N possible boundary phases `m = 0..N-1`, both the coherent and
the incoherent N-ms symbol energy are accumulated over K symbols:

```
E[m] = sum over k of | sum of IP over the N samples of symbol k at phase m |
A[m] = sum over k of   sum of |IP| over the same N samples
```

`E[m]/A[m]` is the coherent purity: 1.0 when every symbol at phase `m` is sign-
consistent, lower as the phase straddles bit transitions. The boundary phase is

```
m0 = argmax_m ( E[m] / A[m] )
```

chosen by purity rather than raw energy, so the choice is scale-invariant even
between phases with unequal |IP| sums. Sync is declared only when all three of
the following hold:

```
coh    = E[m0] / A[m0]                        >= 0.5 (1 + 1/sqrt(N))   (a)
margin = coh - max(coh[m0-1], coh[m0+1])      >= THRES_SYNC_M          (b)
E[m0]  >= THRES_SYNC_R * E[(m0 + N/2) % N]                             (c)
```

(a) requires real coherent structure (rejects noise, whose purity is ~1/sqrt(N)).
(c), the opposite-phase energy ratio, rejects the coarse half-symbol (`N/2`)
error: a half-bit shift fully cancels the coherent sum on a transition, so the
clean-signal ratio is ~2.0 for any N and `THRES_SYNC_R = 1.4` rejects the no-peak
(ratio ~1) case. (c) does **not** by itself separate the +/-1 ms neighbours that
are the core of issue #83, because an adjacent phase still keeps `(N-1)/N` of the
coherent sum. (b), the adjacent-phase coherence margin, is the gate for that 1 ms
ambiguity: `m0` must beat *both* neighbours in purity by `THRES_SYNC_M`. When the
top two phases are within the margin (a lone transition, a near-DC window, or
noise at low C/N0) no boundary is declared; the channel keeps accumulating until
`K` grows enough to separate them, which preserves determinism instead of
committing to a 1 ms-ambiguous estimate.

Because `E[]` and `A[]` are deterministic functions of the IP history, the same
input produces the same `m0`. A single transient sign flip perturbs only the K
windows that contain it; spread over `K` accumulated symbols its effect is
diluted relative to a first-hit test, so it is unlikely to create a spurious peak
or shift the boundary. (The loss is not exactly uniform across phases — it
depends on where the outlier falls relative to each phase's windows.)

### Properties

- Scale-invariant: phase selection uses the per-phase purity `E/A`, and the
  gates use ratios/margins, never an absolute amplitude.
- Deterministic: argmax over accumulated coherent purity (`E/A`) plus an explicit
  confidence margin, so a boundary is committed only when the estimate is
  unambiguous.
- Soft-decision and bit-energy based, so it also synchronizes at lower C/N0
  than the old "all-200-ms-samples-uniform-sign" requirement.
- General across signals (intended, not a side effect): the same scale-invariant
  rule serves every `sync_symb()` caller — L1C/A and I5S (N=20), G1CA (N=10),
  SBAS and B1I D2 (N=2). Unit tests in `test/utest` cover N=2, 10, 20; receiver-
  level behaviour is verified on GLONASS G1CA (N=10) playback. The half-symbol
  contrast (c) is 2:1 for every N; the 1 ms margin (b) is only meaningful for
  N>2 (for N=2 the adjacent and opposite phases coincide).

### Known limitation

`THRES_SYNC_M = 0.03` is a conservative initial value. For random L1C/A data the
structural purity gap between the true phase and its 1 ms neighbour is only about
0.05 (`20/20` vs `19/20`), so the margin must stay well below that to avoid
blocking valid sync, yet be large enough to reject noise-driven ties — a narrow
window that has not yet been tuned against real low-C/N0 IF data. Signals with a
dedicated meander (G1CA, where every 10 ms boundary transitions) have a much
larger gap (~0.2) and are insensitive to the exact value. If weak channels are
seen to stall at symbol sync, lower `THRES_SYNC_M`; if 1 ms boundary jitter
persists at strong signal, raise it.

### Transient guard

The accumulation window is floored at lock count `Lmin = T_SYNC_START / ch->T`,
with `T_SYNC_START = 1.5 s` (equal to `T_NPULLIN` in `src/sdr_ch.c`, where nav
decoding begins). The oldest IP sample used is guaranteed to be at or after
`Lmin`, which is after the PLL pull-in (`T_FPULLIN = 1.0 s`). This excludes the
1.0-1.5 s pull-in transient that was the root cause of the strong-signal
randomness.

### Symbol-lost detection

The periodic branch keeps the original lenient behavior: symbol lost only when
the mean IP magnitude falls below an absolute `THRES_LOST = 0.001`. The real
loss-of-lock is handled by the C/N0 unlock gate in `src/sdr_ch.c`, so this is a
backstop that almost never fires while a channel tracks.

An earlier revision of this change replaced it with a scale-invariant
per-symbol coherent-vs-incoherent ratio (`|mean_IP(N)| >= 0.5 * mean(|IP|)`).
That regressed low-C/N0 frame lock: a single N-ms symbol has high variance, so
at ~30 dB-Hz the ratio dipped below the threshold most symbol periods, resetting
`ssync` every ~N ms. The channel then produced almost no symbols (it cycled
SYMBOL SYNC / SYMBOL LOST instead of adding to the symbol buffer) and frame sync
never completed. Verified on a GLONASS G1CA (N = 10) playback: strong channels
(~49 dB-Hz) were unaffected, weak channels (30-35 dB-Hz) lost frame lock. The
per-symbol ratio test was reverted; the deterministic K-P sync (the actual fix
for issue #83) is unchanged. A scale-invariant lost detector would have to
accumulate over several symbols (or use hysteresis) to avoid this thrashing.

## Parameters

Defined at the top of `src/sdr_nav.c`:

| Name | Value | Meaning |
|---|---|---|
| `SYNC_NMAX` | 20 | max symbol length (ms); sizes the phase and prefix-sum arrays |
| `K_SYNC_MIN` | 20 | min symbols accumulated before sync may be declared |
| `K_SYNC_MAX` | 100 | max symbols accumulated (caps lookback and stack use) |
| `THRES_SYNC_R` | 1.4 | sync energy ratio, peak vs opposite (half-symbol) phase |
| `THRES_SYNC_M` | 0.03 | min coherence margin of the peak over its 1 ms neighbours |
| `THRES_LOST` | 0.001 | symbol-lost absolute threshold (kept from original) |
| `T_SYNC_START` | 1.5 | accumulation history floor (s), equals `T_NPULLIN` |

Removed: the old `THRES_SYNC = 0.05` absolute sync threshold in `src/sdr_nav.c`,
replaced by the K-P energy ratio. `THRES_LOST = 0.001` is retained for the
symbol-lost backstop. (The identically named macros in `src/sdr_ch.c` for
secondary-code sync are unrelated and unchanged.)

## Implementation notes

- The K-P metric is computed statelessly from the existing `ch->trk->P[]`
  history (`SDR_N_HIST = 5000` deep, i.e. 250 symbols at N = 20), so no new
  `sdr_nav_t` field is required.
- Window sums use two prefix-sum arrays over the lookback range (one for IP, one
  for |IP|, giving `E[m]` and `A[m]`), reducing the cost from O(N^2 K) to O(N K)
  per epoch. The heavy branch runs only while a channel is unsynced
  (`ssync == 0`), a short startup transient.
- The boundary is stored as `ch->nav->ssync = ch->lock - m0 - N`, the same
  convention as the old code (the old value `ch->lock - N` is the `m0 = 0`
  case). Only `ssync mod N` and `ssync != 0` are significant for the five
  `sync_symb()` callers; the absolute `ssync` timestamp used by the `+1104`
  and `+6680` checks belongs to the separate `sync_sec_code()` path (G1OCD,
  G3OCD) and is unaffected.

## Trade-offs

- Time to first symbol sync increases by at least `K_SYNC_MIN` symbols, and
  further if the adjacent-phase margin needs more accumulation to clear
  `THRES_SYNC_M` (the window grows with lock up to `K_SYNC_MAX`). For L1C/A this
  is roughly 0.3-0.5 s later than before at strong signal, more at low C/N0.
- Parameter values (`THRES_SYNC_R = 1.4`, `THRES_SYNC_M = 0.03`,
  `K_SYNC_MIN = 20`, `K_SYNC_MAX = 100`) are analysis-based initial settings.
  They should be reviewed against the issue #83 reproduction data, in particular
  the observed distributions of the opposite-phase energy ratio and the
  adjacent-phase margin at high and low C/N0.

## Verification

Unit tests (MSYS2 UCRT64, the project toolchain):

```
make -C test/utest test           # full suite, or:
make -C test/utest test_sdr_nav   # nav decoder only, then ./test_sdr_nav.exe
```

`test_sdr_nav` covers symbol sync for N = 20 (L1C/A), 10 (G1CA) and 2 (SBAS),
the adjacent-margin rejection of a flat (transition-free) window, and the
symbol-lost backstop.

The `$LOG ... SYMBOL SYNC (ratio, coh, margin)` message reports the peak-to-
opposite energy ratio (capped at 99.99 when the opposite phase has no energy),
the coherent purity `coh`, and the adjacent-phase `margin`. Receiver-level
acceptance against the issue #83 reproduction is still the deciding test: across
repeated runs of the same IF data, `ssync mod N` must be identical at strong
signal, the three logged values stable, and weak channels must still reach sync.
The same run is used to tune `THRES_SYNC_M` and `THRES_SYNC_R`.
