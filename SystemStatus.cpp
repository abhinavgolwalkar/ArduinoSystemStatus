/*

*/


#include "SystemStatus.h"
#include <avr/sleep.h>
#include <avr/power.h>
//#include "wiring_private.h"


SystemStatus::SystemStatus(uint8_t apin_batt) : pin_batt(apin_batt) {

}


int SystemStatus::getVCC() {
  //reads internal 1V1 reference against VCC
  #if defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0); // For ATtiny84
  #elif defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2); // For ATtiny85
  #elif defined(__AVR_ATmega1284P__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  // For ATmega1284
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  // For ATmega328
  #endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  uint8_t low = ADCL;
  unsigned int val = (ADCH << 8) | low;
  //discard previous result
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  low = ADCL;
  val = (ADCH << 8) | low;
  
  return ((long)1024 * 1100) / val;  
}


int SystemStatus::getVBatt(int vcc) {
  unsigned int a = analogRead(this->pin_batt); //discard first reading
  a = analogRead(this->pin_batt);
  return (long)vcc * (long)a / 1024;
}


int SystemStatus::getFreeRAM() {
  extern int  __bss_end;
  extern int  *__brkval;
  int free_memory;
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
  }
  return free_memory;
}


int SystemStatus::getkHz() {
  return F_CPU / 1000;
}


int SystemStatus::getMHz() {
  return F_CPU / 1000000;
}


int8_t SystemStatus::getTemperatureInternal() {
  /* from the data sheet
    Temperature / �C -45�C +25�C +85�C
    Voltage     / mV 242 mV 314 mV 380 mV
  */
  ADMUX = (1<<REFS0) | (1<<REFS1) | (1<<MUX3); //turn 1.1V reference and select ADC8
  delay(2); //wait for internal reference to settle
	// start the conversion
	ADCSRA |= bit(ADSC);
	//sbi(ADCSRA, ADSC);
	// ADSC is cleared when the conversion finishes
	while (ADCSRA & bit(ADSC));
	//while (bit_is_set(ADCSRA, ADSC));
	uint8_t low  = ADCL;
	uint8_t high = ADCH;
	//discard first reading
	ADCSRA |= bit(ADSC);
	while (ADCSRA & bit(ADSC));
	low  = ADCL;
	high = ADCH;
	int a = (high << 8) | low;
  return a - 272; //return temperature in C
}


void SystemStatus::SleepWakeOnInterrupt(uint8_t i) {
  cli();
  attachInterrupt(i, jebat_cecky, LOW);
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  power_adc_disable();
  power_spi_disable();
  power_timer0_disable();
  power_timer1_disable();
  power_timer2_disable();
  power_twi_disable();  
  sleep_enable();
  sleep_bod_disable();
  sei(); //first instruction after SEI is guaranteed to execute before any interrupt
  sleep_cpu();
  
  /* The program will continue from here. */
  
  /* First thing to do is disable sleep. */
  sleep_disable();
  power_all_enable();

}

void jebat_cecky() {
  detachInterrupt(0);
}


//