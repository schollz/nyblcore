
#ifndef JERBOA_H_
#define JERBOA_H_

#ifndef MOC_EXTRA_LOOP_COUNT
#define MOC_EXTRA_LOOP_COUNT 0
#endif

#ifndef MOC_TICKS
#define MOC_TICKS 0
#endif

#ifndef WHICH_PWM
#define WHICH_PWM 1  /* 1 or 4 */
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

const PROGMEM char MoctalTick_GapFollows[] = {0/*unused*/,  0,0, 0,0,0, 0,0,1};

bool led;
void LedOn() { led = true; digitalWrite(WHICH_LED, HIGH); }    // Set low bit; other bits are pullups.
void LedOff() { led = false; digitalWrite(WHICH_LED, LOW); }  // Clear low bit; other bits are pullups.
void LedToggle() { if (led) LedOff(); else LedOn(); }
void LedSet(bool value) { if (value) LedOn(); else LedOff(); }

// Fault(n) stops everything else and makes flashy pulses in groups of n.
void Fault(byte n) {
  cli(); // No more interrupts.
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

// word linlin(word f, word slo, word shi, word dlo, word dhi) {
//     if (f<=slo) {
//         return dlo;
//     } else if (f >= shi) {
//         return dhi;
//     } else {
//         return (f-slo)*(dhi-dlo)/(shi-slo)+dlo;
//     }
// }


// byte linlin2(byte f, byte slo, byte shi, byte dlo, byte dhi) {
//     if (f<=slo) {
//         return dlo;
//     } else if (f >= shi) {
//         return dhi;
//     } else {
//         return (byte)(((word)(f-slo))*((word)(dhi-dlo))/(shi-slo)+dlo);
//     }
// }


struct MoctalTicker {
  
  // You must zero these yourself, if not global.
  volatile byte data;     // Number to ouptut.
  volatile byte shifted;  // High bit is output; shifts to the left.
  volatile byte tick;     // Counts within states.
  volatile byte state;    // Counts bits and gaps.
  // volatile byte data_last;

  static void Setup() {
    pinMode(WHICH_LED, OUTPUT);
  }  

  void Tick() {
   //  if (tick == 0 || (pgm_read_byte(MoctalTick_GapFollows+state) == 0 && tick== 5)) {
   //    if (state==0 || state==8) {
   //      shifted = data;
   //      state = 1;
   //    } else {
   //      shifted <<= 1;
   //      ++state;
   //    }
      
   //    if (shifted & 0x80) {
   //      tick = 12;  // Long pulse.
   //    } else {
   //      tick = 9;  // Short pulse.
   //    }
   //  } 

   // if (tick > 7) {
   //    LedOn();
   //  } else {
   //    LedOff();
   //  }
   //  tick--;
    if (tick == 0) {
      // MOVE
      if (state==0 || state==8) {
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
      
      if (pgm_read_byte(MoctalTick_GapFollows+state)) {
        tick += 2;  // Pulse followed by longer gap.
      }
    }
    
    if (tick > (pgm_read_byte(MoctalTick_GapFollows+state) ? 3 : 1)) {
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
    PLLCSR |= _BV(PLLE);               // Enable 64 MHz PLL (p94)
    SpinDelay(1); /// delayMicroseconds(100);            // Stabilize
    while(!(PLLCSR & _BV(PLOCK)));     // Wait for it... (p94)
    PLLCSR |= _BV(PCKE);               // Timer1 source = PLL
  };
};
struct FastPwm1A : public FastPwm1Base {
  static void Setup() {
    SetupPLL();
  
    // Set up Timer/Counter1 for PWM output
    TIMSK  = 0;                        // Timer interrupts OFF (p92)
    TCCR1  = _BV(PWM1A)                // Pulse Width Modulator A Enable. (p89)
           | _BV(COM1A1)               // Clear OC1A on match; set on count $00.
           | _BV(CS10);                // 1:1 prescale (p89)
    GTCCR  = 0;                        // Do not enable PW Modulater B. (p90)
    OCR1C  = 255;                      // Full 8-bit PWM cycle (p92)
    OCR1B  = 0;                        // Not used.
    OCR1A  = 128;                      // 50% duty at start

    pinMode(1, OUTPUT);                // Enable PWM output OC1A on pin PB1.
  }
  static void Output(int x) {
    OCR1A = x;  // (p92)
  }
};

struct FastPwm1B : public FastPwm1Base {
  static void Setup() {
    SetupPLL();
  
    // Set up Timer/Counter1 for PWM output
    TIMSK  = 0;                        // Timer interrupts OFF (p92)
    GTCCR  = _BV(PWM1B)                // Pulse Width Modulator B Enable. (p89)
           | _BV(COM1B1);              // Clear OC1B on match; set on count $00 (p86).
    TCCR1  = _BV(CS10);                // Do not enable PW Modulater A. 1:1 prescale. (p89)
    OCR1C  = 255;                      // Full 8-bit PWM cycle (p92)
    OCR1A  = 128;                      // Not used.
    OCR1B  = 128;                      // 50% duty at start

    pinMode(4, OUTPUT);                // Enable PWM output OC1B on pin PB4.
  }
  static void Output(int x) {
    OCR1B = x;  // (p92)
  }
};

struct AnalogIn {
  static void NextInputA() {
    ADMUX = 0 |    // Use Vcc for Reference; disconnect from PB0 (p134)
            _BV(ADLAR) | // Left-Adjust the ADC Result in ADCH (p134)
            3;     // ADC3 is PB3 is A.
  }
  static void NextInputB() {
    ADMUX = 0 |    // Use Vcc for Reference; disconnect from PB0 (p134)
            _BV(ADLAR) | // Left-Adjust the ADC Result in ADCH (p134)
            2;     // ADC2 is PB4 is B.
  }
  static void NextInputK() {
    ADMUX = 0 |    // Use Vcc for Reference; disconnect from PB0 (p134)
            _BV(ADLAR) | // Left-Adjust the ADC Result in ADCH (p134)
            1;     // ADC1 is PB2 is K.
  }
  static void Setup() {
    ADCSRA = _BV(ADEN);  // Enable ADC first.
    
    NextInputA();
            
    ADCSRB = 0; // free-running ADC; not Bipolar; not reversed. (p137)

    // (p125) max ADC res: clock 50kHz to 200kHz
    // If less than 10 bits, 200khz to 1MHz.
    // We use internal 16MHz clock, so try dividing by 32.
    // 1->/2 2->/4 3->/8 4->/16 5->/32 6->/64 7->/128  (p136)
    // 7->4.88kHz   6->9.75kHz  5->19.53kHz Nyquist (freq of LED 2xToggle)  
    // Use 6:   16MHz / 64 => 250kHz.    
    // 250kHz / 13 => 19,230 samples per sec; nyquist 9615 Hz
    // Use 5: Can do two samples, 9615 Hz.
#define DIVISOR 5
    byte tmp = _BV(ADEN) |  // Enable the Analog-to-Digital converter (p136)
              _BV(ADATE) |  // Enable ADC auto trigger
              _BV(ADIE) |  // Enable interrupt on Conversion Complete
              DIVISOR;
    // ADCSRA = tmp;
    ADCSRA = tmp | _BV(ADSC);  // Start the fist conversion
  }
  static byte Input() {
    return ADCH;   // Just 8-bit.  Assumes ADLAR is 1. 
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
      AnalogB = ADCH;          // on 2, we still save B, but request K next.
      AnalogIn::NextInputK();
    } else if (adc_counter == 3) {
      AnalogK = ADCH;          // on 3, we save K, but request B again.
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
      if (byte(old_counter+1) == c) {
#if MOC_EXTRA_LOOP_COUNT 
        moc.data = loops;
#endif
      } else {
        Fault(3); // Fault on overruns.  May relax this.
      }
      old_counter=c;
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

}  // namespace

void setup() { jerboa_internal::setup(); }
void loop() { jerboa_internal::loop(); }

// public wrappers
// inline byte linlin2(byte f, byte slo, byte shi, byte dlo, byte dhi) { return jerboa_internal::linlin(f,slo,shi,dlo,dhi);}
// inline word linlin(word f, word slo, word shi, word dlo, word dhi) { return jerboa_internal::linlin(f,slo,shi,dlo,dhi); }
inline byte InA()   { return jerboa_internal::AnalogA; }
inline byte InB()   { return jerboa_internal::AnalogB; }
inline byte InR()   { return jerboa_internal::AnalogK; }  // R was old name for K.
inline byte InK()   { return jerboa_internal::AnalogK; }
inline void OutF(byte b) { jerboa_internal::pwm.Output(b); }
inline void Moctal(byte b)  { jerboa_internal::moc.data = b; }

#define IN_A()   (jerboa_internal::AnalogA)
#define IN_B()   (jerboa_internal::AnalogB)
#define IN_R()   (jerboa_internal::AnalogK)  // R was old name for K.
#define IN_K()   (jerboa_internal::AnalogK)
#define OUT_F(B) (jerboa_internal::pwm.Output(B))
#define MOCTAL(B)  (jerboa_internal::moc.data = (B))

using jerboa_internal::LedOn;
using jerboa_internal::LedOff;
using jerboa_internal::LedToggle;
using jerboa_internal::LedSet;
using jerboa_internal::Fault;
using jerboa_internal::SpinDelay;
using jerboa_internal::SpinDelayFast;

#endif
