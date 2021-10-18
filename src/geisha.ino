#include <Homie.h>
#include "SoftwareSerial.h"
#include <time.h>
#include "geisha.h"
#include <string>

unsigned long RefreshInterval = 10;
unsigned long DebugLevel = 1;
unsigned long Uptime = 0;
unsigned long lastSent = millis();

/* 
 * dirtyMap is a FIFO buffer where changed values coming in from MQTT will be stored and later transfered to geisha.
 * Because of ansyncrounus operation, the mqtt-handler pushes() register-numbers into this buffer
 * and additionally mark them as "dirty" in the "geishaStruct". The serial-communication function will then
 * pop() them off the buffer and transfer them to geisha. 
 */
int dirtyCnt = 0;
int dirtyMap[MAX_DIRTY_MAP];

/*
 * SoftwareSerial.cpp is the version in the current directory
 * it has been modified as per Trial/error, until communication worked
*/
SoftwareSerial swSerHeatPump(rxPin, txPin, false, 4);
/* 
 * Homie documentation:
 * https://homieiot.github.io/homie-esp8266/docs/3.0.0/others/cpp-api-reference/
 * https://github.com/homieiot/homie-esp8266
*/
HomieNode PanaNode("PanaEdel42", "PanaEdel42", "MDC05F3E5");

geishaStruct *geishaMap[MAX_PARAM_NUMBER + 1];
  /* The Array "GeishaMap" is an arrays of Pointers to GeishaStruct Types. 
   *  See the definition af the Array "geishaParams", which defines all the Parameter-Numbers
   *  used to communicate with the heatpump and/or with MQTT.
   *  Unfortunately, the lowest registerNumber is 17 (0 would by handy). 
   *  
  *  the geishaMap has MAX_PARAM_NUMBER+1 (=145) entries, but only some have a valid pointer to a geishaParam, 
  *  as not all geisha-register-numbers from 0 ... 144 exist. 
  *  But, in case a register-Number exists, the geishaMap entry geishaMap[i] will point to the geishaParams[n]
  *  where i is the the actual register_number.
  *  In fact, this is a "cheapo" associative Array (aka C++-Map). By referencing geishaMap[register_Number],
  *  we directly get the correct GeishaStruct for this register-Number. Obviously, this trades cost of speed against 
  *  memory size.
  *  .
  *  GeishaMap[i] points to ParamStruct
  *   0             0
  *   1             0
  *   ...
  *   17            ParamStruct[5] (pana_register = 17 !)
  *   18            ParamStruct[6] (pana_register = 18 !) 
  */
 void createGeishaMap(){
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
 *  translates and sends Data from MQTT to geisha
 */
void loopHandler() {

  // heatpump_loop will take care of the serial wire communication between arduino and geisha
  heatpump_loop();

// Every "RefreshInterval"-Seconds, send values from geisha to MQTT, else return
  if ( (millis() - lastSent) < (RefreshInterval * 1000UL) ) 
    return;
    
    Uptime += (millis() - lastSent)/1000UL;
    lastSent = millis();

#ifdef DEBUG    
    if (DebugLevel >= 1)  Homie.getLogger() << "30s refresh pana->mqtt: " << endl ;
#endif    
    
    PanaNode.setProperty("RefreshInterval").send(String(RefreshInterval));
    PanaNode.setProperty("DebugLevel").send(String(DebugLevel));
    PanaNode.setProperty("Uptime").send(String(Uptime));

    // loop over Parameters-Array and calculate the MQTT values from the pana Parameter-Values
    for (int i = 0; i < PARAMS_ARRAY_SIZE; i++) {
      if (geishaParams[i].pana_map & MQTT_BIT) { 
//        SendPanaValueToMqtt(&geishaParams[i]);
      int p = geishaParams[i].pana_register;
      switch (p) {
        case 1: // Betriebsart
           if (geishaMap[27]->pana_value & 1)
             geishaParams[i].pana_value_string = getModeStringFromValue(geishaMap[27]->pana_value); 
           else
             geishaParams[i].pana_value_string = "aus"; 
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
        #ifdef DEBUG
           Homie.getLogger() << "Sollwertv. original: " <<  geishaParams[i].pana_value  << " neu: " << String(geishaParams[i].pana_value - uint8_t( (geishaParams[i].pana_value & 128) ? 256 : 0)) << endl ;
        #endif
              geishaParams[i].pana_value_string = String(geishaParams[i].pana_value - uint8_t( (geishaParams[i].pana_value & 128) ? 256 : 0)); 
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
}

/*
 *  GlobalInputHandler will receive values from MQTT and translate the changes to geisha-parameters:
 *  They will be stored in the global array geishaParams and flagged as Dirty.
 *  Additionally, they will be pushed into the fifo-buffer "dirtyMap".
 *  When the Serial communication encounters a register-17-packet from geisha, the next "dirty" register-number
 *  will be "popped" off the fifo-buffer. The next packet from remote (170-18-nn-nn) 
 *  bill be dropped by switching the "RTS"-Pin to HIGH and thus, opening  the TX wire from the remote.
 *  An appropriate packet for the "dirty" parameter will be sent instead. 
 */
bool globalInputHandler(const HomieNode& node, const HomieRange& range, const String& property, const String& value) {
uint8_t new_mode = 0;
int tmp ;
geishaStruct * gs = getParamStructFromName(property, MQTT_BIT);
int p_register = gs->pana_register;

Homie.getLogger() << "global input handler for " << property << "-> " << value << endl;;
// For manual mqtt properties, the exact property-Handler for the property in question will be called (by returning "false")
if (property == "RefreshInterval" || property == "DebugLevel" 
    || property == "Relais_1" || property == "Relais_2")
    return false;

if (gs->pana_register == 0) {  
    Homie.getLogger() << "Parameter " << property << "not found " << endl;  
    return false; 
    }

switch (gs->pana_register) {
    case 1:  // Betriebsart
      if (value == "aus") {
        new_mode = (geishaMap[144]->pana_value & 0xFE);  // strip the ON/OFF bit off the mode-bitmap
        Homie.getLogger() << " register, " << p_register << " new mode from mqtt: " << new_mode << endl;
        }
      else {
        new_mode = getModeBitsFromString(value);        // mode value in bits 2--8 | 
//      if ( ! (geishaMap[144]->pana_value & 0x01) ) // wenn pana aus
        new_mode |= 1;                              // anschalten
      }
      // original OFF/ON bit from parameter 144 "appended" to new value
//      new_mode |= ( geishaMap[144]->pana_value & 0x01 ); 
    #ifdef DEBUG
          Homie.getLogger() << " register, " << p_register << " new mode from mqtt: " << new_mode << endl;
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
Homie.getLogger() << gs->pana_name << " is set from " << gs->pana_value <<   " to " << value << " -> " << new_mode << endl;
//DebugLevel
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
  if (value != "ON" && value != "OFF") 
    return false;
  digitalWrite(relays[0], (value == "ON" ? HIGH : LOW ));
  PanaNode.setProperty("Relais_1").send(value);
  Homie.getLogger() << "Relais 1 is set to " << value << endl;
return true;
}

bool rel2Handler(const HomieRange& range, const String& value) {
  if (value != "ON" && value != "OFF") 
    return false;
  digitalWrite(relays[1], (value == "ON" ? HIGH : LOW ));
  PanaNode.setProperty("Relais_2").send(value);
  Homie.getLogger() << "Relais 2 is set to " << value << endl;
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

  // publish internal and other (non-geisha) values to MQTT:
  PanaNode.advertise("RefreshInterval").settable(RefreshHandler).setName("Refresh Interval").setDatatype("integer");
  PanaNode.advertise("DebugLevel").settable(DebugHandler).setName("DebugLevel").setDatatype("integer");
  PanaNode.advertise("Uptime").setName("Uptime").setDatatype("integer");
  PanaNode.advertise("Relais_1").settable(rel1Handler).setName("Relais_1").setDatatype("STRING");
  PanaNode.advertise("Relais_2").settable(rel2Handler).setName("Relais_2").setDatatype("STRING");
//  PanaNode.advertise("Relais_3").settable(rel3Handler).setName("Relais_3").setDatatype("STRING");
//  PanaNode.advertise("Relais_4").settable(rel4Handler).setName("Relais_4").setDatatype("STRING");

  // see documentation above
  createGeishaMap();
  Homie.setup();

// Watchdog:
//  ESP.wdtDisable();
// ESP.wdtEnable(2000);
    
}

void loop() {
  Homie.loop();
}
