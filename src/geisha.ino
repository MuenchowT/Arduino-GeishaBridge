#include <Homie.h>
#include "SoftwareSerial.h"
#include <time.h>
#include "geisha.h"
#include <string>

unsigned long RefreshInterval = 10;
unsigned long DebugLevel = 1;
unsigned long lastSent = millis();

int dirtyCnt = 0;
int dirtyMap[MAX_DIRTY_MAP];

/*
 * SoftwareSerial.cpp is the version in the current directory
 * it has been modified per Trial/error, until communication worked
*/
SoftwareSerial swSerHeatPump(rxPin, txPin, false, 4);
/* 
 * Homie documentation:
 * https://homieiot.github.io/homie-esp8266/docs/3.0.0/others/cpp-api-reference/
 * https://github.com/homieiot/homie-esp8266
*/
HomieNode PanaNode("PanaEdel42", "PanaEdel42", "MDC05F3E5");

geishaStruct *geishaMap[MAX_PARAM_NUMBER + 1];

 void createGeishaMap(){
  /* The Array "GeishaMap" is an arrays of Pointers to GeishaStruct Types. 
   *  See the definition af the Array "geishaParams", which defines all the Parameter-Numbers
   *  used to communicate with the heatpump and/or with MQTT.
   *  Unfortunately, the lowest registerNumber is 17 (0 would by handy). 
   *  
  ** the geishaMap has MAX_PARAM_NUMBER (=144) entries, but only some of them have a valid pointer to a geishaParam, 
  *  as not all register-numbers from 0 ... 144 exist. 
  *  So, for the non-existing register-numbers the geishaMap-Entries will be NULL.
  ** But, in case a register-Number exists, the geishaMap entry geishaMap[i] will point to the geishaParams[n]
  *  where n != i, but i is the actual register_number.
  *  In Fact, this is a "cheapo" Associative Array (aka C++-Map). By referencing geishaMap[register-Number],
  *  we directly get the correct GeishaStruct for this register-Number
  *  .
  *  GeishaMap[i] points to ParamStruct
  *   0             NULL
  *   1             NULL
  *   ...
  *   17            ParamStruct[0] (pana_register=17)
  *   18            ParamStruct[1] (pana_register=18) 
  */
   // intiialize geishaMap:
  for (int i = 0; i <= MAX_PARAM_NUMBER; i++) {      // For all register-numbers from 0 to "Max-register-Number"
#ifdef DEBUG
     if ( DebugLevel >= 2 ) Serial <<  "Map[" << i << "]: ";
#endif
      geishaMap[i] = getParamStructFromRegister(i, GEISHA_BIT);
#ifdef DEBUG
       if ( DebugLevel >= 2 ) Serial << "Map found: " <<  geishaMap[i]->pana_register<< ", " <<  geishaMap[i]->pana_name << endl;
#endif       
   }
   // manual corr for register 27 <-> 144. a value for register 144  will be writtten into the 27-struct.
      geishaMap[144] = geishaMap[27];
 }

/*
 *  Main Arduino loop: will be called by "Homie_Loop", don't call it manually!
 *  Handles serial communication between Arduino and geisha, 
 *  translates and sends Data from geisha to MQTT
 */
void loopHandler() {

  // heatpump_loop will take care of the serial wire communication between arduino and geisha
  heatpump_loop();

// Every "RefreshInterval"-Seconds, send values from geisha to MQTT
  if ( (millis() - lastSent) >= (RefreshInterval * 1000UL) ) {
    lastSent = millis();
#ifdef DEBUG    
    if (DebugLevel >= 1)  Homie.getLogger() << "30s refresh pana->mqtt: " << endl ;
#endif    
    // loop over Parameters-Array and calculate the MQTT values from the pana Parameter-Values
    for (int i = 0; i < PARAMS_ARRAY_SIZE; i++) {
//      geishaStruct * gs = &geishaParams[i];
      
      if (geishaParams[i].pana_map & MQTT_BIT) { 
      int p = geishaParams[i].pana_register;
      switch (p) {
        case 1: // Betriebsart
           geishaParams[i].pana_value_string = getModeStringFromValue(geishaMap[27]->pana_value); 
           break;
        case 2: // Power
           geishaParams[i].pana_value_string = ( (geishaMap[27]->pana_value & ON_BIT) ? "ON" : "OFF" ); 
           break;
        case 3: // Eco
           geishaParams[i].pana_value_string = ( (geishaMap[27]->pana_value & 64) ? "ON" : "OFF" ); 
           break;
        case 4: // akt Stromverbrauch
           geishaParams[i].pana_value_string =  String(getAktStromverbrauch());
           break;
        case 17:  //Abtauen
           geishaParams[i].pana_value_string =  ( (geishaParams[i].pana_value & 64) ? "ON" : "OFF" );
           break;
        case 30: 
        case 32:
        case 34:
            geishaParams[i].pana_value_string =  String(geishaMap[p]->pana_value * 256 + geishaMap[p-1]->pana_value);
//          if (DebugLevel >= 2)  Homie.getLogger() << i << ": " << geishaMap[p]->pana_value_string << endl ;
          break;
        case 35: // Pumpe:  16=1, 32=2, 48=3, 64=4
            geishaParams[i].pana_value_string = String(geishaParams[i].pana_value / 16);
            break;
        case 138: // Sollwertverschiebung: 
              geishaParams[i].pana_value_string = String(geishaParams[i].pana_value - ( (geishaParams[i].pana_value & 128) ? 256 : 0)); 
//              if (DebugLevel >= 2)  Homie.getLogger() << "Sollwertv. original: " <<  geishaParams[i].pana_value  << endl ;
              break;
         default:
           geishaParams[i].pana_value_string = String(geishaMap[p]->pana_value); 
       }
#ifdef DEBUG
       if (DebugLevel >= 1) Homie.getLogger() << geishaParams[i].pana_name << "/" << geishaParams[i].pana_value_string << endl;
#endif
       PanaNode.setProperty(geishaParams[i].pana_name).send(geishaParams[i].pana_value_string);
       yield();
      }   // pana_map & MQTT_BIT
    }   // for (i=0
    PanaNode.setProperty("RefreshInterval").send(String(RefreshInterval));
    PanaNode.setProperty("DebugLevel").send(String(DebugLevel));
  }   // every 30s
}

/*
 *  GlobalInputHandler will receive values from MQTT and translate the changes to geisha-parameters:
 *  They will be stored in the global array geishaParams and flagged as Dirty.
 *  When the Serial communication sees a register-17-packet from geisha, the next packet
 *  (170-18-nn-nn) from KFB to geisha bill be dropped and one of the "dirty" parameters
 *  will be sent to geisha
 */
bool globalInputHandler(const HomieNode& node, const HomieRange& range, const String& property, const String& value) {

Homie.getLogger() << "global input handler for " << property << endl;;
uint8_t new_mode = 0;
int tmp ;
int p_register;

geishaStruct * gs = getParamStructFromName(property, MQTT_BIT);
p_register = gs->pana_register;

if (gs->pana_register == 0) {  Homie.getLogger() << "Parameter " << property << "not found " << endl;  return false; }

    switch (gs->pana_register) {
      case 1:  // Betriebsart
        new_mode = getModeBitsFromString(value);        // mode value in bits 2--8 | 
        // original OFF/ON bit from parameter 144 "appended" to new value
        new_mode |= ( geishaMap[144]->pana_value & 0x01 ); 
#ifdef DEBUG
            Homie.getLogger() << " register, " << p_register << " new mode from mqtt: " << new_mode << " is set to " << value << endl;
#endif
         p_register = 144;
        break;
      case 2:  // Power
        if (value != "ON" && value != "OFF") 
          return false;
// strip the Power bit (LSB) off the mode-Number and add the new power bit
        new_mode = (geishaMap[144]->pana_value & 0xFE) | (value == "ON" ? 1 : 0 );  
#ifdef DEBUG
            Homie.getLogger() << " register, " << p_register << " new power bit  from mqtt: " << new_mode << " is set to " << value << endl;
#endif
         p_register = 144;
        break;
      case 3:  // Eco
        if (value != "ON" && value != "OFF") 
          return false;
        // strip the ECO-bit off the mode-number (by applying 0B10111111 = 0xBF) and add the new eco-bit
        new_mode = (geishaMap[144]->pana_value & 0xBF) | (value == "ON" ? 64 : 0 );
         p_register = 144;
        break;
      case 138: // Sollwertverschiebung, can be negative
        tmp = strtol(const_cast<char*>(value.c_str()), NULL, 0);
          // negativ swv's will be manipulated
        new_mode = uint8_t(tmp) - (tmp < 0 ? 256 : 0);
        break;
      default:  // all other mqtt-properties map 1:1 to geisha-parameters
        new_mode = strtol(const_cast<char*>(value.c_str()), NULL, 0);
     }
#ifdef DEBUG 
      Homie.getLogger() << gs->pana_name << " is set to " << value << " -> " << new_mode << endl;
#endif
  sendNewValueToGeisha(p_register, new_mode);
return true;
}


bool RefreshHandler(const HomieRange& range, const String& value) {
  RefreshInterval = strtol(const_cast<char*>(value.c_str()), NULL, 0);
  PanaNode.setProperty("RefreshInterval").send(value);
  Homie.getLogger() << "RefreshInterval is set to " << value << endl;
return true;
  }

bool DebugHandler(const HomieRange& range, const String& value) {
  DebugLevel = strtol(const_cast<char*>(value.c_str()), NULL, 0);
  PanaNode.setProperty("DebugLevel").send(value);
  Homie.getLogger() << "DebugLevel is set to " << value << endl;
  return true;
  }

bool rel1Handler(const HomieRange& range, const String& value) {
return true;
  }

bool rel2Handler(const HomieRange& range, const String& value) {
return true;

  }
bool rel3Handler(const HomieRange& range, const String& value) {
return true;

  }
bool rel4Handler(const HomieRange& range, const String& value) {
return true;

  }


void setup() {
   Homie.disableLedFeedback();
//    Homie.disableResetTrigger();
  Serial.begin(115200);
//  Serial << endl << endl;
  
  Homie_setFirmware("Geisha Proxy", "1.0.0"); // The underscore is not a typo! See Magic bytes
  Homie.setSetupFunction(heatpump_setup).setLoopFunction(loopHandler);
  
  // globalInputHandler will receive values from MQTT: 
  Homie.setGlobalInputHandler(globalInputHandler); 

 // Publish values defined in the global array geishaParams to MQTT:
 for (int i = 1; i < PARAMS_ARRAY_SIZE; i++) {
    if (geishaParams[i].pana_map & MQTT_BIT) { 
      if (geishaParams[i].pana_map & SETTABLE_BIT) 
        PanaNode.advertise(geishaParams[i].pana_name).setName(geishaParams[i].pana_name).setDatatype(DATATYPE_MAP(&geishaParams[i])).settable();
      else
        PanaNode.advertise(geishaParams[i].pana_name).setName(geishaParams[i].pana_name).setDatatype(DATATYPE_MAP(&geishaParams[i]));
    }
 }

  // publish internal and other values to MQTT:
  PanaNode.advertise("RefreshInterval").settable(RefreshHandler).setName("Refresh Interval").setDatatype("integer");
  PanaNode.advertise("DebugLevel").settable(DebugHandler).setName("DebugLevel").setDatatype("integer");
  PanaNode.advertise("Relais_1").settable(rel1Handler).setName("Relais_1").setDatatype("STRING");
  PanaNode.advertise("Relais_2").settable(rel2Handler).setName("Relais_2").setDatatype("STRING");
  PanaNode.advertise("Relais_3").settable(rel3Handler).setName("Relais_3").setDatatype("STRING");
  PanaNode.advertise("Relais_4").settable(rel4Handler).setName("Relais_4").setDatatype("STRING");

  // see documentation above
  createGeishaMap();
  Homie.setup();
}

void loop() {
  Homie.loop();
}
