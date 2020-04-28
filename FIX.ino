#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <LoRa.h>
#include <LiquidCrystal_I2C.h>

int sensorGas= A0; // pin out MQ-2 pada LoRa mini dev
int sensorApi = A1; // pin out sensor api pada LoRa mini dev
LiquidCrystal_I2C lcd(0x27, 16, 2); // address pada LCD dan jumlah kolom serta baris


static const PROGMEM u1_t NWKSKEY[16] ={ 0x98, 0x27, 0x66, 0xE1, 0x1E, 0xC7, 0x31, 0xEA, 0x4E, 0x50, 0xF2, 0xD2, 0x76, 0xF5, 0xE5, 0xC9 }; // network session key
static const u1_t PROGMEM APPSKEY[16] ={ 0xE1, 0x24, 0x71, 0x3E, 0xB3, 0x10, 0xC7, 0xC4, 0x74, 0xCB, 0xE4, 0x74, 0x0A, 0xCC, 0xB2, 0xA2 }; // application session key
static const u4_t DEVADDR = 0x260419A8 ; // device address


void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }


static osjob_t sendjob;
const unsigned TX_INTERVAL = 60; // interval pengiriman data 1menit sekali


const lmic_pinmap lmic_pins = {
    .nss = 10,                       
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,                       
    .dio = {2, 6, 7}, 
};

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
       
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
   
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
       int i;
        int sensorvalue = analogRead(sensorGas); // membaca nilai ADC dari sensor         
        float VRL= sensorvalue * 5.0 / 1024;  // mengubah nilai ADC ( 0 - 1023 ) menjadi nilai voltase ( 0 - 5.00 volt )
        float nilai_Rs = ( 5.0 * 1000 / VRL ) - 1000;
        uint32_t nilai_ppm = 100 * pow(nilai_Rs / 800,-1.53); // ppm = 100 * ((rs/ro)^-1.53);
        lcd.setCursor(4,0);
        lcd.print("LPG = " + String(nilai_ppm));
        lcd.print(" ppm");
       
        if (nilai_ppm > 100)
        {
          lcd.setCursor(0,1);
          lcd.print(" Terjadi kebocoran");
           for (i=0; i<18;i++){
           lcd.scrollDisplayLeft();
           delay(300);
           }
        }
         else
         {
          lcd.setCursor(0,1);
          lcd.print(" Tidak terjadi kebocoran");
          for (i=0; i<24;i++){
           lcd.scrollDisplayLeft();
           delay(300);
          }
         }
         delay(1000);
         lcd.clear();
    
     uint32_t kadar_api= analogRead(sensorApi);
        lcd.setCursor(4,0);
        lcd.print("Api = " + String(kadar_api));
         if (kadar_api < 200)
      {
          lcd.setCursor(0,1);
          lcd.print(" Api terdeteksi");
          for (i=0; i<54;i++){
          lcd.scrollDisplayLeft();
          delay(300);
          }
      }     else
         {
          lcd.setCursor(0,1);
          lcd.print(" Api tidak terdeteksi");
          for (i=0; i<22;i++){
          lcd.scrollDisplayLeft();
          delay(300);
          }
         }
        delay(1000);    
        lcd.clear();

        // baca kekuatan sinyal pada LoRa
        Serial.print("Kekuatan sinyal RSSI ");
        Serial.println(LoRa.packetRssi());
        
        byte payload[6];
        payload[0] = highByte(nilai_ppm);
        payload[1] = lowByte(nilai_ppm);
        payload[2] = highByte(kadar_api);
        payload[3] = lowByte(kadar_api);
        payload[4] = highByte(LoRa.packetRssi());
        payload[5] = lowByte(LoRa.packetRssi());

        LMIC_setTxData2(1, (uint8_t*)payload, sizeof(payload), 0);
         
        Serial.println(F("Packet queued"));
        Serial.print(F("Sending packet on frequency: "));
        Serial.println(LMIC.freq);
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {

    while (!Serial); // wait for Serial to be initialized
    Serial.begin(115200);
    delay(100);     // per sample code on RF_95 test
    Serial.println(F("Starting"));

    lcd.begin();
  
    os_init();
    
    LMIC_reset();

    
    #ifdef PROGMEM
    
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x13, DEVADDR, nwkskey, appskey);
    #else
    
    LMIC_setSession (0x13, DEVADDR, NWKSKEY, APPSKEY);
    #endif

     #if defined(CFG_eu868)
  
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    
    #elif defined(CFG_as923)
    Serial.println(F("Loading to AS923......."));
    LMIC_setDrTxpow(DR_SF7,2);
    for (int i=1;i<2;i++)
    {
      LMIC_disableChannel(i);   // agar frekuensi tidak berubah ubah
    }
    #endif
   
    
    LMIC_setLinkCheckMode(0);

    
    LMIC.dn2Dr = DR_SF9;

    
    LMIC_setDrTxpow(DR_SF7,2);
    
    // Start job
    do_send(&sendjob);
}

void loop() {
    unsigned long now;
    now = millis();
    if ((now & 512) != 0) {
      digitalWrite(13, HIGH);
    }
    else {
      digitalWrite(13, LOW);
    }
      
    os_runloop_once();
 }
