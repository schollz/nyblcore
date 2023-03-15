// Sampler Player
// InA() -> selects sample
// InB() -> gates
// InK() -> also selects sample

// make breakbeat

#include "/home/zns/Arduino/nyblcore/nyblcore.h"
#include "/home/zns/Arduino/nyblcore/generated-breakbeat-table.h"

#define SHIFTY 6
#define PARM1 60
#define PARM2 120
#define PARM3 180

byte noise_gate = 0;
byte distortion = 0;
byte volume_reduce = 0;  // volume 0 to 6
byte volume_mod = 0;
byte thresh_counter = 0;
byte thresh_next = 3;
byte thresh_nibble = 0;
word phase_sample = 0;
word phase_sample_last = 11;
byte select_sample = 0;
byte select_sample_start = 0;
byte select_sample_end = NUM_SAMPLES - 1;
bool direction = 1;       // 0 = reverse, 1 = forward
bool base_direction = 0;  // 0 = reverse, 1 == forward
byte retrig = 4;
byte tempo = 5;
word gate_counter = 0;
word gate_thresh = 16;  // 1-16
word gate_thresh_set = 65000;
bool gate_on = false;
int audio_last = 0;
int audio_next = -1;
byte audio_now = 0;
char audio_add = 0;
byte stretch_amt = 0;
word stretch_add = 0;
byte knobK_last = 0;
byte knobB_last = 0;
byte knobA_last = 0;
byte left = 0;
byte right = 0;
byte probability = 0;
bool do_retrigger = false;
bool do_stutter = false;
bool do_stretch = false;
int r1i = 123798;
int r2i = 123879;
int r3i = 223879;

#define NUM_TEMPOS 12
byte *tempo_steps[] = {
  (byte[]){ 0x99, 0x99, 0x99 },
  (byte[]){ 0x88, 0x88, 0x88 },
  (byte[]){ 0x77, 0x77, 0x77 },
  (byte[]){ 0x66, 0x66, 0x66 },
  (byte[]){ 0x65, 0x65, 0x65 },
  (byte[]){ 0x55, 0x55, 0x55 },
  (byte[]){ 0x54, 0x54, 0x54 },
  (byte[]){ 0x44, 0x44, 0x44 },
  (byte[]){ 0x43, 0x44, 0x43 },
  (byte[]){ 0x43, 0x43, 0x43 },
  (byte[]){ 0x33, 0x33, 0x33 },  // base tempo
  (byte[]){ 0x32, 0x32, 0x32 },
  (byte[]){ 0x22, 0x22, 0x22 },
};

void Setup() {
}

void Loop() {
  byte knobA = InA();
  byte knobB = InB();
  byte knobK = InK();

  if (gate_on == false && phase_sample_last != phase_sample) {
    audio_last = ((int)pgm_read_byte(SAMPLE_TABLE + phase_sample)) << SHIFTY;
    if (thresh_next > thresh_counter) {
      audio_next = ((int)pgm_read_byte(SAMPLE_TABLE + phase_sample + (direction * 2 - 1))) << SHIFTY;
      audio_next = (audio_next - audio_last) / ((int)(thresh_next - thresh_counter));
    } else {
      audio_next = 0;
    }
    audio_add = 0;
    phase_sample_last = phase_sample;
  }
  // no interpolation
  // OutF(audio_last >> SHIFTY);

  // Moctal(knobK);  // 10100101

  if (knobK_last != knobK) {
    knobK_last = knobK;
    // update the left/right parameter setting
    left = (byte)(thresh_next & 0xF0) >> 4;
    right = (byte)(thresh_next & 0x0F);
  }
  if (knobA_last != knobA) {
    knobA_last = knobA;
    // update the left parameter
    if (left < PARM1) {
      // volume
      if (knobA <= 128) {
        volume_reduce = (128 - knobA) * 10 / 128;  // 0-128 -> 10-0
        distortion = 0;
      } else {
        volume_reduce = 0;
        distortion = (knobA - 128) * 60 / 128;  // 128-255 -> 0-60
      }
    } else if (left < PARM2) {
      noise_gate = knobA * 60 / 255;  // 0-255 -> 0-60
    } else if (left < PARM3) {
      probability = knobA * 100 / 255;  // 0-255 -> 0-100
    } else {
      do_stretch = knobA > 128;
    }
  }
  if (knobB_last != knobB) {
    knobB_last = knobB;
    // update the right parameter
    if (right < PARM1) {
      if (knobB < 128) {
        tempo = knobB * NUM_TEMPOS / 128;
        base_direction = 0;  // reverse
      } else {
        tempo = (knobB - 128) * NUM_TEMPOS / 128;
        base_direction = 1;  // forward
      }
    } else if (right < PARM2) {
      gate_thresh = (knobB * 4 / 256 + 1) * 4;
    } else if (right < PARM3) {
      do_retrigger = knobA > 128;
    } else {
      do_stutter = knobB > 128;
    }
  }



  // linear interpolation with shifts
  audio_now = (audio_last + audio_add) >> SHIFTY;

  // mute audio if volume_reduce==10
  if (volume_reduce == 10) audio_now = 128;

  // if not muted, make some actions
  if (audio_now != 128) {
    // noise gating
    if (noise_gate > 0) {
      if (audio_now > 128) {
        if (audio_now > 128 + noise_gate) {
          audio_now -= noise_gate;
        } else {
          audio_now = 128;
        }
      } else {
        if (audio_now < 128 - noise_gate) {
          audio_now += noise_gate;
        } else {
          audio_now = 128;
        }
      }
    }
    // distortion / wave-folding
    if (distortion > 0) {
      if (audio_now > 128) {
        if (audio_now < (255 - distortion)) {
          audio_now += distortion;
        } else {
          audio_now = 255 - distortion;
        }
      } else {
        if (audio_now > distortion) {
          audio_now -= distortion;
        } else {
          audio_now = distortion - audio_now;
        }
      }
    }
    // reduce volume
    if ((volume_reduce + volume_mod) > 0) {
      if (audio_now > 128) {
        audio_now = ((audio_now - 128) >> (volume_reduce + volume_mod)) + 128;
      } else {
        audio_now = 128 - ((128 - audio_now) >> (volume_reduce + volume_mod));
      }
    }
  }
  OutF(audio_now);
  // for linear interpolation
  audio_add = audio_add + audio_next;

  thresh_counter++;
  if (thresh_counter == thresh_next) {
    thresh_counter = 0;
    thresh_nibble++;
    if (thresh_nibble >= 6) {
      thresh_nibble = 0;
    }
    thresh_next = tempo_steps[tempo][thresh_nibble / 2];
    if (thresh_nibble % 2 == 0) {
      thresh_next = (byte)(thresh_next & 0xF0) >> 4;
    } else {
      thresh_next = (byte)(thresh_next & 0x0F);
    }
    // do stretching
    if (do_stretch) {
      stretch_amt++;
      if (stretch_amt > 10) stretch_amt = 10;
      thresh_next = thresh_next + stretch_amt;
    }


    // determine directions
    phase_sample += (direction * 2 - 1);
    if (phase_sample > pos[NUM_SAMPLES]) {
      phase_sample = 0;
    } else if (phase_sample < 0) {
      phase_sample = pos[NUM_SAMPLES];
    }


    // specify the gating
    if (gate_thresh < 16) {
      gate_counter++;
      if (gate_counter > gate_thresh_set) {
        gate_on = true;
      } else {
        gate_on = false;
      }
    }

    if (phase_sample % retrigs[retrig] == 0) {
      // figure out randoms
      r1i = (r1i * 32719 + 3) % 32749;
      r2i = (r2i * 32719 + 3) % 32749;
      r3i = (r3i * 32719 + 3) % 32749;
      byte r1 = (byte)r1i;
      byte r2 = (byte)r2i;
      byte r3 = (byte)r3i;

      if (volume_mod > 0) {
        if (volume_mod > 3 || r3 < 90) {
          volume_mod--;
        }
      } else {
        // randomize direction
        if (r1 < probability) {
          direction = 1 - base_direction;
        } else {
          direction = base_direction;
        }

        // random retrig
        if (r2 < probability / 4) {
          retrig = 6;
        } else if (r2 < probability / 3) {
          retrig = 5;
        } else if (r2 < probability / 2) {
          retrig = 3;
        } else if (r2 < probability) {
          retrig = 2;
        } else {
          retrig = 4;
        }

        // select new sample based on direction
        if (direction == 1) select_sample++;
        if (direction == 0) {
          if (select_sample == 0) {
            select_sample = select_sample_end;
          } else {
            select_sample--;
          }
        }


        // make sure the new sample is not out of bounds
        if (select_sample < select_sample_start) select_sample = select_sample_end;
        if (select_sample > select_sample_end) select_sample = select_sample_start;

        // random jump
        if (r3 < probability) {
          thresh_next = thresh_next + ((r1 - r3) * 4 / 255);
          retrig = ((r1 - r2) * 6 / 255);
          select_sample = (r3 * NUM_SAMPLES) / 60;
        }

        // setup the gating
        gate_counter = 0;
        if (gate_thresh < 16) {
          gate_thresh_set = (retrigs[retrig] * gate_thresh) / 16;
        }


        if (do_stutter) {
          volume_mod = 5;
          retrig = 6;
        }
      }

      // set new phase
      if (select_sample % 2 == 0) {
        LedOn();
      } else {
        LedOff();
      }
      phase_sample = pos[select_sample];
    }
  }
}
