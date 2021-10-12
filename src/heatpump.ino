unsigned long lastReadAt = 0;
uint8_t packageData[4];
int cntRead = 0;


void heatpump_setup() {  
  Homie.getLogger() << " - heatpump interface setup ...." ;
  // this is onother story, no geisha stuff
  pinMode(windPin, INPUT_PULLUP);
  //attachInterrupt(WindPin, function, FALLING);

  swSerHeatPump.begin(960);
  swSerHeatPump.enableIntTx(false);
  swSerHeatPump.setTransmitEnablePin(rtsPin);
  }

/*
 * In case some value from geisha has changed, we will send the value immediately to mqtt (not yet implemented)
 * anyway, value will be set in the global array and sent every 30s (RefreshInterval)
 */
inline void sendNewValueToMqtt(uint8_t p_register, uint8_t p_value) { 
#ifdef DEBUG
  if (DebugLevel >= 2)  
  Homie.getLogger() << "Register " << p_register << "/mapped: " << geishaMap[p_register]->pana_register << " (" << geishaMap[p_register]->pana_name <<
      ") from pana, value=" << geishaMap[p_register]->pana_value << endl;
#endif

if (  geishaMap[p_register]->pana_register == 0 || p_register > MAX_PARAM_NUMBER   || isParamDirty(geishaMap[p_register]) ) 
    return;
    
  geishaMap[p_register]->pana_value = p_value; 

#ifdef DEBUG >= 1
  if (DebugLevel >= 1) 
    Homie.getLogger() << "Register " << p_register << " (" << geishaMap[p_register]->pana_name <<
      ") from pana, value=" << geishaMap[p_register]->pana_value << endl;
#endif

//   if ( isMqttParam(geishaMap[p_register]) ) 
//       PanaNode.setProperty(geishaMap[p_register]->pana_name).send(p_value);
}

/*
 * Main loop called form homie/arduino
 */
void heatpump_loop() {
  int sinceLast = 0;
  
// read Pakets from serial rx:
  while (swSerHeatPump.available() > 0) {
    uint8_t packet = swSerHeatPump.read();
    sinceLast = millis() - lastReadAt;
    lastReadAt = millis();    
    if ( sinceLast > 30 ) {  // Start-Paket
      cntRead = 0;
      if (packet != 170 && packet != 85) {
          Homie.getLogger() << sinceLast << "ms, uncorrect packet " << packet << " received"  << endl;
          continue;
        }
      }
    packageData[cntRead++] = packet;

#ifdef DEBUG
  if ( DebugLevel >= 2 ) 
      Homie.getLogger() << "Serial: " << sinceLast << "ms, Package [" << cntRead << "] read: " << packet <<  " received" << endl;
#endif      
    }  // Serial.available()

    if (cntRead == 4 )  {     // After reading a complete block of 4 Packets:
#ifdef DEBUG
      if ( DebugLevel >= 3 ) 
        Homie.getLogger() << "Serial availabe done: " << (millis() - lastReadAt) << ", s1: " << packageData[1] << ", s2: " << packageData[2] << endl;
#endif
      cntRead = 0;
      if (packageData[0] == 85) {       // If origin of packets is heatpump:
          // update value in the global Value-Array:
         sendNewValueToMqtt(packageData[1], packageData[2]);

#ifdef DEBUG 
        if (DebugLevel >= 2) {
            char msg[80];
            sprintf(msg, "%u - %u - %u - %u",   packageData[0] ,  packageData[1] , packageData[2], packageData[3] );
            if ( DebugLevel >= 1 ) Homie.getLogger() << String(msg) << endl;
          }
#endif

  /* 
  *  When we see a complete register-17-packet from geisha, the next packet
 *  (170-18-nn-nn) from KFB to geisha will be dropped and one of the "dirty" parameters
 *  will be popped from the "dirty"-Array and sent to geisha. 
 */
        if (packageData[1] == MAGIC_PACKET && dirtyCnt > 0 ) { 
          
          //sendCommandsToGeisha();
          uint8_t p_register = getNextDirtyParam();
          if (p_register == 0) return;
          uint8_t p_value = geishaMap[p_register]->pana_value;
          uint8_t source = 170;
          uint8_t checksum = (source + p_register + p_value) & 255;
        // 170 - 144 - 17 - 75
#ifdef DEBUG_TX
          if (DebugLevel >= 1) 
              Homie.getLogger() << "Send segister " << p_register << " value " << p_value << " to HeatPump: " << endl;
          Homie.getLogger() << "rts high " << (millis() - lastReadAt) ;
#endif
          digitalWrite(rtsPin, HIGH );
          delay(25);    
#ifdef DEBUG_TX
          Homie.getLogger() << ".. 170 " << (millis() - lastReadAt) << "ms "  ;
#endif
          if (!swSerHeatPump.write(source)) { Homie.getLogger() << " failed  "; digitalWrite(rtsPin, LOW); return; }
          delay(5);    
#ifdef DEBUG_TX
          Homie.getLogger() << "... register " << p_register << " " << (millis() - lastReadAt) << "ms " ;
#endif
          if (!swSerHeatPump.write(p_register)) { Homie.getLogger() << " failed  "; digitalWrite(rtsPin, LOW); return; }
          delay(5);    
#ifdef DEBUG_TX
           Homie.getLogger() << " ... value " << p_value << " " << (millis() - lastReadAt) << "ms " ;
#endif
          if (!swSerHeatPump.write(p_value)) { Homie.getLogger() << " failed  ";  digitalWrite(rtsPin, LOW); return; }
          delay(5);    
#ifdef DEBUG_TX
          Homie.getLogger() << "  ... checksum " << checksum << ", " << (millis() - lastReadAt) << "ms " ;
#endif
          if (!swSerHeatPump.write(checksum)) { Homie.getLogger() << " failed  " ; digitalWrite(rtsPin, LOW); return; }
          delay(5);  
#ifdef DEBUG_TX
          Homie.getLogger() << "rts low " << (millis() - lastReadAt) << "ms " << endl;
#endif
          digitalWrite(rtsPin, LOW);
#ifdef DEBUG_TX
          Homie.getLogger() << "rts low done" << endl;
#endif
          //reset the dirty flag of the just transmitted parameter and pop it off she stack
          popDirtyParam();

          } // register == 17 && dirty
      } // packageType == 85
    } // cntRead == 4 
  }  // heatpump-loop
  
