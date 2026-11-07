/* *
 * linear range at VS = 3.3V: +/-29A
 *
 * wiring
 *   VOUT = pin 7
 *   VS = pin 8 (3.3V)
 *   GND = pin 5 
 */

#include <Arduino.h>

// feather configs
const int   CURR_SENSE_PIN   = A5;     // ADC_CS

const int   ADC_SDI_PIN      = MOSI;   // Feather MOSI -> ADC DATA IN  (pin 17)
const int   ADC_SDO_PIN      = MISO;   // Feather MISO <- ADC DATA OUT (pin 16)
const int   ADC_SCLK_PIN     = SCK;    // Feather SCK  -> ADC I/O CLOCK (pin 18)
const int   ADC_EOC_PIN      = D0;      // D0 <- ADC EOC (pin 19) 
const int   NUM_CH           = 11;     // 11 sensors 

#define ADC_RESOLUTION_BITS      12    // ADC 12-bit
const float ADC_MAX_COUNTS      = (float)((1UL << ADC_RESOLUTION_BITS) - 1);
const unsigned long ADC_EOC_TIMEOUT_US = 200; // give up waiting for EOC 
const float VREF                = 3.3f;   // ADC reference voltage 
const float VS                  = 3.3f;   // sensor supply voltage

// sensor configs
const float SENSITIVITY = 0.050f; // 50 mV/A
const float IDEAL_VOUT_0A       = 0.5f * VS; // bidirectional
const int   NUM_SAMPLES         = 32; // oversampling 

// overcurrent protection thresholds
// CHANGE THRESHOLDS 
const float OVERCURR_THRESHOLD = 29.0f; // (A)
const float OVERCURR_CLEAR     = 26.0f; // (A) must drop below this to clear
const unsigned long OVERCURR_DUR = 5; // (ms) duration 

// one entry per sensor/channel
float VOUT_0A[NUM_CH]; // filled with IDEAL_VOUT_0A in setup(), updated by offset()
bool  overcurr[NUM_CH];
unsigned long overcurr_start_time[NUM_CH];
bool adc_eoc_fault = false;   // set if EOC never went high in time

// funcs 

// one 12-clock transfer with the ADC (8-bits -> 12-bits) 
uint16_t adc_transfer(uint8_t cmd) {
  uint16_t result = 0;

  digitalWrite(CURR_SENSE_PIN, LOW);   // transfer starts when CS low 
  delayMicroseconds(2);

  for (int i = 0; i < 12; i++) {
    // first 8 clocks carry the command, last 4 are don't care
    digitalWrite(ADC_SDI_PIN, (i < 8) ? ((cmd >> (7 - i)) & 0x01) : LOW);
    delayMicroseconds(1);

    result = (result << 1) | (digitalRead(ADC_SDO_PIN) ? 1 : 0);  // DATA OUT: MSB first

    digitalWrite(ADC_SCLK_PIN, HIGH);
    delayMicroseconds(1);
    digitalWrite(ADC_SCLK_PIN, LOW);
  }

  digitalWrite(CURR_SENSE_PIN, HIGH);  // CS high 
  return result & 0x0FFF;
}

// wait for EOC to go high (conversion done), with timeout
void adc_wait_eoc() {
  unsigned long start = micros();
  while (!digitalRead(ADC_EOC_PIN)) {
    if (micros() - start > ADC_EOC_TIMEOUT_US) {
      adc_eoc_fault = true;   // flag it, then carry on instead of hanging
      return;
    }
  }
}

// reads the ADC -> averaged voltage at VOUT
float read_VOUT_avg(uint8_t ch) {
  // command byte: channel in top 4 bits, config = 0000
  // (12-bit output, MSB first, unipolar binary)
  uint8_t cmd = (uint8_t)(ch << 4);
  uint32_t sum = 0;

  // The ADC returns the PREVIOUS conversion, so do one throwaway transfer
  // to select this channel. (May contain data from a different channel.)
  adc_wait_eoc();
  adc_transfer(cmd);

  for (int i = 0; i < NUM_SAMPLES; i++) {
    adc_wait_eoc();
    sum += adc_transfer(cmd); 
    delayMicroseconds(50); // small delay
  }
  float avg_cnt = (float)sum / NUM_SAMPLES;
  return (avg_cnt / ADC_MAX_COUNTS) * VREF;
}

// corrects for sensor offset error -> updated measuredVout0A 
void offset() {
  for (int ch = 0; ch < NUM_CH; ch++) {
    VOUT_0A[ch] = read_VOUT_avg(ch);
  }
}

// channel -> curr (A) 
float read_curr(uint8_t ch) {
  float vout = read_VOUT_avg(ch);
  return (vout - VOUT_0A[ch]) / SENSITIVITY;
}

// check if overcurr -> overcurr bool (per channel) 
bool check_overcurr(uint8_t ch, float curr) {
  float mag = fabs(curr);

  if (!overcurr[ch]) {
    if (mag >= OVERCURR_THRESHOLD) {
      if (overcurr_start_time[ch] == 0) {
        overcurr_start_time[ch] = millis();
      } else if (millis() - overcurr_start_time[ch] >= OVERCURR_DUR) {
        overcurr[ch] = true;
      }
    } else { // curr drops 
      overcurr_start_time[ch] = 0; // reset duration 
    }
  } else { // already overcurr 
    if (mag < OVERCURR_CLEAR) { // drops below threshold 
      overcurr[ch] = false;
      overcurr_start_time[ch] = 0;
    }
  }

  return overcurr[ch];
}

// setup + loop 
void setup() {
  Serial.begin(115200);
  pinMode(CURR_SENSE_PIN, OUTPUT);
  digitalWrite(CURR_SENSE_PIN, HIGH);  // CS idles high
  pinMode(ADC_SCLK_PIN, OUTPUT);
  digitalWrite(ADC_SCLK_PIN, LOW);     // SCLK idles low
  pinMode(ADC_SDI_PIN, OUTPUT);
  digitalWrite(ADC_SDI_PIN, LOW);
  pinMode(ADC_SDO_PIN, INPUT);
  pinMode(ADC_EOC_PIN, INPUT);
  adc_transfer(0x00);
  delay(1);

  // initialize each channel state 
  for (int ch = 0; ch < NUM_CH; ch++) {
    VOUT_0A[ch] = IDEAL_VOUT_0A;
    overcurr[ch] = false;
    overcurr_start_time[ch] = 0;
  }

  Serial.println("Current Sensing! ");
  Serial.println("correcting sensor offset error... ");
	delay(500);
	// comment this out UNLESS RUNNING WITH 0A CURR
	//offset();
  Serial.print("zero-current VOUT (ch0): "); 
  Serial.print(VOUT_0A[0], 4);
  Serial.println(" (V)");
}

void loop() {
  Serial.print("Current: ");
  for (int ch = 0; ch < NUM_CH; ch++) {
    float curr = read_curr(ch);
    bool x = check_overcurr(ch, curr);

    Serial.print("[");
    Serial.print(ch);
    Serial.print("] ");
    Serial.print(curr, 3);
    Serial.print(" (A)");

    if (x) {
      Serial.print("!!OVERCURRENT!!");
    }
    Serial.print("  ");
  }

  Serial.println();
  
  if (adc_eoc_fault) {
    Serial.println("!! EOC TIMEOUT - check EOC wiring/pin !!");
    adc_eoc_fault = false;
  }
  
  delay(50); // sample rate 
}
