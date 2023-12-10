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
//String HostName;

void HeatPumpQueryStateEngine(void);
void HeatPumpKeepAlive(void);
void Zone1Report(void);
void Zone1Report(void);
void HotWaterReport(void);
void SystemReport(void);
void TestReport(void);

TimerCallBack HeatPumpQuery1(500, HeatPumpQueryStateEngine);
TimerCallBack HeatPumpQuery2(60 *  1000, HeatPumpKeepAlive);

//MQTT stuff
/*
 * MQTT_STATUS_ZONE1
MQTT_STATUS_ZONE2
MQTT_STATUS_HOTWATER
MQTT_STATUS_SYSTEM
MQTT_STATUS_TEST
MQTT_LWT
 */
String HostName = "EcodanStano";
String g_mqttStatusTopic = "esp32iotsensor/" + HostName;
//stuff from mitsubishi2mqtt for CLIMATE
bool useFahrenheit = false;
String mqtt_topic                     = "mitsubishi2mqtt_stano/Ecodan_stano";
String g_UniqueId = "c8f09ef3f07c";

/*
String ha_mode_set_topic              = mqtt_topic + "/mode/set";
String ha_temp_set_topic              = mqtt_topic + "/temp/set";
String ha_remote_temp_set_topic       = mqtt_topic + "/remote_temp/set";
String ha_settings_topic              = mqtt_topic + "/settings";
String ha_state_topic                 = mqtt_topic + "/state";
String ha_custom_packet               = mqtt_topic + "/custom/send";
String ha_availability_topic          = mqtt_topic + "/availability";
String ha_system_set_topic            = mqtt_topic + "/system/set";
*/
/*
String ha_mode_set_topic              = "homeassistant/climate/" + g_UniqueId + "/mode/set";
String ha_temp_set_topic              = "homeassistant/climate/" + g_UniqueId + "/temp/set";
String ha_remote_temp_set_topic       = "homeassistant/climate/" + g_UniqueId + "/remote_temp/set";
String ha_settings_topic              = "homeassistant/climate/" + g_UniqueId + "/settings";
String ha_state_topic                 = "homeassistant/climate/" + g_UniqueId + "/state";
String ha_custom_packet               = "homeassistant/climate/" + g_UniqueId + "/custom/send";
String ha_availability_topic          = "homeassistant/climate/" + g_UniqueId + "/availability";
String ha_system_set_topic            = "homeassistant/climate/" + g_UniqueId + "/system/set";
*/
String ha_mode_set_topic              = "homeassistant/climate/" + g_UniqueId + "/mode/set";
String ha_temp_set_topic              = "homeassistant/climate/" + g_UniqueId + "/temp/set";
String ha_remote_temp_set_topic       = "homeassistant/climate/" + g_UniqueId + "/remote_temp/set";
String ha_settings_topic              = "homeassistant/climate/" + g_UniqueId + "/settings";
String ha_state_topic                 = "homeassistant/climate/" + g_UniqueId + "/state";
String ha_custom_packet               = "homeassistant/climate/" + g_UniqueId + "/custom/send";
String ha_availability_topic          = "homeassistant/climate/" + g_UniqueId + "/availability";
String ha_system_set_topic            = "homeassistant/climate/" + g_UniqueId + "/system/set";


String mqtt_payload_unavailable       = "offline";
String mqtt_payload_available         = "online";
unsigned long min_temp = 16.0;
unsigned long max_temp = 24.0;


//end





void MQTT_HA_Discover();
//String g_UniqueId = "c8f09ef3f07c";
const char* g_deviceModel = "ESP32_EcodanHP_Bridge_v01";                            // Hardware Model
const char*         g_swVersion = "0.0.1";                                      // Firmware Version
const char*         g_manufacturer = "Satano";                               // Manufacturer Name
//String g_mqttStatusTopic;

String MQTTCommandZone1TempSetpoint = MQTT_COMMAND_ZONE1_TEMP_SETPOINT;
String MQTTCommandZone1FlowSetpoint = MQTT_COMMAND_ZONE1_FLOW_SETPOINT;
String MQTTCommandZone1CurveSetpoint = MQTT_COMMAND_ZONE1_CURVE_SETPOINT;

String MQTTCommandHotwaterSetpoint = MQTT_COMMAND_HOTWATER_SETPOINT;

String MQTTCommandSystemHeatingMode = MQTT_COMMAND_SYSTEM_HEATINGMODE;
String MQTTCommandSystemPower = MQTT_COMMAND_SYSTEM_POWER;
String MQTTCommandSystemTemp = MQTT_COMMAND_SYSTEM_TEMP;
//End MQTT stuff

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
  /*HostName = "EcodanBridge-";
#ifdef ESP32
  HostName += String(ESP.getChipModel(), HEX);
#else
  HostName += String(ESP.getChipId(), HEX);
#endif
  //HostName.remove(HostName.length()-17);*/
  //HostName = "EcBrHPSTAv01";
//  HostName = "EcodanHP";
//  g_mqttStatusTopic = "esp32iotsensor/" + HostName;
  WiFi.hostname(HostName);

  setupTelnet();

  OTASetup(HostName.c_str());
  MQTTClient.setServer(MQTT_SERVER, MQTT_PORT);
  MQTTClient.setCallback(MQTTonData);

  byte mac[6];
  WiFi.macAddress(mac);
  //g_UniqueId =  String(mac[0],HEX) +String(mac[1],HEX) +String(mac[2],HEX) +String(mac[3],HEX) + String(mac[4],HEX) + String(mac[5],HEX);
#ifdef TELNET_DEBUG
    DEBUG_PRINT("g_UniqueID"); DEBUG_PRINTLN(g_UniqueId.c_str());
#endif
}

void loop()
{
  if (!MQTTReconnect())
  {
    return;
  }
  MQTTClient.loop();
  //MqttHomeAssistantDiscovery();
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
  String MQTTDebugTopic_str = MQTTDebugTopic;
  DEBUG_PRINTLN("homeassistant/status");
  DEBUG_PRINT("MQTTCommandZone1TempSetpoint  "); DEBUG_PRINTLN(MQTTCommandZone1TempSetpoint.c_str());
  DEBUG_PRINT("MQTTCommandZone1FlowSetpoint  "); DEBUG_PRINTLN(MQTTCommandZone1FlowSetpoint.c_str());
  DEBUG_PRINT("MQTTCommandZone1CurveSetpoint  "); DEBUG_PRINTLN(MQTTCommandZone1CurveSetpoint.c_str());
  DEBUG_PRINT("MQTTCommandSystemHeatingMode  "); DEBUG_PRINTLN(MQTTCommandSystemHeatingMode.c_str());
  DEBUG_PRINT("MQTTCommandHotwaterSetpoint  "); DEBUG_PRINTLN(MQTTCommandHotwaterSetpoint.c_str());
  DEBUG_PRINT("MQTTCommandSystemPower  "); DEBUG_PRINTLN(MQTTCommandSystemPower.c_str());
  DEBUG_PRINT("MQTTCommandSystemTemp  "); DEBUG_PRINTLN(MQTTCommandSystemTemp.c_str());
  DEBUG_PRINT("MQTTDebugTopic  "); DEBUG_PRINTLN(MQTTDebugTopic_str.c_str());
  MQTTClient.subscribe(MQTTDebugTopic_str.c_str());
#endif
  MQTTClient.subscribe("homeassistant/status");
  MQTTClient.publish(MQTT_LWT, "online");
  MQTTClient.subscribe(MQTTCommandZone1TempSetpoint.c_str());
  MQTTClient.subscribe(MQTTCommandZone1FlowSetpoint.c_str());
  MQTTClient.subscribe(MQTTCommandZone1CurveSetpoint.c_str());
  MQTTClient.subscribe(MQTTCommandSystemHeatingMode.c_str());

  MQTTClient.subscribe(MQTTCommandHotwaterSetpoint.c_str());

  MQTTClient.subscribe(MQTTCommandSystemPower.c_str());
  MQTTClient.subscribe(MQTTCommandSystemTemp.c_str());
  MqttHomeAssistantDiscovery();
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

  //doc[F("Temperature")] = HeatPump.Status.Zone1Temperature;
  //doc[F("Setpoint")] = HeatPump.Status.Zone1TemperatureSetpoint;
  doc["ZONE_1_Temperature"] = HeatPump.Status.Zone1Temperature;
  doc["ZONE_1_Temperature_Setpoint"] = HeatPump.Status.Zone1TemperatureSetpoint;

  serializeJson(doc, Buffer);


  //MQTTClient.publish(MQTT_STATUS_ZONE1, Buffer);
  //MQTTClient.publish(g_mqttStatusTopic.c_str(), Buffer);
  MQTTClient.publish(ha_state_topic.c_str(), Buffer);
#ifdef TELNET_DEBUG
  DEBUG_PRINTLN(g_mqttStatusTopic.c_str());
  DEBUG_PRINTLN(Buffer);

  doc.clear();
//  doc["state"] = "online";
//  doc["mode"] = "heat_cool";
  serializeJson(doc, Buffer);
  MQTTClient.publish(ha_availability_topic.c_str(), mqtt_payload_available.c_str(), true);
  
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

// temperature helper functions
float toFahrenheit(float fromCelcius) {
  return round(1.8 * fromCelcius + 32.0);
}

float toCelsius(float fromFahrenheit) {
  return (fromFahrenheit - 32.0) / 1.8;
}

float convertCelsiusToLocalUnit(float temperature, bool isFahrenheit) {
  if (isFahrenheit) {
    return toFahrenheit(temperature);
  } else {
    return temperature;
  }
}


void MqttHomeAssistantDiscovery()
/*
 * This method registers topics in MQTT Server 
 * so that Home Assistant can automatically discover all the topics
 * and send/receive the data
 */
{
    String discoveryTopic;
    String setdiscoveryTopic;
    String stateTopic;
    //String payload;
    //String strPayload;
    if(MQTTClient.connected())
    {
        Serial.println("SEND HOME ASSISTANT DISCOVERY!!!");
        StaticJsonDocument<600> payload;
        JsonObject device;
        JsonArray identifiers;
        
        const size_t capacity = JSON_ARRAY_SIZE(5) + 2 * JSON_ARRAY_SIZE(6) + JSON_ARRAY_SIZE(7) + JSON_OBJECT_SIZE(24) + 2048; //this is taken from mitsubishi2mqtt but shall be adjusted (decreased) in size acc to what I use here
        DynamicJsonDocument haConfig(capacity);
        
        String g_deviceName = HostName;
#ifdef TELNET_DEBUG
        DEBUG_PRINT("HostName: "); DEBUG_PRINTLN(HostName.c_str());
#endif        
        //String g_mqttStatusTopic = "esp32iotsensor/" + g_deviceName;     // MQTT Topic

//        payload["name"] = g_deviceName;
//        payload["uniq_id"] = g_UniqueId;

/*        bool useFahrenheit = false;
        String mqtt_topic                     = "mitsubishi2mqtt_stano/Ecodan_stano";
        String ha_mode_set_topic              = mqtt_topic + "/mode/set";
        String ha_temp_set_topic              = mqtt_topic + "/temp/set";
        String ha_remote_temp_set_topic       = mqtt_topic + "/remote_temp/set";
        String ha_settings_topic              = mqtt_topic + "/settings";
        String ha_state_topic                 = mqtt_topic + "/state";
        String ha_custom_packet               = mqtt_topic + "/custom/send";
        String ha_availability_topic          = mqtt_topic + "/availability";
        String ha_system_set_topic            = mqtt_topic + "/system/set";
        String mqtt_payload_unavailable       = "offline";
        String mqtt_payload_available         = "online";
        unsigned long min_temp = 16.0;
        unsigned long max_temp = 24.0;*/



/*        MQTTClient.subscribe(ha_mode_set_topic.c_str());
        MQTTClient.subscribe(ha_temp_set_topic.c_str());
        MQTTClient.subscribe(ha_remote_temp_set_topic.c_str());
        MQTTClient.subscribe(ha_custom_packet.c_str());
        MQTTClient.subscribe(ha_system_set_topic.c_str());*/
        
        haConfig["name"]                          = "EcodanStano";
        haConfig["uniq_id"]                       = g_UniqueId;
        haConfig["mode_cmd_t"]                    = ha_mode_set_topic;
        haConfig["mode_stat_t"]                   = ha_state_topic;
        haConfig["mode_stat_tpl"]                 = F("{{ value_json.mode if (value_json is defined and value_json.mode is defined and value_json.mode|length) else 'off' }}"); //Set default value for fix "Could not parse data for HA"
        haConfig["temp_cmd_t"]                    = ha_temp_set_topic;
        haConfig["temp_stat_t"]                   = ha_state_topic;
        //haConfig["avty_t"]                        = ha_availability_topic; // MQTT last will (status) messages topic

        JsonObject haAvailability = haConfig.createNestedObject("availability");
        haAvailability["topic"]                   = ha_availability_topic; // MQTT last will (status) messages topic
        haAvailability["pl_not_avail"]            = mqtt_payload_unavailable; // MQTT offline message payload
        haAvailability["pl_avail"]                = mqtt_payload_available; // MQTT online message payload
        //String avail_tpl_str                      = F("{{ '");
        //String avail_tpl_str                             = "{{ '" + mqtt_payload_available + "' if float(as_timestamp(now()) - value_json.timestamp) < 5 else '" + mqtt_payload_unavailable + "' }}";
        //haAvailability["value_template"]          = F("{{ 'online' if float(as_timestamp(now()) - value_json.timestamp) < 5 else 'offline' }}");
        //haAvailability["value_template"] = avail_tpl_str;
        
        haConfig["availability_mode"]             = "latest";
        //haConfig["pl_not_avail"]                  = mqtt_payload_unavailable; // MQTT offline message payload
        //haConfig["pl_avail"]                      = mqtt_payload_available; // MQTT online message payload
        //Set default value for fix "Could not parse data for HA"
        String temp_stat_tpl_str                  = F("{% if (value_json is defined and value_json.ZONE_1_Temperature_Setpoint is defined) %}{% if (value_json.ZONE_1_Temperature_Setpoint|int >= ");
        temp_stat_tpl_str                        +=(String)convertCelsiusToLocalUnit(min_temp, useFahrenheit) + " and value_json.ZONE_1_Temperature_Setpoint|int <= ";
        temp_stat_tpl_str                        +=(String)convertCelsiusToLocalUnit(max_temp, useFahrenheit) + ") %}{{ value_json.ZONE_1_Temperature_Setpoint }}";
        temp_stat_tpl_str                        +="{% elif (value_json.ZONE_1_Temperature_Setpoint|int < " + (String)convertCelsiusToLocalUnit(min_temp, useFahrenheit) + ") %}" + (String)convertCelsiusToLocalUnit(min_temp, useFahrenheit) + "{% elif (value_json.ZONE_1_Temperature_Setpoint|int > " + (String)convertCelsiusToLocalUnit(max_temp, useFahrenheit) + ") %}" + (String)convertCelsiusToLocalUnit(max_temp, useFahrenheit) +  "{% endif %}{% else %}" + (String)convertCelsiusToLocalUnit(22, useFahrenheit) + "{% endif %}";
        
        //String temp_stat_tpl_str                  = F("{{ value_json.ZONE_1_Temperature_Setpoint if (value_json is defined and value_json.ZONE_1_Temperature_Setpoint is defined) else 21 }}");

        haConfig["temp_stat_tpl"]                 = temp_stat_tpl_str;
        haConfig["curr_temp_t"]                   = ha_state_topic;
        String curr_temp_tpl_str                  = F("{{ value_json.ZONE_1_Temperature if (value_json is defined and value_json.ZONE_1_Temperature is defined and value_json.ZONE_1_Temperature|int > ");
        curr_temp_tpl_str                        += (String)convertCelsiusToLocalUnit(1, useFahrenheit) + ") }}"; //Set default value for fix "Could not parse data for HA"
        
        //String curr_temp_tpl_str                  = F("{{ value_json.ZONE_1_Temperature if (value_json is defined and value_json.ZONE_1_Temperature is defined) }}");
        haConfig["curr_temp_tpl"]                 = curr_temp_tpl_str;
        haConfig["min_temp"]                      = convertCelsiusToLocalUnit(min_temp, useFahrenheit);
        haConfig["max_temp"]                      = convertCelsiusToLocalUnit(max_temp, useFahrenheit);
        haConfig["temp_step"]                     = 0.1;//temp_step;
        //haConfig["temperature_unit"]              = useFahrenheit ? "F" : "C";
        haConfig["temperature_unit"]              = "C";
        device = haConfig.createNestedObject("device");
        device["name"] = g_deviceName;
        device["model"] = g_deviceModel;
        device["sw_version"] = g_swVersion;
        device["manufacturer"] = g_manufacturer;
        identifiers = device.createNestedArray("identifiers");
        identifiers.add(g_UniqueId);

        JsonArray haConfigModes = haConfig.createNestedArray("modes");
        haConfigModes.add("heat_cool"); //native AUTO mode
        haConfigModes.add("cool");
//        if (supportHeatMode) {
          haConfigModes.add("heat");
//        }
//        haConfigModes.add("fan_only");  //native FAN mode
        haConfigModes.add("off");


        discoveryTopic = "homeassistant/climate/" + g_deviceName + "/config";
        boolean result = MQTTClient.beginPublish(discoveryTopic.c_str(), measureJson(haConfig),true);
        serializeJson(haConfig, MQTTClient);
        result = MQTTClient.endPublish();
        DEBUG_PRINTLN(discoveryTopic.c_str());
        DEBUG_PRINT("result: "); DEBUG_PRINTLN(result);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // ECODAN MQTT_STATUS_ZONE1 TempSetpoint
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*        device.clear();
        payload.clear();
        discoveryTopic = "homeassistant/climate/" + g_deviceName + "_ZONE1TempSetpoint" + "/config";
        stateTopic = "homeassistant/climate/" + g_deviceName + "/state";
        setdiscoveryTopic = "homeassistant/climate/" + g_deviceName + "/set";
#ifdef TELNET_DEBUG
        //Serial.print("discoveryTopic: ");
        //Serial.println(discoveryTopic.c_str());
        DEBUG_PRINT("discoveryTopic: "); DEBUG_PRINTLN(discoveryTopic.c_str());
#endif
        payload["name"] = g_deviceName + "ZONE_1_Temperature_Setpoint";
        payload["uniq_id"] = g_UniqueId + "ZONE_1_Temperature_Setpoint";
#ifdef TELNET_DEBUG
        DEBUG_PRINT("g_UniqueID"); DEBUG_PRINTLN(g_UniqueId.c_str());
#endif

        payload["stat_t"] = g_mqttStatusTopic;
        payload["command_topic"] = setdiscoveryTopic;
        payload["command_template"] = "{\"val\":\"{{ value }}\"}";
        payload["min"] = 16;
        payload["max"] = 25;
        payload["step"] = 0.5;
        MQTTClient.subscribe(setdiscoveryTopic.c_str());
        payload["dev_cla"] = "temperature";
//        payload["val_tpl"] = "{{ value_json.ZONE_1_Temperature_Setpoint| is_defined }}";
        payload["val_tpl"] = "{{ value_json.ZONE_1_Temperature_Setpoint| float }}";
        payload["unit_of_meas"] = "°C";
        device = payload.createNestedObject("device");
        device["name"] = g_deviceName;
        device["model"] = g_deviceModel;
        device["sw_version"] = g_swVersion;
        device["manufacturer"] = g_manufacturer;
        identifiers = device.createNestedArray("identifiers");
        identifiers.add(g_UniqueId);

        serializeJsonPretty(payload, Serial);
        Serial.println(" ");
        //serializeJson(payload, strPayload);

        //MQTTClient.publish(discoveryTopic.c_str(), strPayload.c_str());
        boolean result = MQTTClient.beginPublish(discoveryTopic.c_str(), measureJson(payload),true);
        serializeJson(payload, MQTTClient);
        result = MQTTClient.endPublish();



        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // ECODAN MQTT_STATUS_ZONE1 Temp
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        payload.clear();
        device.clear();
        identifiers.clear();
        //strPayload.clear();
        discoveryTopic = "homeassistant/climate/" + g_deviceName + "_ZONE1Temp" + "/config";
#ifdef TELNET_DEBUG
        //Serial.print("discoveryTopic: ");
        //Serial.println(discoveryTopic.c_str());
        DEBUG_PRINT("discoveryTopic: "); DEBUG_PRINTLN(discoveryTopic.c_str());
#endif
        payload["name"] = g_deviceName + "ZONE_1_Temperature";
        payload["uniq_id"] = g_UniqueId + "ZONE_1_Temperature";
        payload["stat_t"] = g_mqttStatusTopic;
        payload["dev_cla"] = "temperature";
        payload["val_tpl"] = "{{ value_json.ZONE_1_Temperature | is_defined }}";
        payload["unit_of_meas"] = "°C";
        device = payload.createNestedObject("device");
        device["name"] = g_deviceName;
        device["model"] = g_deviceModel;
        device["sw_version"] = g_swVersion;
        device["manufacturer"] = g_manufacturer;
        identifiers = device.createNestedArray("identifiers");
        identifiers.add(g_UniqueId);

        serializeJsonPretty(payload, Serial);
        Serial.println(" ");
        //serializeJson(payload, strPayload);

        //MQTTClient.publish(discoveryTopic.c_str(), strPayload.c_str());
        result = MQTTClient.beginPublish(discoveryTopic.c_str(), measureJson(payload),true);
        serializeJson(payload, MQTTClient);
        result = MQTTClient.endPublish();

*/



        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // ECODAN MQTT_STATUS_ZONE1 FlowSetpoint
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*        payload.clear();
        device.clear();
        identifiers.clear();
        strPayload.clear();
        discoveryTopic = "homeassistant/sensor/esp32iotsensor/" + g_deviceName + "_ZONE1/FlowSetpoint" + "/config";
#ifdef TELNET_DEBUG
        //Serial.print("discoveryTopic: ");
        //Serial.println(discoveryTopic.c_str());
        DEBUG_PRINT("discoveryTopic: "); DEBUG_PRINTLN(discoveryTopic.c_str());
#endif
        payload["name"] = g_deviceName + ".temp";
        payload["uniq_id"] = g_UniqueId + "_temp";
        payload["stat_t"] = g_mqttStatusTopic;
        payload["dev_cla"] = "temperature";
        payload["val_tpl"] = "{{ value_json.temp | is_defined }}";
        payload["unit_of_meas"] = "°C";
        device = payload.createNestedObject("device");
        device["name"] = g_deviceName;
        device["model"] = g_deviceModel;
        device["sw_version"] = g_swVersion;
        device["manufacturer"] = g_manufacturer;
        identifiers = device.createNestedArray("identifiers");
        identifiers.add(g_UniqueId);

        serializeJsonPretty(payload, Serial);
        Serial.println(" ");
        //serializeJson(payload, strPayload);

        //MQTTClient.publish(discoveryTopic.c_str(), strPayload.c_str());
        boolean result = MQTTClient.beginPublish(discoveryTopic.c_str(), measureJson(payload),true);
        serializeJson(payload, MQTTClient);
        result = MQTTClient.endPublish();
        */

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Temperature
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*        payload.clear();
        device.clear();
        identifiers.clear();
        //strPayload.clear();
        discoveryTopic = "homeassistant/sensor/esp32iotsensor/" + g_deviceName + "_temp" + "/config";
#ifdef TELNET_DEBUG
        //Serial.print("discoveryTopic: ");
        //Serial.println(discoveryTopic.c_str());
        DEBUG_PRINT("discoveryTopic: "); DEBUG_PRINTLN(discoveryTopic.c_str());
#endif
        payload["name"] = g_deviceName + ".temp";
        payload["uniq_id"] = g_UniqueId + "_temp";
        payload["stat_t"] = g_mqttStatusTopic;
        payload["dev_cla"] = "temperature";
        payload["val_tpl"] = "{{ value_json.temp | is_defined }}";
        payload["unit_of_meas"] = "°C";
        device = payload.createNestedObject("device");
        device["name"] = g_deviceName;
        device["model"] = g_deviceModel;
        device["sw_version"] = g_swVersion;
        device["manufacturer"] = g_manufacturer;
        identifiers = device.createNestedArray("identifiers");
        identifiers.add(g_UniqueId);

        serializeJsonPretty(payload, Serial);
        Serial.println(" ");
        //serializeJson(payload, strPayload);

        //MQTTClient.publish(discoveryTopic.c_str(), strPayload.c_str());
        boolean result = MQTTClient.beginPublish(discoveryTopic.c_str(), measureJson(payload),true);
        serializeJson(payload, MQTTClient);
        result = MQTTClient.endPublish();

        */

        
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Humidity
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*        payload.clear();
        device.clear();
        identifiers.clear();
        //strPayload.clear();

        discoveryTopic = "homeassistant/sensor/esp32iotsensor/" + g_deviceName + "_hum" + "/config";
        
        payload["name"] = g_deviceName + ".hum";
        payload["uniq_id"] = g_UniqueId + "_hum";
        payload["stat_t"] = g_mqttStatusTopic;
        payload["dev_cla"] = "humidity";
        payload["val_tpl"] = "{{ value_json.hum | is_defined }}";
        payload["unit_of_meas"] = "%";
        device = payload.createNestedObject("device");
        device["name"] = g_deviceName;
        device["model"] = g_deviceModel;
        device["sw_version"] = g_swVersion;
        device["manufacturer"] = g_manufacturer;
        identifiers = device.createNestedArray("identifiers");
        identifiers.add(g_UniqueId);

        serializeJsonPretty(payload, Serial);
        Serial.println(" ");
        //serializeJson(payload, strPayload);

        //MQTTClient.publish(discoveryTopic.c_str(), strPayload.c_str());
        result = MQTTClient.beginPublish(discoveryTopic.c_str(), measureJson(payload),true);
        serializeJson(payload, MQTTClient);
        result = MQTTClient.endPublish();

*/
        
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Binary Door
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        payload.clear();
        device.clear();
        identifiers.clear();
        //strPayload.clear();

        /*discoveryTopic = "homeassistant/binary_sensor/esp32iotsensor/" + g_deviceName + "_door" + "/config";
        
        payload["name"] = g_deviceName + ".door";
        payload["uniq_id"] = g_UniqueId + "_door";
        payload["stat_t"] = g_mqttStatusTopic;
        payload["dev_cla"] = "door";
        payload["val_tpl"] = "{{ value_json.inputstatus | is_defined }}";
        device = payload.createNestedObject("device");
        device["name"] = g_deviceName;
        device["model"] = g_deviceModel;
        device["sw_version"] = g_swVersion;
        device["manufacturer"] = g_manufacturer;
        identifiers = device.createNestedArray("identifiers");
        identifiers.add(g_UniqueId);

        serializeJsonPretty(payload, Serial);
        Serial.println(" ");
        //serializeJson(payload, strPayload);

        //MQTTClient.publish(discoveryTopic.c_str(), strPayload.c_str());
        result = MQTTClient.beginPublish(discoveryTopic.c_str(), measureJson(payload),true);
        serializeJson(payload, MQTTClient);
        result = MQTTClient.endPublish();*/

    }
}
