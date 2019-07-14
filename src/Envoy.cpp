#include "Envoy.h"

static TaskHandle_t autoUpdateTask = NULL;
static Envoy *self = NULL; //self reference used by task to get at the class.
static TickType_t updateDelay = portTICK_PERIOD_MS * 2000; //default is 2 seconds

void task_EnvoyAutoUpdate( void *pvParameters )
{
  while (1)
  {
    vTaskDelay(updateDelay);
    self->update();
  }
}

Envoy::Envoy(WiFiClient *ptr)
{
    client = ptr;
    self = this;
}

void Envoy::begin(char *password)
{   
    strncpy(installerPassword, password, 18);

    if (!MDNS.begin("esp32")) { //you do need to call begin and it does need a name. No .local here either!
        Serial.println("Error setting up MDNS responder!");
    }
    for (int i = 0; i < 4; i++) 
    {
        envoyIP = MDNS.queryHost(envoyName, 1000); //wait up to 1 second for a reply
        if (envoyIP > 0) break; //no need to keep trying if it worked.
        Serial.println("Failed to get IP of Envoy!");
    }
    Serial.print("IP Address of Envoy: ");
    Serial.println(envoyIP.toString());
    
    serverString = "http://" + envoyIP.toString();
    //serverString = "http://www.kkmfg.com/jack"; //for testing

    xTaskCreate(&task_EnvoyAutoUpdate, "ENVOY_UPDATE", 3072, this, 10, &autoUpdateTask);
    vTaskSuspend(autoUpdateTask); //by default it should be off

    //call all three immediately to populate all the variables
    update();
}

void Envoy::setDebugging(bool state)
{
    debug = state;  
}

void Envoy::update()
{
    getInventory();
    getMeterStream();
    getProduction();
}

void Envoy::setAutoUpdateInterval(uint32_t mill)
{
  if (mill == 0) vTaskSuspend(autoUpdateTask);
  else
  {
    updateDelay = portTICK_PERIOD_MS * mill;
    vTaskResume(autoUpdateTask);
  }
  
}

void Envoy::getInventory()
{
  const char *uri = "/api/v1/production/inverters"; 
  const char *keys[] = {"WWW-Authenticate"};
  //it looks like one entry in the inventory array will take about 100 bytes and we could have 200 of them so 20k
  DynamicJsonDocument doc(25000); //a little bigger for safety. The ESP32 has a good amount of RAM anyway
  char * rendered;
  String payload;  
   
   if (debug) Serial.print("\nhttp://envoy.local/api/v1/production/inverters\n");
   http.begin(*client, serverString + String(uri));
   http.collectHeaders(keys, 1);
   int httpCode = http.GET(); //First GET
   if (httpCode > 0) 
      {
        String authReq = http.header("WWW-Authenticate");
        String authorization = getDigestAuth(authReq, String(username), String(installerPassword), String(uri), 1);
        http.end();  //Two step process. Get authorization, then reopen and send authorization
        http.begin(*client, serverString + String(uri));
        http.addHeader("Authorization", authorization);
        
        int httpCode = http.GET(); //Second GET Actually gets the resulting payload
        if (httpCode > 0) 
          {
            payload = http.getString();
             
          }
        else Serial.printf("[HTTP] Second GET... failed, error: %s\n", http.errorToString(httpCode).c_str());           
      } 
      else Serial.printf("[HTTP] First GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      http.end();
      
      deserializeJson(doc, payload);
      JsonArray obj = doc.as<JsonArray>();

      int i;
      int devType;
      numInverters = 0;
      TotalPower = 0;
      TotalMaxPower = 0;
          
      for (i = 0; i < 200; i++) 
      {
          devType = obj.getElement(i)["devType"];
          if (devType > 0)
          {
              numInverters++;
              serialNumber[i] = obj[i]["serialNumber"].as<String>();
              Watts[i] = obj[i]["lastReportWatts"];
              MaxWatts[i] = obj[i]["maxReportWatts"];
              TotalPower += Watts[i];
              TotalMaxPower += MaxWatts[i];
          }
          else break;
      }
      
      if (debug)
      {
        Serial.print("\nNumber of inverters located:");
        Serial.println(i);
        numInverters = i;

        Serial.print("\nTotal Power:");
        Serial.print(TotalPower);
        Serial.print("   Total Peak Power:");
        Serial.println(TotalMaxPower);
          
        for (int j = 0; j < numInverters; j++)
        {
            Serial.print("\nSerial Number: ");
            Serial.print(serialNumber[j]);
            Serial.print("   Output:");
            Serial.print(Watts[j]);
            Serial.print("w   Peak Output:");
            Serial.print(MaxWatts[j]);
        }
      }
}//End getEnvoyInventory

void Envoy::getMeterStream()
{

   const char *uri = "/stream/meter";
  
   const char *keys[] = {"WWW-Authenticate"};
   String payload;
   DynamicJsonDocument doc(8192); //probably a bit large but this is dynamic and will be automatically freed at the end of the function call
    
   if (debug) Serial.print("\nhttp://envoy.local/stream/meter\n");
      
   http.begin(*client, serverString + String(uri));
   http.collectHeaders(keys, 1);
   int httpCode = http.GET(); //First GET
   if (httpCode > 0) 
      {
        String authReq = http.header("WWW-Authenticate");
        String authorization = getDigestAuth(authReq, String(username), String(installerPassword), String(uri), 1);
        http.end();  //Two step process. Get authorization, then reopen and send authorization
        http.begin(*client, serverString + String(uri));
        http.addHeader("Authorization", authorization);
        
        int httpCode = http.GET(); //Second GET Actually gets the resulting payload
        if (httpCode > 0) 
          {
            payload = http.getString(); 
          }
        else Serial.printf("[HTTP] Second GET... failed, error: %s\n", http.errorToString(httpCode).c_str());           
      } 
      else Serial.printf("[HTTP] First GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      http.end();
      
      deserializeJson(doc, &payload.c_str()[5]);
      JsonObject obj = doc.as<JsonObject>();
      
      if (obj.containsKey("production"))
      {
          if (obj["production"].containsKey("ph-a"))
          {
              Apower = obj["production"]["ph-a"]["p"];    
              Avoltage = obj["production"]["ph-a"]["v"];
              Acurrent = obj["production"]["ph-a"]["i"];
              Apf = obj["production"]["ph-a"]["pf"];
              Afreq = obj["production"]["ph-a"]["f"];
              if (debug) 
              {
                  Serial.print("\nPhase A - ");
                  Serial.print("Power: ");
                  Serial.print(Apower);
                  Serial.print("   Voltage: ");
                  Serial.print(Avoltage);
                  Serial.print("   Current: ");
                  Serial.print(Acurrent);  
                  Serial.print("  Power Factor: ");
                  Serial.print(Apf);  
                  Serial.print("   Frequency: ");
                  Serial.println(Afreq);
              }
          }
          if (obj["production"].containsKey("ph-b"))
          {
              Bpower = obj["production"]["ph-b"]["p"];
              Bvoltage = obj["production"]["ph-b"]["v"];
              Bcurrent = obj["production"]["ph-b"]["i"];
              Bpf = obj["production"]["ph-b"]["pf"];
              Bfreq = obj["production"]["ph-b"]["f"];
              if (debug)
              {
                  Serial.print("\nPhase B - ");
                  Serial.print("Power: ");
                  Serial.print(Apower);
                  Serial.print("   Voltage: ");
                  Serial.print(Avoltage);
                  Serial.print("   Current: ");
                  Serial.print(Acurrent);  
                  Serial.print("  Power Factor: ");
                  Serial.print(Apf);  
                  Serial.print("   Frequency: ");
                  Serial.println(Afreq);
              }
          }
          if (debug)
          {
              Serial.print("  Power Produced:");
              Serial.print(Apower+Bpower);
              Serial.print("  Total Voltage:");
              Serial.print(Avoltage+Bvoltage);
              Serial.print("  Current:");
              Serial.print((Acurrent+Bcurrent)/2);
              Serial.print("  PowerFactor:");
              Serial.print(Apf);
              Serial.print("  Frequency:");
              Serial.print(Afreq);
              Serial.println();
          }
      }
      
      if (obj.containsKey("total-consumption"))
      {
          CApower = obj["total-consumption"]["ph-a"]["p"];
          CBpower = obj["total-consumption"]["ph-b"]["p"];
          CApower *= -1;
          CBpower *= -1;
          if (debug)
          {
              Serial.print("  Power Consumed:");
              Serial.println(CApower+CBpower);
              Serial.print("       Net Power:");
              Serial.println((Apower+Bpower)+(CApower+CBpower));
              Serial.println();                      
          }
      }
} 

//http://envoy.local/production.json
//{"production":[{"type":"inverters","activeCount":40,"readingTime":1562438278,"wNow":2934,"whLifetime":6743986},{"type":"eim","activeCount":1,"measurementType":"production","readingTime":1562438461,"wNow":1549.89,"whLifetime":4432077.5,"varhLeadLifetime":30.943,"varhLagLifetime":1895636.327,"vahLifetime":5737287.699,"rmsCurrent":15.422,"rmsVoltage":244.079,"reactPwr":801.044,"apprntPwr":1883.676,"pwrFactor":0.81,"whToday":17777.5,"whLastSevenDays":321114.5,"vahToday":25755.699,"varhLeadToday":0.943,"varhLagToday":10950.327}],"consumption":[{"type":"eim","activeCount":1,"measurementType":"total-consumption","readingTime":1562438461,"wNow":533.97,"whLifetime":930461.843,"varhLeadLifetime":2103240.489,"varhLagLifetime":1923105.934,"vahLifetime":5570362.453,"rmsCurrent":3.866,"rmsVoltage":244.104,"reactPwr":-1449.973,"apprntPwr":943.708,"pwrFactor":0.57,"whToday":2296.843,"whLastSevenDays":52319.843,"vahToday":25867.453,"varhLeadToday":11858.489,"varhLagToday":10953.934},{"type":"eim","activeCount":1,"measurementType":"net-consumption","readingTime":1562438461,"wNow":-1015.921,"whLifetime":347764.828,"varhLeadLifetime":2103209.546,"varhLagLifetime":27469.607,"vahLifetime":5570362.453,"rmsCurrent":11.556,"rmsVoltage":244.13,"reactPwr":-648.929,"apprntPwr":1416.057,"pwrFactor":-0.7,"whToday":0,"whLastSevenDays":0,"vahToday":0,"varhLeadToday":0,"varhLagToday":0}],"storage":[{"type":"acb","activeCount":0,"readingTime":0,"wNow":0,"whNow":0,"state":"idle"}]}
void Envoy::getProduction()
{
    const char *uri = "/production.json";
    char * rendered;
    const char *keys[] = {"WWW-Authenticate"};
    String payload;
    DynamicJsonDocument doc(8192); //probably a bit large but this is dynamic and will be automatically freed at the end of the function call

    if (debug) Serial.print("\nhttp://envoy.local/production.json");               
    http.begin(*client, serverString + String(uri));
    http.collectHeaders(keys, 1);
    int httpCode = http.GET(); //First GET
    if (httpCode > 0) 
    {
        payload = http.getString(); 
    //    Serial.println(&payload.c_str()[0]);
    }
    else Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());           
    http.end();
    
    deserializeJson(doc, payload);
    JsonObject obj = doc.as<JsonObject>();

    if (obj.containsKey("production"))
    {
        JsonObject array0 = obj["production"][0];
        JsonObject array1 = obj["production"][1];
        //each entry in the array has a type element that describes what it is. At the moment
        //we kind of assume that they're in the same order (and they always should be) but check anyway
        if (array0["type"] == "inverters")
        {
            //all of the elements are children of the array entry so we reference from there
            int InvCount = array0["activeCount"];
            if (debug) 
            {
                Serial.print("Number of inverters:");
                Serial.print(InvCount);
            }
        }
        if (array1["type"] == "eim")
        {
            ProdPower = array1["wNow"];     
            rmsCurrent = array1["rmsCurrent"];
            rmsCurrent/=2;   
            rmsVoltage = array1["rmsVoltage"];                 
            kWhToday = array1["whToday"];   
            kWhToday/=1000;   
            kWhLastSevenDays = array1["whLastSevenDays"];   
            kWhLastSevenDays/=1000;
            MWhLifetime = array1["whLifetime"];   
            MWhLifetime/=1000000;
            if (debug)
            {
                Serial.print("   Production Power:");
                Serial.print(ProdPower);   
                Serial.print("   Current:");
                Serial.print(rmsCurrent); 
                Serial.print("   Voltage:");
                Serial.print(rmsVoltage);   
                Serial.print("\n                         kWh Today:");
                Serial.print(kWhToday);   
                Serial.print("   kWh LastSevenDays:");
                Serial.print(kWhLastSevenDays);  
                Serial.print("   MWh Lifetime:");
                Serial.println(MWhLifetime);
            }
        }
    }

    if (obj.containsKey("consumption"))
    {
        JsonObject array2 = obj["consumption"][0];
        JsonObject array3 = obj["consumption"][1];
        
        ConPower = array2["wNow"];
        ConPower*=-1;   
        ConrmsCurrent = array2["rmsCurrent"];
        ConrmsCurrent/=-2;
        ConrmsVoltage = array2["rmsVoltage"];   
        ConkWhToday = array2["whToday"];   
        ConkWhToday/=-1000;
        ConkWhLastSevenDays = array2["whLastSevenDays"];   
        ConkWhLastSevenDays/=-1000;
        ConMWhLifetime = array2["whLifetime"];   
        ConMWhLifetime/=-1000000;
        NetConPower = array3["wNow"];   
        NetConPower*=-1;
        if (debug)
        {
            Serial.print("\n                         Consumption Power:");
            Serial.print(ConPower);   
            Serial.print("   Current:");
            Serial.print(ConrmsCurrent); 
            Serial.print("   Voltage:");
            Serial.print(ConrmsVoltage);   
            Serial.print("\n                         kWh Today:");
            Serial.print(ConkWhToday);   
            Serial.print("   kWh LastSevenDays:");
            Serial.print(ConkWhLastSevenDays);  
            Serial.print("   MWh Lifetime:");
            Serial.println(ConMWhLifetime); 
            Serial.print("\n                         Net Power:");
            Serial.print(NetConPower);
            Serial.print("  kWhToday:");
            Serial.print(kWhToday+ConkWhToday); 
            Serial.print("  kWh Last 7 Days:");
            Serial.print(kWhLastSevenDays+ConkWhLastSevenDays);   
            Serial.print("  MWh Lifetime:");
            Serial.print(MWhLifetime+ConMWhLifetime);                   
        }
    }             
   
} //END production.json

void Envoy::printInventory()
{
    for (int j = 0; j < numInverters; j++)
    {
        Serial.println();
        Serial.print(j + 1);
        Serial.print(" - ");
        Serial.print("Serial Number: ");
        Serial.print(serialNumber[j]);
        Serial.print("   Output:");
        Serial.print(Watts[j]);
        Serial.print("w   Peak Output:");
        Serial.print(MaxWatts[j]);
    }
}

int Envoy::inverterWatts(String serialnumber)
{
    for (int j = 0; j < numInverters; j++)
    {
        if (serialNumber[j] == serialnumber)
        {
            return Watts[j];
        }
    }
    return 0;
}

int Envoy::inverterWatts(int index)
{
  if (index < 0 || index >= numInverters) return 0;
  return Watts[index];
}

int Envoy::inverterMaxWatts(String serialnumber)
{
    for (int j = 0; j < numInverters; j++)
    {
        if (serialNumber[j] == serialnumber)
        {
            return MaxWatts[j];
        }
    }
    return 0;
}

int Envoy::inverterMaxWatts(int index)
{
  if (index < 0 || index >= numInverters) return 0;
  return MaxWatts[index];
}

String Envoy::SerialNumber(int index)
{
  if (index < 0 || index >= numInverters) return String();
  return serialNumber[index];
}

//A set of routines for generating MD5 auth for a webserver. Does this belong in the Envoy class? No, it does not.
//But, here it is... How can that be explained? Laziness I suppose. It will be moved eventually to it's own area... somewhere...
//somewhere it belongs... I don't yet know where.

String Envoy::extractParam(String& authReq, const String& param, const char delimit) {
  int _begin = authReq.indexOf(param);
  if (_begin == -1) {
    return "";
  }
  return authReq.substring(_begin + param.length(), authReq.indexOf(delimit, _begin + param.length()));
}

//It should be noted that nothing seems to have called srand and besides, the C psuedorandom number generator sucks.
//But, then again, we aren't really shooting for super security here. This function generates unsufficiently random
//results but we're not trying to hide solar parameters from the NSA here. So, it might not be a huge deal.
String Envoy::getCNonce(const int len) {
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  String s = "";

  for (int i = 0; i < len; ++i) {
    s += alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  return s;
}

String Envoy::getDigestAuth(String& authReq, const String& username, const String& password, const String& uri, unsigned int counter) {
  // extracting required parameters for RFC 2069 simpler Digest
  String realm = extractParam(authReq, "realm=\"", '"');
  String nonce = extractParam(authReq, "nonce=\"", '"');
  String cNonce = getCNonce(8);
  if(debug)Serial.println(realm);
  if(debug)Serial.println(nonce);
  if(debug)Serial.println(cNonce);
  
  char nc[9];
  snprintf(nc, sizeof(nc), "%08x", counter);

  // parameters for the RFC 2617 newer Digest
  MD5Builder md5;
  md5.begin();
  md5.add(username + ":" + realm + ":" + password);  // md5 of the user:realm:user
  md5.calculate();
  String h1 = md5.toString();

  md5.begin();
  md5.add(String("GET:") + uri);
  md5.calculate();
  String h2 = md5.toString();

  md5.begin();
  md5.add(h1 + ":" + nonce + ":" + String(nc) + ":" + cNonce + ":" + "auth" + ":" + h2);
  md5.calculate();
  String response = md5.toString();

  String authorization = "Digest username=\"" + username + "\", realm=\"" + realm + "\", nonce=\"" + nonce +
                         "\", uri=\"" + uri + "\", algorithm=\"MD5\", qop=auth, nc=" + String(nc) + ", cnonce=\"" + cNonce + "\", response=\"" + response + "\"";
  if (debug) Serial.println(authorization);

  return authorization;
}

