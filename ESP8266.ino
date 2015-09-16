char OK[] = "OK";
char GOTIP[] = "GOT IP";

void espON() {
  pinMode(ESP_CHPD, OUTPUT);
  digitalWrite(ESP_CHPD, HIGH);
}

void espOFF() {
  pinMode(ESP_CHPD, OUTPUT);
  digitalWrite(ESP_CHPD, LOW);
}

void espToggle() {
  espOFF();
  delay(1000);
  espON();
}

boolean serialFind(char* keyword, boolean trace = false, int timeout = 2000) { 
  unsigned long deadline = millis() + timeout;
  while(millis() < deadline) {
    if (esp.available()) {
      char ch = esp.read();
      if (trace && (ch > 19 || ch == 10 || ch == 13) && ch < 128) Serial.write(ch);
      if (ch == *keyword && !*(++keyword)) return true;
    }
  }
  return false;  // Timed out
} 

boolean checkBaudRate(long b) {
  Serial << endl << F("Checking baud rate: ") << b << endl;
  esp.begin(b);
  espToggle();
  return serialFind("ready", true, 6000);
}

boolean espFixBaudRate() {
  Serial << F("Fixing ESP Baudrate") << endl;
  if (checkBaudRate(9600)) return true;
  if (checkBaudRate(115200L) || checkBaudRate(74880L)) {
    esp << F("AT+UART_DEF=9600,8,1,0,0") << endl;
    serialFind(OK, true, 6000);
    espToggle();
    return checkBaudRate(9600);
  }
}

int setESPWifiPass(const char *ssid, const char *pass) {
  Serial << "len ssid:" << strlen(ssid) << " len psass:" << strlen(pass) << endl;
  esp << F("AT+CWMODE_DEF=1") << endl;
  if (!serialFind(OK)) return -1;
  esp.flush();
  esp << "AT" << endl;
  serialFind(OK, true, 1000);
  Serial <<"will send:" << F("AT+CWJAP_DEF=") << F("\"") << ssid << F("\"") << F(",") << F("\"") << pass << F("\"") << endl;
 esp << F("AT+CWJAP_DEF=") << F("\"") << ssid << F("\"") << F(",") << F("\"") << pass << F("\"") << endl;
//  esp << F("AT+CWJAP_DEF=") << F("\"") << "vladiHome" << F("\"") << F(",") << F("\"") << "0888414447" << F("\"") << endl;
  if (!serialFind(GOTIP, true, 20000)) return -2;
  esp << F("AT+CWAUTOCONN=1") << endl;
  return 1;  
}

void startSerialProxy() {
  Serial << F("ESP Proxy") << endl;
  espToggle();
    for (;;) {
      if (esp.available())    Serial.write(esp.read());
      if (Serial.available()) esp.write(Serial.read());
    }
}

#define TS_IP F("184.106.153.149")
#define TS_GET F("GET /update?key=")
#define TS_GET_LEN 16
#define TS_FIELD F("&field1=")
#define TS_FIELD_LEN 8

int sendToThingSpeak(char* tsKey, int value) {
  Serial << endl;
  esp << F("AT+CIPSTART=\"TCP\",\"184.106.153.149\",80") << endl;
  if (!serialFind(OK, true, 4000)) return -1;
  
  int len = TS_GET_LEN + strlen(tsKey) + TS_FIELD_LEN + String(value).length() + 2;
  esp << F("AT+CIPSEND=") << len << endl;
  if (!serialFind(">", true, 6000)) return -2;
  
  esp << TS_GET << tsKey << TS_FIELD << value << "\r\n";
  if (!serialFind(OK, true, 6000)) return -3;
  
  return 1;
}

void serialProxy() {
  Serial << "Initializing Serial Proxy" << endl;
  delay(5000);
  espFixBaudRate();

  esp.begin(9600);
  espToggle();
  Serial << serialFind("ready", true, 3000) << endl;
  if (!serialFind("GOT IP", true, 30000)) {
    Serial << "no wifi" << endl;
    
  } else {
    Serial << sendToThingSpeak("7CY21ASLQUK6RT8D", 12) << endl;
  }

  for (;;) {
    if (esp.available())    Serial.write(esp.read());
    if (Serial.available()) esp.write(Serial.read());
  }
}

