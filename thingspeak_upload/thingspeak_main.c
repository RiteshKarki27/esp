#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <WiFi.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

#include "driver/i2c.h"
#include "bmp180/bmp180_main.c"
#include "dht11/dht11_main.c"



const char* host = "api.thingspeak.com"; 
const int httpPort = 80;
const String channelID   = ""; 
const String writeApiKey = "";
const String readApiKey = "";
int field1 = 0;
const float BETA = 3950;
int numberOfResults = 3; 
int fieldNumber = 1; 
float celsius=20000.1; 

static void setup_wifi()  
{
  const char *password="", *ssid="";  
  WiFi.begin(ssid,password);
  while(WiFi.status() != WL_CONNECTED) 
  {
    Serial.println("Connecting");
    delay(1000);
  }
  Serial.println("Connected!!!");
  Serial.println("Ip Address: ");
  Serial.print(WiFi.localIP());
}
void setup() 
{
  Serial.begin(115200);
  setup_wifi();
  analogReadResolution(12);  
}
static void send_to_cloud(WiFiClient *client)  
{
  unsigned long time_passed = millis();

  while(client->available() == 0) 
  {
    if(millis() - time_passed >= 5000)  
    {
      Serial.println("Time out\n");
      client->stop();
      return;
    }
  }
  while(client->available()) 
  {
    String string_t=client->readStringUntil('\r');
    Serial.println(string_t);
  }
}
void thingspeak_main_task() 
{
  WiFiClient client; 
  int analogValue=analogRead(34); 
  delay(10);        
  celsius = 1 / (log(1 / (4096. / analogValue - 1)) / BETA + 1.0 / 298.15) - 273.15;
  Serial.print("Temperature: ");
  Serial.print(celsius);
  Serial.println(" degree");
  
  String footer = String(" HTTP/1.1\r\n") + "Host: " + String(host) + "\r\n" + "Connection: close\r\n\r\n";

  if (!client.connect(host, httpPort))
  {
    return;
  }
  client.print("GET /update?api_key=" + writeApiKey + "&field1=" + celsius + footer);
  send_to_cloud(&client);

  String readRequest = "GET /channels/" + channelID + "/fields/" + fieldNumber + ".json?results=" + numberOfResults + " HTTP/1.1\r\n" +
                       "Host: " + host + "\r\n" +
                       "Connection: close\r\n\r\n";

  if (!client.connect(host, httpPort))
  {
    return;
  }
  
  client.print(readRequest);
  send_to_cloud(&client);
  delay(60000); 
}

void app_main() {
	xTaskCreate(thingspeak_main_task, "thingspeak_main_task task", 2048, NULL,
			tskIDLE_PRIORITY, NULL);
}
