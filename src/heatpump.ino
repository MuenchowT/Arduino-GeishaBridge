unsigned long lastReadAt = 0;
uint8_t packageData[4];
int cntRead = 0;
uint8_t sentRegister = 0;
uint8_t sentValue = 0;

void heatpump_setup() {  
  Homie.getLogger() << " - heatpump interface setup ...." ;
  // this is onother story, no geisha stuff
  pinMode(windPin, INPUT_PULLUP);
  //attachInterrupt(WindPin, function, FALLING);
  for (int i = 0; i < 2; i++) {
      pinMode(relays[i], OUTPUT);
//      digitalWrite(relays[i], LOW);

  }

  //digitalWrite(LED_BUILTIN, LOW);

  swSerHeatPump.begin(960);
  swSerHeatPump.enableIntTx(false);
  swSerHeatPump.setTransmitEnablePin(rtsPin);
  }



/*
 * changed values will be put into the global array and sent every 30s (RefreshInterval)
 */
inline void UpdateNewPanaValue(uint8_t p_register, uint8_t p_value , bool sendImmediate) { 
#ifdef DEBUG
  Homie.getLogger() << "Register " << p_register << "/mapped: " << geishaMap[p_register]->pana_register << " (" << geishaMap[p_register]->pana_name <<
      ") from pana, value=" << geishaMap[p_register]->pana_value << endl;
#endif

if (  geishaMap[p_register]->pana_register == 0 || p_register > MAX_PARAM_NUMBER   || isParamDirty(geishaMap[p_register]) ) 
    return;
    
  geishaMap[p_register]->pana_value = p_value; 

#ifdef DEBUG
    Homie.getLogger() << "Register " << p_register << " (" << geishaMap[p_register]->pana_name <<
      ") from pana, value=" << geishaMap[p_register]->pana_value << endl;
#endif

}

/*
 * Main loop called form homie/arduino, handles the serial communication between remote and geisha
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
    }  // Heatpump.available()

    if (cntRead == 4 )  {     // After reading a complete block of 4 Packets:
#ifdef DEBUG
      if ( DebugLevel >= 3 ) 
        Homie.getLogger() << "Serial availabe done: " << (millis() - lastReadAt) << ", s1: " << packageData[1] << ", s2: " << packageData[2] << endl;
#endif
      cntRead = 0;
      if (packageData[0] == 85) {       // If origin of packets is heatpump:
          // maybe it's an answer of one of the packets we sent:
/*         if (sentRegister == packageData[1] && sentValue == packageData[2]) {
          UpdateNewPanaValue(packageData[1], packageData[2], true);
          sentRegister = 0;
          sentValue  = 0;
         }
         else*/
          UpdateNewPanaValue(packageData[1], packageData[2], false);
         

#ifdef DEBUG 
        if (DebugLevel >= 2) {
            char msg[80];
            sprintf(msg, "%u - %u - %u - %u",   packageData[0] ,  packageData[1] , packageData[2], packageData[3] );
            if ( DebugLevel >= 1 ) Homie.getLogger() << String(msg) << endl;
          }
#endif

  /* 
  *  When we see a complete register-17-packet (MAGICK_PACKET) from geisha, the next packet
 *  (170-18-nn-nn) from KFB to geisha will be dropped  bill be dropped by switching the "RTS"-Pin to HIGH 
 *  and thus, causing the PCB board to open the TX wire from the remote. Instead, the Arduino-TX wire will be
 *  connected to the Geisha RX-Pin. 
 *  The next "Dirty" register-number will be read from the fifo-buffer "dirtyMap", if availabe (dirtyCnt > 0).  
 *  An appropriate packet for the this register will be sent instead. When successful, the dirty register will be Popped()
 *  off the buffer. Otherwise it stays in the buffer, and maybe after a round-robin, sent again.
 */
        if (packageData[1] == MAGIC_PACKET && dirtyCnt > 0 ) { 
          uint8_t p_register = sentRegister = getNextDirtyParam();
          if (p_register == 0) return;
          uint8_t p_value = sentValue = geishaMap[p_register]->pana_value;
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
  
  
