#ifndef _H_ENVOY_H_
#define _H_ENVOY_H_
/*
 * Copyright 2019 EVTV. All rights reserved. This program was witten by Jack Rickard and Collin Kidder http://evtv.me
 * 
 * Envoy is a gateway communications device created by Enphase Energy to port data from their line of IQ microinverters.
 * It communicates with the IQ microinverters using a proprietary version of the Power Line Communications PLC protocol.
 * It provides that information in TCP/IP format via your home WiFi access point to their Enlighten portal web site
 * on the external internet.
 * 
 * This-> Envoy library allows you to access the Envoy directly from an ESP32 microcontroller on the same home WiFi AP
 * using an undocumented installer http interface on the Envoy device to get JSON data.  
 * It does NOT access Enlighten or use the Enlighten API.
 * 
 * It DOES require the "installer" password which you can obtain from your system installer or Enphase by pleading
 * with them piteously for a month or so generally.
 * 
 * Or download the Android app from https://www.dropbox.com/s/xc40op8eqfrykaa/AndroidXam.AndroidXam.Signed2019.apk?dl=0
 * and enter "installer" as the login and the full serial number from the Envoy device.  IT will generate the proper
 * installer password from that data.
 * 
 * * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * 
 */

#include <WiFi.h>
#include "HTTPClient.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <MD5Builder.h>

class Envoy
{
public:
    Envoy(WiFiClient *ptr);
    void begin(char *password);
    void begin(char *password,char *device);
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
    bool debug;
    //stream/meter variables
    double Apower;
    double Avoltage;
    double Acurrent;
    double pf;
    double freq;
    double Bpower;
    double Bvoltage;
    double Bcurrent;
    double Bpf;
    double Bfreq;
    double CApower;
    double CBpower;
    double Power;
    double Consumption;
    double NetPower;

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
    double InvTotalPower=0;
    double InvTotalMaxPower=0;
    int numInverters;
    IPAddress envoyIP;
 
protected:
    String extractParam(String& authReq, const String& param, const char delimit);
    String getCNonce(const int len);
    String getDigestAuth(String& authReq, const String& username, const String& password, const String& uri, unsigned int counter);
private:
    HTTPClient http;
    String serverString;
    WiFiClient *client;
    
    char installerPassword[20];
    char envoyName[20] = "envoy"; //the .local is automatically added so basically never explicitly do so anywhere.
                                     //In multiple Envoy systems this would be envoy, envoy2, envoy3, etc.
    const char *username = "installer"; //login is always installer
  };

#endif
