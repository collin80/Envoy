#ifndef _H_ENVOY_H_
#define _H_ENVOY_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "HTTPClient.h"
#include <ESPmDNS.h>
#include <MD5Builder.h>

class Envoy
{
public:
    Envoy(WiFiClient *ptr);
    void begin(char *password);
    void setDebugging(bool state);
    void getInventory();
    void getMeterStream();
    void getProduction();
    void update();
    void setAutoUpdateInterval(uint32_t mill);
    void printInventory();
    int inverterWatts(String serialnumber);
    int inverterWatts(int index);
    int inverterMaxWatts(String serialnumber);
    int inverterMaxWatts(int index);
    String SerialNumber(int index);
    
    //It's quick and easy but all these variables shouldn't be public. It's a pain to provider getters for each one but that would
    //prevent accidental modification of these variables. Or, the developer can be careful not to make a mistake. Those never happen!
    
    //stream/meter variables
    double Apower;
    double Avoltage;
    double Acurrent;
    double Apf;
    double Afreq;
    double Bpower;
    double Bvoltage;
    double Bcurrent;
    double Bpf;
    double Bfreq;
    double CApower;
    double CBpower;

    //production.json variables
    double ProdPower;
    double rmsCurrent;
    double rmsVoltage;
    double kWhToday;
    double kWhLastSevenDays;
    double MWhLifetime;
    double ConPower;
    double ConrmsCurrent;
    double ConrmsVoltage;
    double ConkWhToday;
    double ConkWhLastSevenDays;
    double ConMWhLifetime;
    double NetConPower;
    
    //inventory variables
    String serialNumber[200];
    int Watts[200];
    int MaxWatts[200];
    double TotalPower=0;
    double TotalMaxPower=0;
    int numInverters;

protected:
    String extractParam(String& authReq, const String& param, const char delimit);
    String getCNonce(const int len);
    String getDigestAuth(String& authReq, const String& username, const String& password, const String& uri, unsigned int counter);
private:
    HTTPClient http;
    IPAddress envoyIP;
    String serverString;
    WiFiClient *client;
    bool debug;
    
    char installerPassword[20];
    
    //these are hard coded as they do not change for anyone.
    const char *username = "installer";
    const char *envoyName = "envoy"; //the .local is automatically added so basically never explicitly do so anywhere.
};

#endif
