# PrimeEight v133 — PrimalTap functional GUI audit
Source: Soundtoys PrimalTap v5 user guide (https://store.soundtoys.com/wp-content/uploads/PrimalTap-Manual.pdf) and https://www.soundtoys.com/product/primaltap/
Reference only; do not copy Soundtoys trade dress, artwork or branding.

## Implemented in PrimeEight (src/PrimeEightProcessor.h)
Parameter IDs are stable and must not be reordered:
0 Multiply (0..3 mapped to 1x/2x/4x/8x)
1 Tap 1 delay (ms) (0..1023)
2 Tap 2 delay (ms) (0..1023)
3 Tap 1 level (-2..2)
4 Tap 2 level (-2..2)
5 Feedback (-0.95..0.95)
6 Mix (0..1)
7 Repeat Hold (0..1)
8 Mod rate (Hz) (0..20)
9 Mod depth (ms) (0..20)
10 Smoothing (ms) (0..500)

## v133 first UI scope
Keep the confirmed-working Win32 CLAP lifecycle/parenting code from checkpoint/mulab-working-v132.
Render and expose exactly the existing 11 parameters, organized as:
- Header: original protected ape/8 SVG, PrimeEight wordmark
- Dual delay: Tap 1 and Tap 2 time and level
- Clock: four-position Multiply
- Regeneration: Feedback and Repeat Hold
- Modulation: Rate and Depth
- Output: Mix
- Advanced: Smoothing
- Value labels and host parameter refresh.
All GUI gestures must propagate to the host via CLAP host params events/flush and reflect host automation and state restoration. Do not merely alter UI state.

## Features present in PrimalTap but not in current PrimeEight DSP
Do not draw active controls for these until separately designed and implemented:
- Time/beat selection, independent tempo sync per delay
- Input drive saturation
- Adjustable VCO/Adjust manual sweep
- Feedback algorithm selector (classic, parallel, series, ping-pong, reverb)
- Per-tap modulation depth/phase and LFO waveshape
- A/B link and further tweak-panel controls

## Visual constraints
Preserve the user's approved flat, texture-free, gradient-free GUI mockup and exact unmodified PrimeEight_Original_Ape8_Logo.svg (path11.svg). The artwork is not currently committed to this branch; obtain original source asset before integrating. Do not recreate or approximate it.

## Release/testing
Keep checkpoint/mulab-working-v132 unchanged. Windows CLAP GUI must be tested in MuLab 10.2.37 and REAPER; Linux remains headless until a separate GUI backend exists. DSP changes require separate tests. Preserve all existing parameter IDs and state compatibility.
