// Sampler Player
// InA() -> selects sample
// InB() -> gates
// InK() -> also selects sample

// make breakbeat

#include "/home/zns/Arduino/nyblcore/jerboa.h"
#include "/home/zns/Arduino/nyblcore/random.h"
#include "/home/zns/Arduino/nyblcore/generated-breakbeat-table.h"

#define SHIFTY 6

byte noise_gate = 0;
byte distortion = 0;
byte volume_reduce = 2;  // volume 0 to 6
byte volume_mod = 0;
byte thresh_counter = 0;
byte thresh_next = 3;
byte thresh_nibble = 0;
word phase_sample = 0;
word phase_sample_last = 11;
byte select_sample = 0;
byte select_sample_start = 0;
byte select_sample_end = NUM_SAMPLES - 1;
byte direction = 1;  // 0 = reverse, 1 = forward
byte retrig = 4;
byte tempo = 7;
word gate_counter = 0;
word gate_thresh = 16;  // 1-16
word gate_thresh_set = 65000;
bool gate_on = false;
int audio_last = 0;
int audio_next = -1;
byte audio_now = 0;
char audio_add = 0;
byte stretch_amt = 0;
byte stretch_max = 0;
word stretch_time = 0;
word stretch_add = 0;
byte r3;

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
  RandomSetup();
}

void Loop() {
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

  // linear interpolation with shifts
  audio_now = (audio_last + audio_add) >> SHIFTY;
  if (noise_gate > 0) {
    if (audio_now > 128) {
      if (audio_now > 128 + noise_gate) {
        audio_now -= noise_gate;
      } else {
        audio_now = 128;
      }
    }
    if (audio_now < 128) {
      if (audio_now < 128 - noise_gate) {
        audio_now += noise_gate;
      } else {
        audio_now = 128;
      }
    }
  }
  if (distortion > 0) {
    if (audio_now > 128) {
      if (audio_now < (255 - distortion)) {
        audio_now += distortion;
      } else {
        // fold
        audio_now = 255 - distortion;
      }
    }
    if (audio_now < 128) {
      if (audio_now > distortion) {
        audio_now -= distortion;
      } else {
        // fold
        audio_now = distortion - audio_now;
      }
    }
  }
  if ((volume_reduce + volume_mod) > 0) {
    if (audio_now > 128) {
      audio_now = ((audio_now - 128) >> (volume_reduce + volume_mod)) + 128;
    } else if (audio_now < 128) {
      audio_now = 128 - ((128 - audio_now) >> (volume_reduce + volume_mod));
    }
  }
  OutF(audio_now);
  audio_add = audio_add + audio_next;

  thresh_counter++;
  if (thresh_counter == thresh_next) {
    thresh_counter = 0;
    thresh_nibble++;
    if (thresh_nibble >= 6) {
      // update the tempo
      // TODO
      // tempo = linlin(InK(),0,255,0,12);
      thresh_nibble = 0;
    }
    thresh_next = tempo_steps[tempo][thresh_nibble / 2];
    if (thresh_nibble == 0 || thresh_nibble == 2 || thresh_nibble == 4) {
      thresh_next = (byte)(thresh_next & 0xF0) >> 4;
    } else {
      thresh_next = (byte)(thresh_next & 0x0F);
    }
    // do stretching
    if (stretch_time > 0) {
      stretch_time--;
      if (stretch_time % stretch_add == 0) {
        stretch_amt++;
        if (stretch_amt > stretch_max) stretch_amt = stretch_max;
      }
      thresh_next = thresh_next + stretch_amt;
    }


    LedOn();

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
      if (volume_mod > 0) {
        if (volume_mod >3 || r3 < 90) {
          volume_mod--;
        }
      } else {
        byte r1 = RandomByte();
        byte r2 = RandomByte();

        // randomize direction
        if (r1 < 60) {
          direction = 0;
        } else {
          direction = 1;
        }

        // random retrig
        if (r2 < 15) {
          retrig = 6;
        } else if (r2 < 30) {

          retrig = 5;
        } else if (r2 < 45) {
          retrig = 3;
        } else if (r2 < 60) {
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
        if (r3 < 15) {
          thresh_next = thresh_next + linlin(r1 - r3, 0, 255, 0, 4);
          retrig = linlin(r1 - r2, 0, 255, 0, 6);
          select_sample = linlin(r3, 0, 60, 0, NUM_SAMPLES);
        }

        // setup the gating
        gate_counter = 0;
        if (gate_thresh < 16) {
          gate_thresh_set = (retrigs[retrig] * gate_thresh) / 16;
        }


        byte audio = InA();
        if (audio < 128) {
          // activate stretch
          // stretch_amt = 0;
          // stretch_time = retrigs[retrig] * 2;
          // stretch_max = 10;
          // stretch_add = stretch_time / stretch_max;
          volume_mod = 5;
          retrig = 6;
          //   select_sample = linlin(audio, 0, 128, 0, NUM_SAMPLES);
        }
      }

      // set new phase
      phase_sample = pos[select_sample];
    }
  } else {
    r3 = RandomByte();
    LedOff();
  }
}
