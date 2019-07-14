Envoy
=======

An interface library meant to facilitate access to Enphase Envoy systems on your local WiFi network. Envoy systems traditionally
must be accessed over the standard internet based API. However, the Envoy units actually support accessing the data locally as well.
This library uses the local access to provide your programs with information from any Envoy units you have on your network. With this
library you do NOT need to register with Enphase and you do NOT need an internet connection. 

The first Envoy unit registers itself as envoy.local

The next one as envoy1.local, and so on. 

This library was meant to run on the ESP32. YMMV if you try to use it on anything else. In particular, it includes a modified
version of HTTPClient from the Arduino-ESP32 files. This was required as the Envoy will return infinite chunks when accessing
some information and thus the HTTPClient library needed to be modified to not pull down multiple chunks. Instead this version stops
after the first chunk which is proper for interface with the Envoy hardware.
