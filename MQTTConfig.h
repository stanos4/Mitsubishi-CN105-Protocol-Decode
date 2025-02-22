//#define MQTT_SERVER Set_Your_MQTT_Server_Here //  Fill in below, and delete this line
#include "Secrets.h" //contains MQTT Server, Port, Username, Passwd
//#define MQTT_SERVER "192.168.1.1"
//#define MQTT_PORT 1883
//#define MQTT_USER  Set_Your_MQTT_User_Name_Here // Fill in below and delete this line
//#define MQTT_USER "username"
//#define MQTT_PASS "password"


#define MQTT_BASETOPIC "Ecodan/HomeHeatPump"

#define MQTT_STATUS MQTT_BASETOPIC "/Status"
#define MQTT_COMMAND MQTT_BASETOPIC "/Command"

#define MQTT_STATUS_ZONE1 MQTT_STATUS "/Zone1"
#define MQTT_STATUS_ZONE2 MQTT_STATUS "/Zone2"
#define MQTT_STATUS_HOTWATER MQTT_STATUS "/HotWater"
#define MQTT_STATUS_SYSTEM MQTT_STATUS "/System"
#define MQTT_STATUS_TEST MQTT_STATUS "/Test"

#define MQTT_COMMAND_ZONE1 MQTT_COMMAND "/Zone1"
#define MQTT_COMMAND_ZONE2 MQTT_COMMAND "/Zone2"
#define MQTT_COMMAND_HOTWATER MQTT_COMMAND "/HotWater"
#define MQTT_COMMAND_SYSTEM MQTT_COMMAND "/System"

#define MQTT_COMMAND_ZONE1_TEMP_SETPOINT MQTT_COMMAND_ZONE1  "/TempSetpoint"
#define MQTTDebugTopic MQTT_COMMAND_ZONE1  "/TempSetpointDebug" //Ecodan/HomeHeatPump/Command/Zone1/TempSetpointDebugPayload
#define MQTT_COMMAND_ZONE1_FLOW_SETPOINT MQTT_COMMAND_ZONE1  "/FlowSetpoint"
#define MQTT_COMMAND_ZONE1_CURVE_SETPOINT MQTT_COMMAND_ZONE1  "/CurveSetpoint"

#define MQTT_COMMAND_SYSTEM_HEATINGMODE MQTT_COMMAND_SYSTEM "/HeatingMode"
#define MQTT_COMMAND_HOTWATER_SETPOINT MQTT_COMMAND_HOTWATER "/Setpoint"
#define MQTT_COMMAND_SYSTEM_POWER MQTT_COMMAND_SYSTEM "/Power"
#define MQTT_COMMAND_SYSTEM_TEMP MQTT_COMMAND_SYSTEM "/Temp"


#define MQTT_LWT MQTT_BASETOPIC "/LWT"
