/*
    Copyright (C) <2020>  <Mike Roberts>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define ESP32 //keep this define if you use ESP32 board; comment out if e.g. ESP8266 is used
#define TELNET_DEBUG

//#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//#include <DNSServer.h>
//#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <ESPTelnet.h>


#include "Ecodan.h"
#include "config.h"
#include "MQTTConfig.h"
#include "TimerCallBack.h"
#include "Debug.h"
#include "OTA.h"



ECODAN HeatPump;

SoftwareSerial SwSerial;
WiFiClient NetworkClient;
PubSubClient MQTTClient(NetworkClient);
ESPTelnet TelnetServer;
String HostName;

void HeatPumpQueryStateEngine(void);
void HeatPumpKeepAlive(void);
void Zone1Report(void);
void Zone1Report(void);
void HotWaterReport(void);
void SystemReport(void);
void TestReport(void);

TimerCallBack HeatPumpQuery1(500, HeatPumpQueryStateEngine);
TimerCallBack HeatPumpQuery2(60 *  1000, HeatPumpKeepAlive);

String MQTTCommandZone1TempSetpoint = MQTT_COMMAND_ZONE1_TEMP_SETPOINT;
String MQTTCommandZone1FlowSetpoint = MQTT_COMMAND_ZONE1_FLOW_SETPOINT;
String MQTTCommandZone1CurveSetpoint = MQTT_COMMAND_ZONE1_CURVE_SETPOINT;

String MQTTCommandHotwaterSetpoint = MQTT_COMMAND_HOTWATER_SETPOINT;

String MQTTCommandSystemHeatingMode = MQTT_COMMAND_SYSTEM_HEATINGMODE;
String MQTTCommandSystemPower = MQTT_COMMAND_SYSTEM_POWER;
String MQTTCommandSystemTemp = MQTT_COMMAND_SYSTEM_TEMP;

void setup()
{
  //DEBUGPORT.begin(DEBUGBAUD);
//  HEATPUMP_STREAM.begin(SERIAL_BAUD, SERIAL_CONFIG, D3, D4); // Rx, Tx
//  HEATPUMP_STREAM.begin(SERIAL_BAUD, SERIAL_CONFIG, 10, 9); // Rx, Tx
  HEATPUMP_STREAM.begin(SERIAL_BAUD, SERIAL_CONFIG, 3, 1); // Rx, Tx
  pinMode(3, INPUT_PULLUP);

  HeatPump.SetStream(&HEATPUMP_STREAM);

  WiFiManager MyWifiManager;

  //wifiManager.resetSettings(); //reset settings - for testing

  MyWifiManager.setTimeout(180);

  if (!MyWifiManager.autoConnect("Ecodan Bridge AP"))
  {
#ifdef ESP32
    ESP.restart();
#else
    ESP.reset();
#endif
    delay(5000);
  }
  HostName = "EcodanBridge-";
#ifdef ESP32
  HostName += String(ESP.getChipModel(), HEX);
#else
  HostName += String(ESP.getChipId(), HEX);
#endif
  WiFi.hostname(HostName);

  setupTelnet();

  OTASetup(HostName.c_str());
  MQTTClient.setServer(MQTT_SERVER, MQTT_PORT);
  MQTTClient.setCallback(MQTTonData);
}

void loop()
{
  if (!MQTTReconnect())
  {
    return;
  }
  MQTTClient.loop();
  TelnetServer.loop();

  
  HeatPumpQuery1.Process();
  HeatPumpQuery2.Process();

  HeatPump.Process();
  ArduinoOTA.handle();
}


void HeatPumpKeepAlive(void)
{
  HeatPump.KeepAlive();
  HeatPump.TriggerStatusStateMachine();
}

void HeatPumpQueryStateEngine(void)
{
  HeatPump.StatusStateMachine();
  if (HeatPump.UpdateComplete())
  {
#ifdef TELNET_DEBUG
    DEBUG_PRINTLN("Update Complete");
#endif
    if (MQTTReconnect())
    {
#ifdef TELNET_DEBUG
      DEBUG_PRINTLN("Reporting started");
#endif
      Zone1Report();
      Zone2Report();
#ifdef TELNET_DEBUG
      DEBUG_PRINTLN("Reporting zone2 done");
#endif
      HotWaterReport();
#ifdef TELNET_DEBUG
      DEBUG_PRINTLN("Reporting hw done");
#endif
      SystemReport();
#ifdef TELNET_DEBUG
      DEBUG_PRINTLN("Reporting system done");
#endif
      TestReport();
#ifdef TELNET_DEBUG
      DEBUG_PRINTLN("Reporting test report done");
#endif
    }
  }
}


uint8_t MQTTReconnect()
{
  if (MQTTClient.connected())
  {
    return 1;
  }

#ifdef TELNET_DEBUG
  DEBUG_PRINTLN("Attempting MQTT connection...");
#endif
  if (MQTTClient.connect(HostName.c_str(), MQTT_USER, MQTT_PASS, MQTT_LWT, 0, true, "offline" ))
  {
#ifdef TELNET_DEBUG
    DEBUG_PRINTLN("MQTT Connected");
#endif
    MQTTonConnect();
    return 1;
  }
  else
  {
    switch (MQTTClient.state())
    {
      case -4 :
#ifdef TELNET_DEBUG
        DEBUG_PRINTLN("MQTT_CONNECTION_TIMEOUT");
#endif
        break;
      case -3 :
#ifdef TELNET_DEBUG
        DEBUG_PRINTLN("MQTT_CONNECTION_LOST");
#endif
        break;
      case -2 :
#ifdef TELNET_DEBUG
        DEBUG_PRINTLN("MQTT_CONNECT_FAILED");
#endif
        break;
      case -1 :
#ifdef TELNET_DEBUG
        DEBUG_PRINTLN("MQTT_DISCONNECTED");
#endif
        break;
      case 0 :
#ifdef TELNET_DEBUG
        DEBUG_PRINTLN("MQTT_CONNECTED");
#endif
        break;
      case 1 :
#ifdef TELNET_DEBUG
        DEBUG_PRINTLN("MQTT_CONNECT_BAD_PROTOCOL");
#endif
        break;
      case 2 :
#ifdef TELNET_DEBUG
        DEBUG_PRINTLN("MQTT_CONNECT_BAD_CLIENT_ID");
#endif
        break;
      case 3 :
#ifdef TELNET_DEBUG
        DEBUG_PRINTLN("MQTT_CONNECT_UNAVAILABLE");
#endif
        break;
      case 4 :
#ifdef TELNET_DEBUG
        DEBUG_PRINTLN("MQTT_CONNECT_BAD_CREDENTIALS");
#endif
        break;
      case 5 :
#ifdef TELNET_DEBUG
        DEBUG_PRINTLN("MQTT_CONNECT_UNAUTHORIZED");
#endif
        break;
    }
    return 0;
  }
  return 0;
}


void MQTTonConnect(void)
{
#ifdef TELNET_DEBUG
  DEBUG_PRINTLN("MQTT ON CONNECT");
#endif
  MQTTClient.publish(MQTT_LWT, "online");
  MQTTClient.subscribe(MQTTCommandZone1TempSetpoint.c_str());
  MQTTClient.subscribe(MQTTCommandZone1FlowSetpoint.c_str());
  MQTTClient.subscribe(MQTTCommandZone1CurveSetpoint.c_str());
  MQTTClient.subscribe(MQTTCommandSystemHeatingMode.c_str());

  MQTTClient.subscribe(MQTTCommandHotwaterSetpoint.c_str());

  MQTTClient.subscribe(MQTTCommandSystemPower.c_str());
  MQTTClient.subscribe(MQTTCommandSystemTemp.c_str());
}

void MQTTonDisconnect(void* response)
{
  
}

void MQTTonData(char* topic, byte* payload, unsigned int length)
{
  payload[length] = 0;
  String Topic = topic;
  String Payload = (char *)payload;

#ifdef TELNET_DEBUG
  DEBUG_PRINT("Recieved "); DEBUG_PRINT(Topic.c_str());
  DEBUG_PRINT("Payload "); DEBUG_PRINTLN(Payload.c_str());
#endif

  if (Topic == MQTTCommandZone1TempSetpoint) HeatPump.SetZoneTempSetpoint(Payload.toInt(), BOTH);
  if (Topic == MQTTCommandZone1FlowSetpoint) HeatPump.SetZoneFlowSetpoint(Payload.toInt(), BOTH);
  if (Topic == MQTTCommandZone1CurveSetpoint)HeatPump.SetZoneCurveSetpoint(Payload.toInt(), BOTH);


  if (Topic == MQTTCommandHotwaterSetpoint)
  {
#ifdef TELNET_DEBUG
    DEBUG_PRINTLN("MQTT Set HW Setpoint");
#endif
    HeatPump.SetHotWaterSetpoint(Payload.toInt());
  }

  if (Topic == MQTTCommandSystemHeatingMode)
  {
#ifdef TELNET_DEBUG
    DEBUG_PRINTLN("MQTT Set Heating Mode");
#endif
    HeatPump.SetHeatingControlMode(&Payload, BOTH);
  }
  if (Topic == MQTTCommandSystemPower)
  {
#ifdef TELNET_DEBUG
    DEBUG_PRINTLN("MQTT Set System Power Mode");
#endif
    HeatPump.SetSystemPowerMode(&Payload);
  }
  if (Topic == MQTTCommandSystemTemp)
  {
#ifdef TELNET_DEBUG
    DEBUG_PRINTLN("Temp Trigger");
#endif
    HeatPump.Scratch(Payload.toInt());
  }
  //HeatPump.TriggerStatusStateMachine();
}

void Zone1Report(void)
{
  StaticJsonDocument<256> doc;
  char Buffer[256];

  doc[F("Temperature")] = HeatPump.Status.Zone1Temperature;
  doc[F("Setpoint")] = HeatPump.Status.Zone1TemperatureSetpoint;

  serializeJson(doc, Buffer);

  MQTTClient.publish(MQTT_STATUS_ZONE1, Buffer);
#ifdef TELNET_DEBUG
  DEBUG_PRINTLN(Buffer);
#endif
}

void Zone2Report(void)
{
  StaticJsonDocument<256> doc;
  char Buffer[256];

  doc[F("Temperature")] = HeatPump.Status.Zone2Temperature;
  doc[F("Setpoint")] = HeatPump.Status.Zone2TemperatureSetpoint;

  serializeJson(doc, Buffer);
  MQTTClient.publish(MQTT_STATUS_ZONE2, Buffer);
#ifdef TELNET_DEBUG
  DEBUG_PRINTLN(Buffer);
#endif
}

void HotWaterReport(void)
{
  StaticJsonDocument<512> doc;
  char Buffer[512];

  doc["Temperature"] = HeatPump.Status.HotWaterTemperature;
  doc["Setpoint"] = HeatPump.Status.HotWaterSetpoint;
  doc["HotWaterBoostActive"] = HotWaterBoostStr[HeatPump.Status.HotWaterBoostActive];
  doc["HotWaterTimerActive"] = HeatPump.Status.HotWaterTimerActive;
  doc["HotWaterControlMode"] = HowWaterControlModeString[HeatPump.Status.HotWaterControlMode];
  doc["LegionellaSetpoint"] = HeatPump.Status.LegionellaSetpoint;
  doc["HotWaterMaximumTempDrop"] = HeatPump.Status.HotWaterMaximumTempDrop;

  serializeJson(doc, Buffer);
  MQTTClient.publish(MQTT_STATUS_HOTWATER, Buffer);
#ifdef TELNET_DEBUG
  DEBUG_PRINTLN(Buffer);
#endif
}

void SystemReport(void)
{
  StaticJsonDocument<256> doc;
  char Buffer[256];

  doc["HeaterFlow"] = HeatPump.Status.HeaterOutputFlowTemperature;
  doc["HeaterReturn"] = HeatPump.Status.HeaterReturnFlowTemperature;
  doc["HeaterSetpoint"] = HeatPump.Status.HeaterFlowSetpoint;
  doc["OutsideTemp"] = HeatPump.Status.OutsideTemperature;
  doc["HeaterPower"] = HeatPump.Status.OutputPower;
  doc["SystemPower"] = SystemPowerModeString[HeatPump.Status.SystemPowerMode];
  doc["SystemOperationMode"] = SystemOperationModeString[HeatPump.Status.SystemOperationMode];
  doc["HeatingControlMode"] = HeatingControlModeString[HeatPump.Status.HeatingControlMode];
  doc["FlowRate"] = HeatPump.Status.PrimaryFlowRate;

  serializeJson(doc, Buffer);
  MQTTClient.publish(MQTT_STATUS_SYSTEM, Buffer);
#ifdef TELNET_DEBUG
  DEBUG_PRINTLN(Buffer);
#endif
}

void TestReport(void)
{
  StaticJsonDocument<512> doc;
  char Buffer[512];

  doc["Z1FSP"] = HeatPump.Status.Zone1FlowTemperatureSetpoint;
  doc["CHEAT"] = HeatPump.Status.ConsumedHeatingEnergy;
  doc["CDHW"] = HeatPump.Status.ConsumedHotWaterEnergy;
  doc["DHEAT"] = HeatPump.Status.DeliveredHeatingEnergy;
  doc["DDHW"] = HeatPump.Status.DeliveredHotWaterEnergy;
  doc["Compressor"] = HeatPump.Status.CompressorFrequency;
  doc["RunHours"] = HeatPump.Status.RunHours;
  doc["FlowTMax"] = HeatPump.Status.FlowTempMax;
  doc["FlowTMin"] = HeatPump.Status.FlowTempMin;
  doc["Unknown5"] = HeatPump.Status.UnknownMSG5;
  doc["Defrost"] = HeatPump.Status.Defrost;

  serializeJson(doc, Buffer);
  MQTTClient.publish(MQTT_STATUS_TEST, Buffer);
#ifdef TELNET_DEBUG
  DEBUG_PRINTLN(Buffer);
#endif
}


void setupTelnet()
{
  TelnetServer.onConnect(onTelnetConnect);
  TelnetServer.onConnectionAttempt(onTelnetConnectionAttempt);
  TelnetServer.onReconnect(onTelnetReconnect);
  TelnetServer.onDisconnect(onTelnetDisconnect);


#ifdef TELNET_DEBUG
  DEBUG_PRINT("Telnet: ");
#endif
  if (TelnetServer.begin())
  {
#ifdef TELNET_DEBUG
    DEBUG_PRINTLN("running");
#endif
  }
  else
  {
#ifdef TELNET_DEBUG
    DEBUG_PRINTLN("error.");
#endif
    //errorMsg("Will reboot...");
  }
}

void onTelnetConnect(String ip)
{
#ifdef TELNET_DEBUG
  DEBUG_PRINT("Telnet: ");
  DEBUG_PRINT(ip);
  DEBUG_PRINTLN(" connected");
#endif
  TelnetServer.println("\nWelcome " + TelnetServer.getIP());
  TelnetServer.println("(Use ^] + q  to disconnect.)");
}

void onTelnetDisconnect(String ip)
{
#ifdef TELNET_DEBUG
  DEBUG_PRINT("Telnet: ");
  DEBUG_PRINT(ip);
  DEBUG_PRINTLN(" disconnected");
#endif
}

void onTelnetReconnect(String ip)
{
#ifdef TELNET_DEBUG
  DEBUG_PRINT("Telnet: ");
  DEBUG_PRINT(ip);
  DEBUG_PRINTLN(" reconnected");
#endif
}

void onTelnetConnectionAttempt(String ip)
{
#ifdef TELNET_DEBUG
  DEBUG_PRINT("Telnet: ");
  DEBUG_PRINT(ip);
  DEBUG_PRINTLN(" tried to connected");
#endif
}
