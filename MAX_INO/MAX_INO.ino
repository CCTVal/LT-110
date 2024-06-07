#include "string.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdlib.h"
#include "PWFusion_MAX31856.h" // Thermocouples reader chip

#define MAX_COMND_COUNT 3 // Número máximo de veces consecutivas para el mismo carácter
#define NUM_THERMOCOUPLES   (sizeof(tcChipSelects) / sizeof(uint8_t)) // number of thermocouples to read

uint8_t tcChipSelects[] = {7, 8, 9, 10};  // define chip select pins for each thermocouple
MAX31856 thermocouples[NUM_THERMOCOUPLES];

char incoming_byte;
char last_char;
int same_char_count;
int chars_to_expect;
int temperatures[16];
char temperatures_msg[70];

void setup() {
  delay(1000); // Give the MAX31856 a chance to stabilize
  // Initialize each MAX31856... options can be seen in the PWFusion_MAX31856.h file
  for (int i=0; i<NUM_THERMOCOUPLES; i++)
  {
    thermocouples[i].begin(tcChipSelects[i]);
    thermocouples[i].config(K_TYPE, CUTOFF_50HZ, AVG_SEL_4SAMP, CMODE_AUTO); // TODO change to 60Hz for USA
  }
  last_char = '\0'; // Variable para almacenar el último carácter recibido
  same_char_count = 0; // Contador para contar la cantidad de veces consecutivas que se ha recibido el mismo carácter
  chars_to_expect = -1; // Counter for the digits that should be sent from the SMART in commands like GGG
  pinMode(LED_BUILTIN, OUTPUT);
  temperatures[16]=0;
  read_temperatures();
  Serial.begin(1200, SERIAL_8E1);
}

void name_and_status() {
  Serial.write('1');
  Serial.write('1');
  Serial.write('1');
  Serial.write('0');
  Serial.write('0');
  Serial.write('0');
}

void done() {
  Serial.write('D');
  Serial.write('D');
  Serial.write('D');
}

void read_temperatures() {

  temperatures[0] = 3500;  temperatures[1]  = 3600; temperatures[2]  = 3700; temperatures[3]  = 3800;
  temperatures[4] = 3500;  temperatures[5]  = 3600; temperatures[6]  = 3700; temperatures[7]  = 3800;
  temperatures[8] = 3500;  temperatures[9]  = 3600; temperatures[10] = 3700; temperatures[11] = 3800;
  temperatures[12] = 3500; temperatures[13] = 3600; temperatures[14] = 3700; temperatures[15] = 3800;

  for (int i=0; i<NUM_THERMOCOUPLES; i++)
  {
    // Get latest measurement from MAX31856 channels
    thermocouples[i].sample();
  
    // Print information to serial port
    //print31856Results(i, thermocouples[i]);
    MAX31856 tc = thermocouples[i];
    uint8_t status = tc.getStatus();

    if(status)
    {
      // lots of faults possible at once, technically... handle all 8 of them
      // Faults detected can be masked, please refer to library file to enable faults you want represented
      digitalWrite(LED_BUILTIN, HIGH);
      /*
      if(TC_FAULT_OPEN & status)        { Serial.print(F("OPEN, ")); }
      if(TC_FAULT_VOLTAGE_OOR & status) { Serial.print(F("Overvolt/Undervolt, ")); }
      if(TC_FAULT_TC_TEMP_LOW & status) { Serial.print(F("TC Low, ")); }
      if(TC_FAULT_TC_TEMP_HIGH & status){ Serial.print(F("TC High, ")); }
      if(TC_FAULT_CJ_TEMP_LOW & status) { Serial.print(F("CJ Low, ")); }
      if(TC_FAULT_CJ_TEMP_HIGH & status){ Serial.print(F("CJ High, ")); }
      if(TC_FAULT_TC_OOR & status)      { Serial.print(F("TC Range, ")); }
      if(TC_FAULT_CJ_OOR & status)      { Serial.print(F("CJ Range, ")); }
      */
    }
    else { // no fault, print temperature data
      // cold_junction_temperature = tc.getColdJunctionTemperature();
      int temp = int(tc.getTemperature() * 100);
      if (temp < 1000)
      {
        temp = 1000;
      } else if(temp > 9999) {
        temp = 9999;
      }
      temperatures[i] = temp;
    }
  
    // Attempt to recove misconfigured channels
    if(thermocouples[i].getStatus() == 0xFF)
    {
      thermocouples[i].config(K_TYPE, CUTOFF_60HZ, AVG_SEL_1SAMP, CMODE_AUTO);
      digitalWrite(LED_BUILTIN, HIGH); // Tell there is some error
    }
  }

  
  //temperatures_msg = "EEE3500360037003800350036003700380035003600370038003500360037003800LH";
}

void send_temperatures() {
  char to_send[8] = "3665";
  Serial.write('E');
  Serial.write('E');
  Serial.write('E');
  unsigned checksum = 0;
  for(int i = 0; i < 16; i++) {
    itoa(temperatures[i], to_send, 10);
    for(int j = 0; j < 4; j++) {
      Serial.write(to_send[j]);
      checksum += to_send[j];
    }
  }
  Serial.write(checksum % 256);
	Serial.write(checksum / 256);
}

void loop() {
  if (Serial.available() > 0) {
    // read the incoming byte:
    incoming_byte = Serial.read();
    if(incoming_byte == last_char) {
      same_char_count++;
    } else {
      same_char_count = 1;
    }
    if(same_char_count == 3) {
      same_char_count = 0;
      switch(last_char) {
        case 'N':
          name_and_status();
          break;
        case 'I':
          done();
          break;
        case 'T':
          send_temperatures();
          read_temperatures();
          break;
        case 'R':
          send_temperatures();
          read_temperatures();
          break;
        default:
          digitalWrite(LED_BUILTIN, HIGH);
          break;
      }
      
    }
    last_char = incoming_byte;
  }
  // put your main code here, to run repeatedly:

}
