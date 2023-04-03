// Sampler Player
// InA() -> selects sample
// InB() -> gates
// InK() -> also selects sample

// make breakbeat

#include "/home/zns/Arduino/nyblcore/nyblcore.h"
#include "/home/zns/Arduino/nyblcore/random.h"
#include "/home/zns/Arduino/nyblcore/generated-breakbeat-table.h"
#include <EEPROM.h>

#define GRAINS 1
#define SHIFTY 6
#define PARM1 30
#define PARM2 220

byte distortion = 0;
byte volume_reduce = 0;  // volume 0 to 6
byte volume_mod = 0;

byte knobB_last = 0;
byte knobA_last = 0;
byte knobK_last = 0;
byte bcount = 0;
byte lastMoved = 0;
bool firstrun = true;
word phase_sample[GRAINS] = 0;
word phase_sample_last[GRAINS]  = 11;
byte thresh_counter[GRAINS] = 0;
byte thresh_next[GRAINS] = 3;
byte thresh_nibble[GRAINS];
int audio_last[GRAINS];
int audio_next[GRAINS];
word audio_diff[GRAINS];
char audio_add[GRAINS];
word sample_start[GRAINS];
byte env_cur[GRAINS];
byte env_step[GRAINS];
bool env_rev[GRAINS];
byte audio_now[GRAINS];
byte audio_played;
byte tempo[GRAINS];
byte audio_final;



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
    volume_reduce = EEPROM.read(0);
    delay(5);
    distortion = EEPROM.read(1);
    delay(5);
    tempo = EEPROM.read(2);
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

  audio_final = 0;
  for (byte g=0;g<GRAINS;g++) {
    // next sample
    thresh_counter[g]++;

    // update the sample
    if (thresh_counter[g] == thresh_next[g]) {
      thresh_counter[g] = 0;
      thresh_nibble[g]++;
      if (thresh_nibble[g] >= 6) {
        thresh_nibble[g] = 0;
      }
      thresh_next[g] = tempo_steps[tempo][thresh_nibble[g] / 2];
      if (thresh_nibble[g] % 2 == 0) {
        thresh_next[g] = (byte)(thresh_next[g] & 0xF0) >> 4;
      } else {
        thresh_next[g] = (byte)(thresh_next[g] & 0x0F);
      }
      phase_sample[g] += 1;
      if (phase_sample[g] > NUM_SAMPLES) {
        phase_sample[g] = 0;
      } else if (phase_sample[g] < 0) {
        phase_sample[g] = NUM_SAMPLES;
      }
      audio_last[g] = audio_next[g];
      audio_next[g] = ((int)pgm_read_byte(SAMPLE_TABLE + phase_sample[g])) << SHIFTY;
      audio_diff[g] = (audio_next[g] - audio_last[g]) / ((int)(thresh_next[g] - thresh_counter[g]));
      audio_add[g] = 0;

      if (env_rev[g]) {
        env_cur[g]-=env_step[g];
      } else {
        env_cur[g]+=env_step[g];
      }
      if (env_cur[g]==0) {
        env_rev[g]=false; // go forward
        // TODO: choose new location to start grain?
      } else if (env_cur[g]==250) {
        env_rev[g]=true;
      }
    }

    // add to the current audio
    audio_now[g] = audio_last[g] + audio_add[g];

    // update the final audio
    audio_final += (audio_now[g] * env_rev[g])/255;

    // linear interpolation
    audio_add[g] += audio_diff[g];

  }


  // check knobs
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
      EEPROM.write(0, volume_reduce);
      EEPROM.write(1, distortion);
    } else if (knobK < PARM2) {
    } else {
    }
  }
  if (knobB > knobB_last + 5 || knobB < knobB_last - 5) {
    lastMoved = 2;
    knobB_last = knobB;
    // update the right parameter
    if (knobK < PARM1) {
      tempo = knobB * NUM_TEMPOS / 255;
      EEPROM.write(2, tempo);
    } else if (knobK < PARM2) {
    } else {
    }
  }


  // mute audio if volume_reduce==10
  if (volume_reduce == 10) audio_final = 128;

  // if not muted, make some actions
  if (audio_final != 128) {
    // distortion / wave-folding
    if (distortion > 0) {
      if (audio_final > 128) {
        if (audio_final < (255 - distortion)) {
          audio_final += distortion;
        } else {
          audio_final = 255 - distortion;
        }
      } else {
        if (audio_final > distortion) {
          audio_final -= distortion;
        } else {
          audio_final = distortion - audio_final;
        }
      }
    }
    // reduce volume
    if ((volume_reduce + volume_mod) > 0) {
      if (audio_final > 128) {
        audio_final = ((audio_final - 128) >> (volume_reduce + volume_mod)) + 128;
      } else {
        audio_final = 128 - ((128 - audio_final) >> (volume_reduce + volume_mod));
      }
    }
  }
  // click preventer??
  if (abs(audio_final-audio_played)>32) {
    audio_final = (audio_final+audio_played)/2;
    if (abs(audio_final-audio_played)>32) {
      audio_final = (audio_final+audio_played)/2;
    }
  }
  OutF(audio_final);
  audio_played=audio_final;
}
