const char *  DATATYPE_MAP(geishaStruct * x)  { 
      if (x->pana_map & 0x08 ) 
        return "integer";
      if (x->pana_map & 0x20 ) 
        return "float";
      return "string";
   }

inline uint8_t getModeBitsFromString(String modeString)
{
if (modeString == "heizen")   return 2;
if (modeString == "kuehlen")  return 4;
if (modeString == "tank")     return  16;
if (modeString == "auto")     return 32;
//if (modeString == "eco")     return 64;
}


inline String getModeStringFromValue(uint8_t p_value) {
if (p_value & 2) return "heizen";
if (p_value & 4) return "kuehlen";
if (p_value & 16) return "tank";
if (p_value & 32) return "auto"; 
//if (p_value & 64) return "eco"; 
if (p_value & 1) return "an";
return "aus";
  }


inline void pushParamDirty(int p_register) {

if (dirtyCnt >= MAX_DIRTY_MAP) {
  Homie.getLogger() << "DirtyMap >= " << dirtyCnt << " .. aborted " << endl;
  exit(1);
}

#ifdef DEBUG
  Homie.getLogger() << " push " << p_register  << ", isParamDirty?: " << isParamDirty(geishaMap[p_register]) << endl;
#endif

if (!isParamDirty(geishaMap[p_register]) ) {
#ifdef DEBUG
  Homie.getLogger() << " push from cnt " << dirtyCnt << " to " << dirtyCnt + 1 << 
      ", register: " << p_register << ", geishaMap.register: " << geishaMap[p_register]->pana_register <<  endl;
#endif
// ggf. steht 144 in diryMap, aber geishaMap[144] -> 27
  dirtyMap[dirtyCnt++] = p_register;
  geishaMap[p_register]->pana_map |= DIRTY_BIT;
  }
}

int getNextDirtyParam() {

#ifdef DEBUG
  Homie.getLogger() << " dirtyArray: " << endl;
for (int i = dirtyCnt-1; i>=0; i--)
    Homie.getLogger() << " cnt  " << i << "/" << dirtyMap[i] << ", isdirty: " << isParamDirty(geishaMap[dirtyMap[i]]) << endl;
#endif  
  
  if (dirtyCnt <= 0) { 
    dirtyCnt = 0; 
    return 0; 
    }
  return dirtyMap[dirtyCnt-1];
  }

 void popDirtyParam(void)
{
  // 10111111 =  191 (0xBF)
  // reset the dirty-bit in the bit-map of the last dirty parameter.
  int p_register = dirtyMap[dirtyCnt - 1];

  #ifdef DEBUG
     Homie.getLogger() << " Pop dirtyCnt  " << dirtyCnt << " -> " << dirtyCnt-1 << 
      ", register " << dirtyMap[dirtyCnt - 1] << ", geishaMap->register: " <<  geishaMap[p_register]->pana_register << 
      " map: " << geishaMap[p_register]->pana_map << 
      ", isDirty: " << isParamDirty(geishaMap[p_register]) << endl;
  #endif

  //int p_register = dirtyMap[--dirtyCnt];
  geishaMap[dirtyMap[--dirtyCnt]]->pana_map &= 0xBF;
  
#ifdef DEBUG
  Homie.getLogger() << " after pop register " << geishaMap[dirtyMap[dirtyCnt]]->pana_register << ", map: " << geishaMap[dirtyMap[dirtyCnt]]->pana_map  << ", isParamDirty?: " << 
      isParamDirty(geishaMap[dirtyMap[dirtyCnt]]) << endl;
#endif

}


/*
 * Value from MQTT will be written into the global Array and flagged as "dirty"
 * The register number will be pushed onto the "dirty"-array
 */
bool sendNewValueToGeisha(int p_register, uint8_t new_value) {
#ifdef DEBUG
      Homie.getLogger() << " sendNewValue2Geisha " << p_register << ", newvalue " << new_value << ", oldVal: " << geishaMap[p_register]->pana_value << endl;
#endif

if (geishaMap[p_register]->pana_value == new_value)
  return false;

geishaMap[p_register]->pana_value = new_value;
pushParamDirty(p_register);

return true;
}


inline bool isMqttParam(geishaStruct * x)  { return ( (x->pana_map & MQTT_BIT) ? true : false ); }
inline bool isGeishaParam(geishaStruct * x) { return  (x->pana_map & GEISHA_BIT ? true : false ); }
inline bool isSettableParam(geishaStruct * x)   { return ( (x->pana_map & SETTABLE_BIT) ? true : false ); }
inline bool isParamDirty(geishaStruct * x)   { return ( (x->pana_map) & DIRTY_BIT ? true : false ); }
//inline bool isManualParam(geishaStruct * x)   { return ( (x->pana_map) & MANUAL_BIT ? true : false ); }


// Helper Function: Scan through the GeishaParams-Array and find a specific 
// Register-Number of a requested type (e.g. geisha-params, mqtt-params...):
// by register-number
geishaStruct * getParamStructFromRegister(unsigned int i_register, uint8_t requested_bitmap) {
//   Serial <<  "search Map no " << i_register << " from array size " << PARAMS_ARRAY_SIZE << endl;
for (int s = 0; s < PARAMS_ARRAY_SIZE; s++)  {
  if ( (geishaParams[s].pana_register == i_register) && ( geishaParams[s].pana_map & requested_bitmap ) ) {
    return &geishaParams[s];
    }
  }
return &geishaParams[0];
  }

// Helper Function: Scan through the GeishaParams-Array and find a specific 
// Entry of a requested type (e.g. geisha-params, mqtt-params...):
// by name
geishaStruct * getParamStructFromName(String i_name, uint8_t requested_bitmap) {
for (int s = 0; s < PARAMS_ARRAY_SIZE; s++)   {
  if ( String(geishaParams[s].pana_name) == i_name  && ( geishaParams[s].pana_map & requested_bitmap ) ) 
    return &geishaParams[s];
  }
return &geishaParams[0];
}


int getAktStromverbrauch(void)
{
  int p_register = 0;
  int sb = (geishaMap[30]->pana_value * 256 + geishaMap[29]->pana_value) +  // heizen
    (geishaMap[32]->pana_value * 256 + geishaMap[31]->pana_value) +    // kühlen
    (geishaMap[34]->pana_value * 256 + geishaMap[33]->pana_value);    // tank
 return sb; 
 
}
