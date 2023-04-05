// Sampler Player
// InA() -> selects sample
// InB() -> gates
// InK() -> also selects sample

// make breakbeat

#include "/home/zns/Arduino/nyblcore/nyblcore.h"
#include "/home/zns/Arduino/nyblcore/random.h"
#include "/home/zns/Arduino/nyblcore/generated-breakbeat-table.h"
#include <EEPROM.h>

#define SHIFTY 6
#define PARM1 30
#define PARM2 220

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
bool base_direction = 1;  // 0 = reverse, 1 == forward
byte retrig = 4;
byte tempo = 12;
int audio_last = 0;
int audio_next = -1;
byte audio_now = 0;
byte audio_played = 0;
char audio_add = 0;
byte stretch_amt = 0;
word stretch_add = 0;
byte knobB_last = 0;
byte knobA_last = 0;
byte knobK_last = 0;
byte probability = 30;
bool do_retrigger = false;
bool do_stutter = false;
bool do_stretch = false;
byte do_retriggerp = false;
byte do_stutterp = false;
byte do_stretchp = false;
byte bcount = 0;
byte lastMoved = 0;
bool firstrun = true;

#define NUM_TEMPOS 16
byte *tempo_steps[] = {
  (byte[]){ 0xAA, 0xAA, 0xAA, 0xAA },
  (byte[]){ 0x99, 0x99, 0x99, 0x99 },
  (byte[]){ 0x88, 0x88, 0x88, 0x88 },
  (byte[]){ 0x77, 0x77, 0x77, 0x77 },
  (byte[]){ 0x66, 0x66, 0x66, 0x66 },
  (byte[]){ 0x65, 0x65, 0x65, 0x65 },
  (byte[]){ 0x55, 0x55, 0x55, 0x55 },
  (byte[]){ 0x54, 0x54, 0x54, 0x54 },
  (byte[]){ 0x44, 0x44, 0x44, 0x44 },
  (byte[]){ 0x43, 0x44, 0x43, 0x43 },
  (byte[]){ 0x43, 0x43, 0x43, 0x43 },
  (byte[]){ 0x34, 0x34, 0x33, 0x43 },
  (byte[]){ 0x33, 0x33, 0x33, 0x33 },  // base tempo
  (byte[]){ 0x32, 0x32, 0x32, 0x32 },
  (byte[]){ 0x32, 0x22, 0x32, 0x22 },
  (byte[]){ 0x22, 0x22, 0x22, 0x22 },
};

void Setup() {
  RandomSetup();
}

void Loop() {
  byte knobA = InA();
  byte knobK = InK();
  byte knobB = InB();
  if (firstrun) {
    firstrun = false;
    tempo = EEPROM.read(0);
    delay(5);
    base_direction = EEPROM.read(1);
    delay(5);
    volume_reduce = EEPROM.read(2);
    delay(5);
    distortion = EEPROM.read(3);
    delay(5);
    probability = EEPROM.read(4);
    delay(5);
    do_stretchp = EEPROM.read(5);
    delay(5);
    do_retriggerp = EEPROM.read(6);
    delay(5);
    do_stutterp = EEPROM.read(7);
    delay(5);
    knobA_last = knobA;
    knobK_last = knobK;
    knobB_last = knobB;
  }
  bcount++;
  byte bthresh = knobA;
  if (lastMoved == 1) {
    bthresh = knobK;
  } else if (lastMoved == 2) {
    bthresh = knobB;
  }
  if (bcount < bthresh) {
    LedOn();
  } else {
    LedOff();
  }
  if (bcount == 255) {
    bcount = 0;
  }

  if (phase_sample_last != phase_sample) {
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

  if (knobK > knobK_last + 5 || knobK < knobK_last - 5) {
    lastMoved = 1;
    knobK_last = knobK;
  }
  if (knobA > knobA_last + 5 || knobA < knobA_last - 5) {
    lastMoved = 0;
    knobA_last = knobA;
    // update the left parameter
    if (knobK < PARM1) {
      // volume
      if (knobA <= 200) {
        volume_reduce = (200 - knobA) * 10 / 200;  // 0-220 -> 10-0
        distortion = 0;
      } else {
        volume_reduce = 0;
        distortion = knobA - 200;  // 200-255 -> 0-30
      }
      EEPROM.write(2, volume_reduce);
      EEPROM.write(3, distortion);
    } else if (knobK < PARM2) {
      probability = knobA / 2;  // 0-255 -> 0-100
      EEPROM.write(4, probability);
    } else {
      do_stretchp = knobA / 2;
      EEPROM.write(5, do_stretchp);
    }
  }
  if (knobB > knobB_last + 5 || knobB < knobB_last - 5) {
    lastMoved = 2;
    knobB_last = knobB;
    // update the right parameter
    if (knobK < PARM1) {
      if (knobB < 128) {
        tempo = knobB * NUM_TEMPOS / 128;
        base_direction = 0;  // reverse
      } else {
        tempo = (knobB - 128) * NUM_TEMPOS / 128;
        base_direction = 1;  // forward
      }
      EEPROM.write(0, tempo);
      EEPROM.write(1, base_direction);
    } else if (knobK < PARM2) {
      do_retriggerp = knobB / 4;
      EEPROM.write(6, do_retriggerp);
    } else {
      do_stutterp = knobB / 4;
      EEPROM.write(7, do_stutterp);
    }
  }


  // linear interpolation with shifts
  audio_now = (audio_last + audio_add) >> SHIFTY;

  // mute audio if volume_reduce==10
  if (volume_reduce == 10) audio_now = 128;

  // if not muted, make some actions
  if (audio_now != 128) {
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
  // click preventer??
  if (abs(audio_now-audio_played)>32) {
    audio_now = (audio_now+audio_played)/2;
    if (abs(audio_now-audio_played)>32) {
      audio_now = (audio_now+audio_played)/2;
    }
  }
  OutF(audio_now);
  audio_played=audio_now;
  // for linear interpolation
  audio_add = audio_add + audio_next;

  thresh_counter++;
  if (thresh_counter == thresh_next) {
    thresh_counter = 0;
    thresh_nibble++;
    if (thresh_nibble >= 6) {
      thresh_nibble = 0;
    }
    thresh_next = tempo_steps[tempo - stretch_amt][thresh_nibble / 2];
    if (thresh_nibble % 2 == 0) {
      thresh_next = (byte)(thresh_next & 0xF0) >> 4;
    } else {
      thresh_next = (byte)(thresh_next & 0x0F);
    }

    // determine directions
    phase_sample += (direction * 2 - 1);
    if (phase_sample > pos[NUM_SAMPLES]) {
      phase_sample = 0;
    } else if (phase_sample < 0) {
      phase_sample = pos[NUM_SAMPLES];
    }

    if (phase_sample % retrigs[retrig] == 0) {
      // randoms
      byte r1 = RandomByte();
      byte r2 = RandomByte();
      byte r3 = RandomByte();
      byte r4 = RandomByte();

      // do stretching
      if (do_stretchp > 10) {
        do_stretch = (r1 < do_stretchp);
      } else {
        do_stretch = false;
      }
      if (do_stretch) {
        stretch_amt++;
        if (tempo - stretch_amt < 0) stretch_amt = tempo;
      } else {
        stretch_amt = 0;
      }


      if (volume_mod > 0) {
        if (volume_mod > 3 || r3 < 200) {
          volume_mod--;
        }
      } else {
        // randomize direction
        if (direction==base_direction) {
          if (r1 < probability/8) {
            direction = 1-base_direction;
          }
        } else {
          if (r1 < probability) {
            direction = base_direction;
          }          
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


        if (do_retrigger == false && do_stutter == false) {
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
          if (r3 < probability/2) {
            thresh_next = thresh_next + ((r1 - r3) * 4 / 255);
            retrig = ((r1 - r2) * 6 / 255);
            select_sample = (r3 * NUM_SAMPLES) / 60;
          }
        }

        if (do_retriggerp > 10) {
          do_retrigger = (r4 < do_retriggerp);
        } else {
          do_retrigger = false;
        }
        if (do_retrigger) {
          retrig = 6;
          if (r2 < 120) {
            retrig = 5;
          }
        }

        if (do_stutterp > 10) {
          do_stutter = (r3 < do_stutterp);
        } else {
          do_stutter = false;
        }
        if (do_stutter) {
          if (volume_mod == 0) {
            volume_mod = r2 * 6 / 255;
            if (r3 < 120) {
              retrig = 5;
            } else {
              retrig = 6;
            }
          }
        }
      }

      // set new phase
      // if (select_sample % 2 == 0) {
      //   LedOn();
      // } else {
      //   LedOff();
      // }
      phase_sample = pos[select_sample];
    }
  }
}
