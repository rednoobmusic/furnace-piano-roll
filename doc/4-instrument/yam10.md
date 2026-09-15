# YAM10 instrument editor

the YAM10 editor is laid out as a grid of operator panels. the number of columns follows the FM layout setting, so it is two wide by default and three across if you have picked one of the wide layouts.

## YAM10 tab

each operator gets a panel containing:

- **A, D, S, D2, R**: the envelope rates as vertical sliders, with the sustain level sitting where the sustain position setting puts it. attack, decay and decay 2 run 0 to 31; sustain and release run 0 to 15.
- **Scale Rate**: key scale rate. higher notes run their envelopes faster.
- **Waveform**: a picture of the operator's wave, the picker under it, and a wavetable toggle with the wavetable number beside it. the picture is drawn from the chip's own waveform table, so it is always what you will hear, and it shows one cycle of the note whatever the waveform is. the picker lists all 77 waveforms grouped by family, each row drawn the same way, with its number beside the name because that is what the waveform effects take. under the picker is the pulse width, which only the pulse waveform reads and which is greyed out for every other one.
- **Envelope**: the resulting envelope shape.
- **TL** and **OL**: total level and output level. TL sets how loud the operator runs, at 0.75 dB a step. OL is a linear gain and decides whether the operator is heard at all: anything above zero makes it a carrier.
- **Multiplier**, **Detune**, **Fine**, **EnvScale**, **Pan**: the operator's tuning and placement. detune is in semitones, fine is in cents, pan is 0 to 255 with 128 in the centre.
- **Modulation Input**: six checkboxes choosing which operators feed this one. the box carrying the operator's own number is labelled SELF and gives self-feedback.
- **Feedback**: self-feedback depth, available on every operator.
- **Delay**: how long the operator waits before its attack begins. 0 is none, and each step doubles the wait up to 659 ms.
- **PR**: the phase reset timer, in engine ticks, so it stays on tempo. 0 switches it off.
- **Fixed** with **Blk** and **F**: fixed pitch. the operator ignores the note and holds `F * 2^Blk / 8` hertz, shown underneath. 0 holds the phase still.

clicking an operator's name switches it on or off.

## DSP tab

the per-channel chain. three filters in series, each with an enable, a mode, a cutoff and a resonance; then distortion, chorus, reverb, a three band compressor, an EQ, phase inversion and echo. every readout shows the real value, so the frequencies are in hertz, the times in milliseconds, the gains in decibels and the resonances as a Q.

the EQ is drawn as a curve. it starts with no bands at all: click anywhere on the graph to place one where you clicked. a band placed in the bottom quarter of the range, under about 112 Hz, starts as a low shelf, one in the top quarter, over about 3.5 kHz, starts as a high shelf, and anything between them starts as a peak. drag a node to move its frequency and gain, roll the wheel over it to change its Q, and right click it to pick the shape or take it away. the row underneath edits whichever band is selected. eight bands fit.

## macros

- **FM Macros**: what the chip carries channel wide. the operator mask, which switches operators on and off, and one feedback that applies to all six at once.
- **DSP Macros**: the DSP chain. filter cutoff and resonance for all three filters, distortion gain and level, chorus mix, rate and depth, and echo mix and feedback.
- **OP1 to OP6 Macros**: per operator. level, attack, decay, decay 2, sustain, release, multiplier, detune, envelope scale, waveform, key scale rate, feedback, output level, modulation input, panning, fine detune and delay.
- **Macros**: volume, arpeggio, pitch, panning and phase reset.
