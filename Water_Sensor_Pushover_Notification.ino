
#include <SPI.h>
#include <Ethernet.h>
#include "pushover.h"

byte mac[] = { 
  0xDE,0xAC,0xBF,0xEF,0xFE,0xAB};
IPAddress ip(192,168,1,11);
byte gateway[] = { 
  192, 168, 1, 1 };
byte subnet[] = { 
  255, 255, 255, 0 };

const int SENSOR_PIN = 2;
const int GREEN_LED_PIN = 7;
const int RED_LED_PIN = 8;

const unsigned long SENSOR_READ_INTERVAL_IN_MILLIS = 10ul; 

// Pushover settings
//const char apitoken[] = "your api key defined in ./pushover.h";
//const char userkey [] = "your user key defined in ./pushover.h";
const char pushoversite[] = "api.pushover.net";
const int NORMAL_PRIORITY = 0;
const int HIGH_PRIORITY = 1;
const unsigned long REPUSH_INTERVALL_IN_MILLIS = 1800000ul; // 30 min
unsigned long repushIntervalCounter = 0ul;

EthernetServer server = EthernetServer(80);

void setup()
{
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(GREEN_LED_PIN, OUTPUT); 
  pinMode(RED_LED_PIN, OUTPUT); 

  //  Serial.begin(9600);

  Ethernet.begin(mac, ip);
  delay(1000);
  server.begin();

  digitalWrite(GREEN_LED_PIN, HIGH);  
}

void loop()
{
  delay(SENSOR_READ_INTERVAL_IN_MILLIS);

  int water = !digitalRead(SENSOR_PIN);

  digitalWrite(RED_LED_PIN, water);

  handlePushoverNotification(water);

  handleHttpClient(water);
}

void handleHttpClient(int water) {
  EthernetClient client = server.available();
  if (client) {
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // http request has ended, send a reply
        if (c == '\n' && currentLineIsBlank) {
          printResponse(client, water);
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    // give the http client time to receive the data
    delay(1);
    client.stop();
  }
}

void printResponse(EthernetClient& client,int water) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json; charset=ISO-8859-1");
  client.println("Connection: close");  // the connection will be closed after completion of the response
  client.println();

  client.print("{");
  printJson(client,"waterAlarm", String(water), 1);
  printJson(client,"nextPushoverAlarmInSeconds", String(repushIntervalCounter/1000ul), 1);
  printJson(client, "uptimeInSeconds", String(millis()/1000ul), 0);
  client.println("}");
}

void printJson(EthernetClient& client, String key, String value, int comma) {
  client.print("\"");  
  client.print(key);
  client.print("\":\"");
  client.print(value);
  client.print("\"" + String(comma == 0 ? "" : ","));
}

void handlePushoverNotification(int water) {

  if (water && repushIntervalCounter == 0ul) {
    // alarm
    sendPushoverMessage("Auffangbecken ist voll. Bitte leeren!", HIGH_PRIORITY);  
    pushoverVisualization();    
    repushIntervalCounter = REPUSH_INTERVALL_IN_MILLIS;
  }

  if (!water && repushIntervalCounter > 0ul) {
    // all-clear
    repushIntervalCounter = 0ul;
    sendPushoverMessage("Auffangbecken ist wieder leer.", NORMAL_PRIORITY);  
    pushoverVisualization();
  }

  repushIntervalCounter= repushIntervalCounter > 0ul ? repushIntervalCounter - SENSOR_READ_INTERVAL_IN_MILLIS : 0ul;
}

void pushoverVisualization() {
  for (int i =0; i < 3; i++) {
    digitalWrite(GREEN_LED_PIN, LOW);
    delay(150); 
    digitalWrite(GREEN_LED_PIN, HIGH);
    delay(150); 
  }
}

byte sendPushoverMessage(char *pushovermessage, int priority)
{
  String message = pushovermessage;

  int length = 92 + message.length();

  EthernetClient httpClient;

  if(httpClient.connect(pushoversite,80))
  {
    httpClient.println("POST /1/messages.json HTTP/1.1");
    httpClient.println("Host: api.pushover.net");
    httpClient.println("Connection: close\r\nContent-Type: application/x-www-form-urlencoded");
    httpClient.print("Content-Length: ");
    httpClient.print(length);
    httpClient.println("\r\n");

    httpClient.print("token=");
    httpClient.print(apitoken);
    httpClient.print("&user=");
    httpClient.print(userkey);

    httpClient.print("&priority=");
    httpClient.print(priority);
    httpClient.print("&message=");
    httpClient.print(message);
    while(httpClient.connected())  
    {
      while(httpClient.available())
      {
        char ch = httpClient.read();
        //        Serial.write(ch);
      }
    }
    httpClient.stop();
  }  
}























