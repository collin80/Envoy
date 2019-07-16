#include <WiFi.h>
#include "lwip/inet.h"
#include "lwip/dns.h"
#include <Envoy.h>
 
const char* ssid = "YourSSID";
const char* ssidPassword = "YourPassword";

WiFiClient client;
bool debug = true;
Envoy envoy(&client);
 
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, ssidPassword);
  delay(5000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  ip_addr_t dnssrv=dns_getserver(0);
  Serial.print("DNS server: ");
  Serial.println(inet_ntoa(dnssrv));
  
  envoy.debug = true;
  envoy.begin("abcd1234"); //you need to get your installer password and paste it here. Look at Envoy.h for more details
  
  //the next call causes the library to automatically call the update function for you at the set interval in milliseconds
  //you are not required to do this. You are welcome to call envoy.update() any time you'd like or call 
  //getInventory()  getMeterStream() or getProduction()
  envoy.setAutoUpdateInterval(3000); //auto update every 3 seconds
}

void loop() 
{
  Serial.println("\f================================================================================================");
  envoy.printInventory();
  Serial.println("\n================================================================================================");

  delay(5000);
}
