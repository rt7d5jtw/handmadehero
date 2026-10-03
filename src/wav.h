#pragma once
#include "base.h"

// 1. Sampling denotes the number of samples taken per second.
// 2. Sampling is the reduction of a continuous-time signal to a discrete-time
// signal.

// Sampling can be thought of as the audio waveform amplitude modulating a
// series of narrow pulses or needles, to produce a pulse-amplitude-modulated
// (PAM) signal.

// Sampling rate or sampling frequency defines the number of samples per second
// (or per other unit) taken from a continous signal to make a discrete or
// digital signal.

// For time-domain signals like the waveforms for sound (and other audio-visual
// content types), frequencies are measured in in hertz (Hz) or cycles per
// second.

// The Nyquist–Shannon sampling theorem (Nyquist principle) states that perfect
// reconstruction of a signal is possible when the sampling frequency is greater
// than twice the maximum frequency of the signal being sampled. For example, if
// an audio signal has an upper limit of 20,000 Hz (the approximate upper limit
// of human hearing), a sampling frequency greater than 40,000 Hz (40 kHz) will
// avoid aliasing and allow theoretically perfect reconstruction.

// Bit depth determines the number of possible amplitude values we can record
// for each audio sample. The higher the bit depth, the more amplitude values
// per sample are captured to recreate the original audio signal. The most
// common audio bit depths are 16-bit, 24-bit, and 32-bit. Each is a binary
// term, representing a number of possible values. Systems of higher audio bit
// depths are able to express more possible values:
// 	* 16-bit: 65,536 values
//	* 24-bit: 16,777,216 values
//	* 32-bit: 4,294,967,296 values

// https://web.archive.org/web/20220930103235/https://ccrma.stanford.edu/courses/422-winter-2014/projects/WaveFormat/
// https://web.archive.org/web/20180305160421/https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html

// https://web.archive.org/web/20220817004634/http://netghost.narod.ru/gff/graphics/summary/micriff.htm
// https://johnloomis.org/cpe102/asgn/asgn1/riff.html

typedef struct RIFFHeader RIFFHeader;
PACK(struct RIFFHeader {
    u32 riff_id;  // "RIFF"
    u32 filesize; // file size - 8 bytes
    u32 wave_id;  // "WAVE"
});

typedef struct RIFFChunk RIFFChunk;
PACK(struct RIFFChunk {
    u32 id;   // e.g. "fmt ", "data", "LIST", "JUNK"
    u32 size; // size of the data that follows this header
});

typedef struct WAVEChunk WAVEChunk;
PACK(struct WAVEChunk {
  u16 audio_format;    // 1 = PCM (Uncompressed)
  u16 num_channels;    // 1 = Mono, 2 = Stereo
  u32 sample_rate;     // e.g., 44100, 48000
  u32 byte_rate;       // sample_rate * num_channels * bits_per_sample / 8
  u16 block_align;     // num_channels & bits_per_sample / 8
  u16 bits_per_sample; // e.g., 16, 24, 32
});

typedef struct ToneGenerator ToneGenerator;
struct ToneGenerator {
  f32 frequency;
  f32 amplitude;
  f64 angle;
  f64 phase_increment;
};
