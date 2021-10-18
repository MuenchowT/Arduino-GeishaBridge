#if defined(ESP8266)
// Beschriftung : Nummer im Code
// https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
#define D1 (5)
#define D2 (4)
#define D3 (0)    // FLASH
#define D4 (2)    // nur output  LED
#define D5 (14)
#define D6 (12)
#define D7 (13)
#define D8 (15)   // hm, hm
#define RX (3)    // USB
#define TX (1)    // USB
#define D9 (11)
#define D10 (12)
#define A0  (ADC0)  // Analog
#endif

//#define DEBUG           1
#define DEBUG_TX           1
#define MAGIC_PACKET    (17)

const int rxPin = D5;  // 14
const int txPin = D6;  // 12
const int rtsPin D2;   // 4
const int windPin D1;  // 5

const int relays[2] = { D4, D7 };
//const int relays[4] = { D4, D7, D9, D10 };


#define MQTT_BIT   (0x01)
#define GEISHA_BIT (0x02)
#define SETTABLE_BIT  (0x04)
#define INT_BIT     (0x08)
#define STRING_BIT   (0x10)
#define FLOAT_BIT   (0x20)
#define DIRTY_BIT   (0x40)
#define MANUAL_BIT  (0x80)

typedef struct {        // We love Bitmaps :-)
const int     pana_register;
const char *  pana_name;
uint8_t       pana_value;      // value coming from pana/sent to pana
String        pana_value_string;
int           mqtt_value;     // value sent to from MQTT
uint8_t       pana_map;       // 0x01=mqtt-propery, 0x02=geisha-param, 0x04 = settable -- datatypes: 0x08: int, 0x10: String, 0x20: float
} geishaStruct;             // b00000001        , b00000010        , b00000100                     b00001000, b0001 0000   , b0010 0000


geishaStruct geishaParams[] = {
{ 0,  "N/A",                  0, String(""), 0, (0)},  // Dummy
{ 1,  "Betriebsart",          0, String(""), 0, (MQTT_BIT               | SETTABLE_BIT | STRING_BIT | MANUAL_BIT)}, 
{ 2,  "Power",                0, String(""), 0, (MQTT_BIT               | SETTABLE_BIT | STRING_BIT | MANUAL_BIT)}, 
{ 3,  "Eco",                  0, String(""), 0, (MQTT_BIT               | SETTABLE_BIT | STRING_BIT | MANUAL_BIT)}, 
{ 4,  "AktStromverbrauch",    0, String(""), 0, (MQTT_BIT                              | INT_BIT | MANUAL_BIT)}, 
{ 17, "Abtauen",              0, String(""), 0, (MQTT_BIT | GEISHA_BIT | STRING_BIT)},  
{ 18, "Aussentemperatur",     0, String(""), 0, (MQTT_BIT | GEISHA_BIT | INT_BIT)},  
{ 19, "VL",                   0, String(""), 0, (MQTT_BIT | GEISHA_BIT | INT_BIT)},
{ 20, "Fehlercode",           0, String(""), 0, (MQTT_BIT | GEISHA_BIT | INT_BIT)},  
{ 21, "RL",                   0, String(""), 0, (MQTT_BIT | GEISHA_BIT | INT_BIT)},
{ 22, "WW_Temperatur",        0, String(""), 0, (MQTT_BIT | GEISHA_BIT | INT_BIT)},
{ 23, "Kompressor",		      0, String(""), 0, (MQTT_BIT | GEISHA_BIT | INT_BIT)},
{ 24, "letzterFehlercode",    0, String(""), 0, (MQTT_BIT | GEISHA_BIT | INT_BIT)},
{ 27, "Betriebsart",          0, String(""), 0, (           GEISHA_BIT | INT_BIT )},  
{ 144,"Betriebsart",          0, String(""), 0, (           GEISHA_BIT | INT_BIT )}, 
{ 29, "Energie Heizen LB",    0, String(""), 0, (           GEISHA_BIT | INT_BIT)},  
{ 30, "EnergieHeizen",       0, String(""), 0, (            GEISHA_BIT | INT_BIT)},  
{ 31, "Energie Kuehlen LB",   0, String(""), 0, (           GEISHA_BIT | INT_BIT)},  
{ 32, "EnergieKuehlen",      0, String(""), 0, (            GEISHA_BIT | INT_BIT)},
{ 33, "Energie WW LB",        0, String(""), 0, (           GEISHA_BIT | INT_BIT)},  
{ 34, "EnergieWW",           0, String(""), 0, (            GEISHA_BIT | INT_BIT)},
{ 35, "Pumpenstufe",          0, String(""), 0, (MQTT_BIT | GEISHA_BIT | SETTABLE_BIT | INT_BIT)},
{ 129, "Error-Reset",         0, String(""), 0, (MQTT_BIT | GEISHA_BIT | SETTABLE_BIT | INT_BIT)},
{ 130, "Heizkurve-AT-niedrig", 0, String(""), 0, (MQTT_BIT | GEISHA_BIT | SETTABLE_BIT | INT_BIT)},
{ 131, "Heizkurve-AT-hoch",   0, String(""), 0, (MQTT_BIT | GEISHA_BIT | SETTABLE_BIT | INT_BIT)},
{ 132, "Heizkurve-VL-niedrig", 0, String(""), 0, (MQTT_BIT | GEISHA_BIT | SETTABLE_BIT | INT_BIT)},
{ 133, "Heizkurve-VL-hoch",   0, String(""), 0, (MQTT_BIT | GEISHA_BIT | SETTABLE_BIT | INT_BIT)},
{ 134, "Heizung-aus-bei-AT",  0, String(""), 0, (MQTT_BIT | GEISHA_BIT | SETTABLE_BIT | INT_BIT)},
{ 136, "Cool_Solltemperatur", 0, String(""), 0, (MQTT_BIT | GEISHA_BIT | SETTABLE_BIT | INT_BIT)},
{ 137, "Tank_Solltemperatur", 0, String(""), 0, (MQTT_BIT | GEISHA_BIT | SETTABLE_BIT | INT_BIT)},
{ 138, "Sollwertverschiebung", 0, String(""), 0, (MQTT_BIT | GEISHA_BIT | SETTABLE_BIT | INT_BIT)}
};

const int MAX_PARAM_NUMBER  = 144;
const int PARAMS_ARRAY_SIZE = (sizeof(geishaParams)/sizeof(geishaParams[0]));

#define ON_BIT (0x01)
#define ECO_BIT (0x040)
#define MAX_DIRTY_MAP 6

/*
129 = Reset Error(Taste an der FB)
130 Einstellen der niedrigen Außentemperatur
131 Einstellen der hohen Außentemperatur
132 Einstellen der Wasseraustrittstemperatur bei niedriger Außentemperatur
133 Einstellen der Wasseraustrittstemperatur bei hoher
134 Einstellen der Außentemperatur, bei der der Heizbetrieb in der Heizbetriebsart abgeschaltet wird 170 134 25 73 85 134 25 244 25=25 Grad
135 Einsteller der Außentemperatur, bei der die Elektrozusatzheizung eingeschaltet wird
136 Cool Set
138 Sollwertverschiebung
137 Heißwasserspeichertemperatur 170 137 50 101 85 137 50 16
Heißwasser(Tanktemperatur) auf 50 eingestellt
138 =1/2/3/4/5/0 Datensatz 170 138 2 54 85 138 2 225 das ist die Parallelverschiebung der Heizkurve ,in dem Fall 2
*/

  /*
   register
   17   value & 64 = abtauen

   * 
   */
