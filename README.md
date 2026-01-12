# OBDSnifferWebserver<br>
ESP32 canbus sniffer with web interface<br>
<br>
This arduino project is a single value canbus monitor with a web interface.<br>
The web interface is used to configure the wifi and then continue with the ESP in the home network.<br>
In the configuration page a visualisation can be selected: number, bar, pie (gauge) or graph.<br>
and the CANbus code, nr of bytes, endian (little/big), multiplication factor, range and max value.<br>
For each of the visualisations, there is a test page and a monitor page that shows the live data.<br>
<br>
The idea is that you configure what you want to monitor and then reset the wifi to have the ESP work as an AP station in your car.<br>
The update of the canbus value is not implemented yet.
