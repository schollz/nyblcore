// SAMPLETABLE

#include <avr/eeprom.h>

#ifndef JERBOA_H_
#define JERBOA_H_

#ifndef MOC_EXTRA_LOOP_COUNT
#define MOC_EXTRA_LOOP_COUNT 0
#endif

#ifndef MOC_TICKS
#define MOC_TICKS 0
#endif

#ifndef WHICH_PWM
#define WHICH_PWM 1 /* 1 or 4 */
#endif

#ifndef WHICH_LED
#define WHICH_LED 0
#endif

// ATtiny{25,45,85}
//
//         +--u--+
//       x |R   V| x
//   (3) 3 |A   K| 2 (1)
//   (2) 4 |B   F| 1
//       x |G   L| 0
//         +-----+
//  (ADC)          PB

extern void Setup(void);
extern void Loop(void);

namespace jerboa_internal {

volatile word spin_tmp;
void SpinDelay(word n) {
  for (word i = 0; i < n; i++) {
    for (word j = 0; j < 100; j++) {
      spin_tmp += j;
    }
  }
}
void SpinDelayFast(word n) {
  for (word i = 0; i < n; i++) {
    spin_tmp += i;
  }
}

const PROGMEM char MoctalTick_GapFollows[] = {
    0 /*unused*/, 0, 0, 0, 0, 0, 0, 0, 1};

bool led;
void LedOn() {
  led = true;
  digitalWrite(WHICH_LED, HIGH);
}  // Set low bit; other bits are pullups.
void LedOff() {
  led = false;
  digitalWrite(WHICH_LED, LOW);
}  // Clear low bit; other bits are pullups.
void LedToggle() {
  if (led)
    LedOff();
  else
    LedOn();
}
void LedSet(bool value) {
  if (value)
    LedOn();
  else
    LedOff();
}

// Fault(n) stops everything else and makes flashy pulses in groups of n.
void Fault(byte n) {
  cli();  // No more interrupts.
  USICR = 0;
  pinMode(WHICH_LED, OUTPUT);
  while (true) {
    for (byte k = 0; k < n; k++) {
      for (word i = 0; i < 8; i++) {
        SpinDelay(400);
        LedOn();
        SpinDelay(100);
        LedOff();
      }
      SpinDelay(5000);
    }
    SpinDelay(8000);
  }
}

struct MoctalTicker {
  // You must zero these yourself, if not global.
  volatile byte data;     // Number to ouptut.
  volatile byte shifted;  // High bit is output; shifts to the left.
  volatile byte tick;     // Counts within states.
  volatile byte state;    // Counts bits and gaps.
  // volatile byte data_last;

  static void Setup() { pinMode(WHICH_LED, OUTPUT); }

  void Tick() {
    if (tick == 0) {
      // MOVE
      if (state == 0 || state == 8) {
        shifted = data;
        state = 1;
      } else {
        shifted <<= 1;
        ++state;
      }

      if (shifted & 0x80) {
        tick = 5;  // Long pulse.
      } else {
        tick = 2;  // Short pulse.
      }

      if (pgm_read_byte(MoctalTick_GapFollows + state)) {
        tick += 2;  // Pulse followed by longer gap.
      }
    }

    if (tick > (pgm_read_byte(MoctalTick_GapFollows + state) ? 3 : 1)) {
      LedOn();
    } else {
      LedOff();
    }
    tick--;
  }
};
MoctalTicker moc;

// Timer/Counter 1 PWM Output OC1A (PB1)
struct FastPwm1Base {
  static void SetupPLL() {
    // de https://github.com/viking/attiny85-player //
    PLLCSR |= _BV(PLLE);  // Enable 64 MHz PLL (p94)
    SpinDelay(1);         /// delayMicroseconds(100);            // Stabilize
    while (!(PLLCSR & _BV(PLOCK)))
      ;                   // Wait for it... (p94)
    PLLCSR |= _BV(PCKE);  // Timer1 source = PLL
  };
};
struct FastPwm1A : public FastPwm1Base {
  static void Setup() {
    SetupPLL();

    // Set up Timer/Counter1 for PWM output
    TIMSK = 0;             // Timer interrupts OFF (p92)
    TCCR1 = _BV(PWM1A)     // Pulse Width Modulator A Enable. (p89)
            | _BV(COM1A1)  // Clear OC1A on match; set on count $00.
            | _BV(CS10);   // 1:1 prescale (p89)
    GTCCR = 0;             // Do not enable PW Modulater B. (p90)
    OCR1C = 255;           // Full 8-bit PWM cycle (p92)
    OCR1B = 0;             // Not used.
    OCR1A = 128;           // 50% duty at start

    pinMode(1, OUTPUT);  // Enable PWM output OC1A on pin PB1.
  }
  static void Output(int x) {
    OCR1A = x;  // (p92)
  }
};

struct FastPwm1B : public FastPwm1Base {
  static void Setup() {
    SetupPLL();

    // Set up Timer/Counter1 for PWM output
    TIMSK = 0;              // Timer interrupts OFF (p92)
    GTCCR = _BV(PWM1B)      // Pulse Width Modulator B Enable. (p89)
            | _BV(COM1B1);  // Clear OC1B on match; set on count $00 (p86).
    TCCR1 = _BV(CS10);      // Do not enable PW Modulater A. 1:1 prescale. (p89)
    OCR1C = 255;            // Full 8-bit PWM cycle (p92)
    OCR1A = 128;            // Not used.
    OCR1B = 128;            // 50% duty at start

    pinMode(4, OUTPUT);  // Enable PWM output OC1B on pin PB4.
  }
  static void Output(int x) {
    OCR1B = x;  // (p92)
  }
};

struct AnalogIn {
  static void NextInputA() {
    ADMUX = 0 |           // Use Vcc for Reference; disconnect from PB0 (p134)
            _BV(ADLAR) |  // Left-Adjust the ADC Result in ADCH (p134)
            3;            // ADC3 is PB3 is A.
  }
  static void NextInputB() {
    ADMUX = 0 |           // Use Vcc for Reference; disconnect from PB0 (p134)
            _BV(ADLAR) |  // Left-Adjust the ADC Result in ADCH (p134)
            2;            // ADC2 is PB4 is B.
  }
  static void NextInputK() {
    ADMUX = 0 |           // Use Vcc for Reference; disconnect from PB0 (p134)
            _BV(ADLAR) |  // Left-Adjust the ADC Result in ADCH (p134)
            1;            // ADC1 is PB2 is K.
  }
  static void Setup() {
    ADCSRA = _BV(ADEN);  // Enable ADC first.

    NextInputA();

    ADCSRB = 0;  // free-running ADC; not Bipolar; not reversed. (p137)

    // (p125) max ADC res: clock 50kHz to 200kHz
    // If less than 10 bits, 200khz to 1MHz.
    // We use internal 16MHz clock, so try dividing by 32.
    // 1->/2 2->/4 3->/8 4->/16 5->/32 6->/64 7->/128  (p136)
    // 7->4.88kHz   6->9.75kHz  5->19.53kHz Nyquist (freq of LED 2xToggle)
    // Use 6:   16MHz / 64 => 250kHz.
    // 250kHz / 13 => 19,230 samples per sec; nyquist 9615 Hz
    // Use 5: Can do two samples, 9615 Hz.
#define DIVISOR 5
    byte tmp = _BV(ADEN) |   // Enable the Analog-to-Digital converter (p136)
               _BV(ADATE) |  // Enable ADC auto trigger
               _BV(ADIE) |   // Enable interrupt on Conversion Complete
               DIVISOR;
    // ADCSRA = tmp;
    ADCSRA = tmp | _BV(ADSC);  // Start the fist conversion
  }
  static byte Input() {
    return ADCH;  // Just 8-bit.  Assumes ADLAR is 1.
  }
};

#if WHICH_PWM == 1
FastPwm1A pwm;
#else
FastPwm1B pwm;
#endif

volatile byte AnalogA, AnalogB, AnalogK;
AnalogIn in;
volatile byte adc_counter;
volatile bool adc_switch;

ISR(ADC_vect) {
  if (adc_switch) {
    ++adc_counter;
    adc_switch = false;
    // One time out of 256 we sample K.  The rest we sample B.
    if (adc_counter == 2) {
      AnalogB = ADCH;  // on 2, we still save B, but request K next.
      AnalogIn::NextInputK();
    } else if (adc_counter == 3) {
      AnalogK = ADCH;  // on 3, we save K, but request B again.
      AnalogIn::NextInputB();
    } else {
      AnalogB = ADCH;
      AnalogIn::NextInputB();
    }
  } else {
    adc_switch = true;
    AnalogA = ADCH;
    AnalogIn::NextInputA();
  }
}

void setup() {
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(WHICH_LED, OUTPUT);
  pinMode(WHICH_PWM, OUTPUT);
  LedOff();

  pwm.Setup();
  in.Setup();
  // moc.Setup();
  // moc.data = 0;

  ::Setup();  // Call user's Setup.
}

void loop() {
  // no need to return; just loop here.
  while (true) {
    // Wait for the next Analog sample in.
    {
      byte old_counter = adc_counter;
      byte c;
#if MOC_EXTRA_LOOP_COUNT
      word loops = 0;
#endif
      do {
#if MOC_EXTRA_LOOP_COUNT
        ++loops;
#endif
        c = adc_counter;
      } while (old_counter == c);
      if (byte(old_counter + 1) == c) {
#if MOC_EXTRA_LOOP_COUNT
        moc.data = loops;
#endif
      } else {
        Fault(3);  // Fault on overruns.  May relax this.
      }
      old_counter = c;
    }

    ::Loop();  // Call user's Loop.

#if MOC_TICKS
    if (adc_counter == 0) {
      static byte tick_counter;
      ++tick_counter;
      if ((tick_counter & 15) == 0) {
        moc.Tick();
      }
    }
#endif
  }
}

}  // namespace jerboa_internal

void setup() { jerboa_internal::setup(); }
void loop() { jerboa_internal::loop(); }

// public wrappers
inline byte InA() { return jerboa_internal::AnalogA; }
inline byte InB() { return jerboa_internal::AnalogB; }
inline byte InR() { return jerboa_internal::AnalogK; }  // R was old name for K.
inline byte InK() { return jerboa_internal::AnalogK; }
inline void OutF(byte b) { jerboa_internal::pwm.Output(b); }
inline void Moctal(byte b) { jerboa_internal::moc.data = b; }

#define IN_A() (jerboa_internal::AnalogA)
#define IN_B() (jerboa_internal::AnalogB)
#define IN_R() (jerboa_internal::AnalogK)  // R was old name for K.
#define IN_K() (jerboa_internal::AnalogK)
#define OUT_F(B) (jerboa_internal::pwm.Output(B))
#define MOCTAL(B) (jerboa_internal::moc.data = (B))

using jerboa_internal::Fault;
using jerboa_internal::LedOff;
using jerboa_internal::LedOn;
using jerboa_internal::LedSet;
using jerboa_internal::LedToggle;
using jerboa_internal::SpinDelay;
using jerboa_internal::SpinDelayFast;

#endif

namespace jerboa_random {

typedef struct rc4_key {
  unsigned char state[256];
  unsigned char x;
  unsigned char y;
} rc4_key;

void swap_byte(unsigned char *a, unsigned char *b) {
  unsigned char swapByte;

  swapByte = *a;
  *a = *b;
  *b = swapByte;
}

void prepare_key(unsigned char *key_data_ptr, int key_data_len, rc4_key *key) {
  unsigned char swapByte;
  unsigned char index1;
  unsigned char index2;
  unsigned char *state;
  short counter;

  state = &key->state[0];
  for (counter = 0; counter < 256; counter++) state[counter] = counter;
  key->x = 0;
  key->y = 0;
  index1 = 0;
  index2 = 0;
  for (counter = 0; counter < 256; counter++) {
    index2 = (key_data_ptr[index1] + state[counter] + index2) % 256;
    swap_byte(&state[counter], &state[index2]);

    index1 = (index1 + 1) % key_data_len;
  }
}

void rc4(unsigned char *buffer_ptr, int buffer_len, rc4_key *key) {
  unsigned char x;
  unsigned char y;
  unsigned char *state;
  unsigned char xorIndex;
  short counter;

  x = key->x;
  y = key->y;

  state = &key->state[0];
  for (counter = 0; counter < buffer_len; counter++) {
    x = (x + 1) % 256;
    y = (state[x] + y) % 256;
    swap_byte(&state[x], &state[y]);

    xorIndex = state[x] + (state[y]) % 256;

    buffer_ptr[counter] ^= state[xorIndex];
  }
  key->x = x;
  key->y = y;
}

rc4_key Engine;

}  // namespace jerboa_random

void RandomSetup() {
  jerboa_random::prepare_key("RandomSetup", 12, &jerboa_random::Engine);
}

byte RandomByte() {
  unsigned char buf[1] = {0};
  jerboa_random::rc4(buf, 1, &jerboa_random::Engine);
  return buf[0];
}

#define SHIFTY 6
#define PARM1 30
#define PARM2 220

byte distortion = 0;
byte volume_reduce = 0;  // volume 0 to 6
byte volume_mod = 0;
byte thresh_counter = 0;
byte thresh_next = 3;
bool thresh_nibble = 0;
word phase_sample = 0;
word phase_sample_last = 11;
byte select_sample = 0;
byte select_sample_start = 0;
byte select_sample_end = NUM_SAMPLES - 1;
bool direction = 1;  // 0 = reverse, 1 = forward
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
word debounce_eeprom = 0;

#define NUM_TEMPOS 16
const byte tempo_steps[] = {
    0xAA, 0x99, 0x98, 0x88, 0x87, 0x77, 0x76, 0x66,
    0x65, 0x55, 0x54, 0x44, 0x43, 0x33, 0x32, 0x22,
};

void Setup() { RandomSetup(); }

void Loop() {
  byte knobA = InA();
  byte knobK = InK();
  byte knobB = InB();
  if (firstrun) {
    firstrun = false;
    tempo = eeprom_read_byte((uint8_t *)0);
    delay(5);
    volume_reduce = eeprom_read_byte((uint8_t *)2);
    delay(5);
    distortion = eeprom_read_byte((uint8_t *)3);
    delay(5);
    probability = eeprom_read_byte((uint8_t *)4);
    delay(5);
    do_stretchp = eeprom_read_byte((uint8_t *)5);
    delay(5);
    do_retriggerp = eeprom_read_byte((uint8_t *)6);
    delay(5);
    do_stutterp = eeprom_read_byte((uint8_t *)7);
    delay(5);
    knobA_last = knobA;
    knobK_last = knobK;
    knobB_last = knobB;
  } else if (debounce_eeprom > 0) {
    debounce_eeprom--;
    if (debounce_eeprom == 0) {
      eeprom_write_byte((uint8_t *)1, 1);
      eeprom_write_byte((uint8_t *)0, tempo);
      eeprom_write_byte((uint8_t *)2, volume_reduce);
      eeprom_write_byte((uint8_t *)3, distortion);
      eeprom_write_byte((uint8_t *)4, probability);
      eeprom_write_byte((uint8_t *)5, do_stretchp);
      eeprom_write_byte((uint8_t *)6, do_retriggerp);
      eeprom_write_byte((uint8_t *)7, do_stutterp);
    }
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
      audio_next = ((int)pgm_read_byte(SAMPLE_TABLE + phase_sample +
                                       (direction * 2 - 1)))
                   << SHIFTY;
      audio_next =
          (audio_next - audio_last) / ((int)(thresh_next - thresh_counter));
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
    } else if (knobK < PARM2) {
      probability = knobA / 2;  // 0-255 -> 0-100
    } else {
      do_stretchp = knobA / 2;
    }
    debounce_eeprom = 65534;
  }
  if (knobB > knobB_last + 5 || knobB < knobB_last - 5) {
    lastMoved = 2;
    knobB_last = knobB;
    // update the right parameter
    if (knobK < PARM1) {
      tempo = knobB * NUM_TEMPOS / 255;
    } else if (knobK < PARM2) {
      do_retriggerp = knobB / 4;
    } else {
      do_stutterp = knobB / 4;
    }
    debounce_eeprom = 65534;
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
  if (abs(audio_now - audio_played) > 32) {
    audio_now = (audio_now + audio_played) / 2;
    if (abs(audio_now - audio_played) > 32) {
      audio_now = (audio_now + audio_played) / 2;
    }
  }
  OutF(audio_now);
  audio_played = audio_now;
  // for linear interpolation
  audio_add = audio_add + audio_next;

  thresh_counter++;
  if (thresh_counter == thresh_next) {
    thresh_counter = 0;
    thresh_nibble = 1 - thresh_nibble;
    thresh_next = tempo_steps[tempo - stretch_amt];
    if (thresh_nibble) {
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
        if (direction == 1) {
          if (r1 < probability / 8) {
            direction = 0;
          }
        } else {
          if (r1 < probability) {
            direction = 1;
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
          if (select_sample < select_sample_start)
            select_sample = select_sample_end;
          if (select_sample > select_sample_end)
            select_sample = select_sample_start;

          // random jump
          if (r3 < probability / 2) {
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

      phase_sample = pos[select_sample];
    }
  }
}
