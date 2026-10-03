#include "wav.h"
#include <math.h>

f32 generate_sine_sample(ToneGenerator *tone_generator)
{
  f32 sample = tone_generator->amplitude * sin(tone_generator->angle);
  tone_generator->angle += tone_generator->phase_increment;
  return sample;
}
