/*
Name:       wESP32.ino
Created:    08-Nov-18 12:44:00
Author:     Joaquim Monteiro
*/

#include <EEPROM.h>
#include <TaskScheduler.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <rom/rtc.h>
#include "error_lib.h"
#include "wdevice.h"

//**** DEFINE CONSTANTS 
#define MAJOR_VERSION       1
#define MINOR_VERSION       10
#define SLEEP_PERIOD        30e6	//20 seconds
#define SHUTDOWN_TIMEOUT    500
#define TASK_RUN_TWICE      2
#define BUILTIN_LED         2
#define SERIAL_BAUDRATE     115200
#define ALARM_THRESHOLD     3000



//**** NETWORK CREDENTIALS
const char* ssid    = "(-_-)";
const char* psk     = "monteiro";

//**** SERVER INFO [cloudmqtt.com] ****
const char* mqtt_server     = "m20.cloudmqtt.com";
const char* mqtt_user       = "lgnnlude";
const char* mqtt_password   = "iWg6ghr1pMLO";
const int   mqtt_port       = 17283;
const char* will_message    = "CONNECTION_LOST";
const char* main_topic      = "MAIN";
String      device_topic;

//**** CALLBACK METHODS
void t1Callback();
void t2Callback();
void t3Callback();
void t4Callback();

//**** TASK DECLARATIONS
Task t1(1000, TASK_FOREVER, &t1Callback);		// re-check connections
Task t2(10000, TASK_FOREVER, &t2Callback);		// update and publish data
Task t3(500, TASK_FOREVER, &t3Callback);		// 
Task t4(5000, TASK_RUN_TWICE, &t4Callback);		// deep sleep

												//Objects
WiFiClient espClient;
PubSubClient client(espClient);
wDevice thisDevice;
Scheduler runner;
//ADC_MODE(ADC_VCC);

//***************************************************//

void t1Callback() {
	Serial.print("t1: ");
	serialDebug(millis());

	if (WiFi.status() != WL_CONNECTED) {
		setupWifi();
	}

	if (!client.connected()) {
		mqtt_connect();
	}

}

void t2Callback() {
	Serial.print("t2: ");
	serialDebug(millis());





	if (client.connected()) {
		publish();
	}

}

void t3Callback() {
	Serial.print("t3: ");
	serialDebug(millis());

}

void t4Callback() {
	Serial.print("t4: ");
	serialDebug(millis());


	if (t4.isLastIteration()) {
		publish();
		terminateAll();
		ESP.deepSleep(SLEEP_PERIOD);
	}
}

void terminateAll() {
	int timeout = 0;

	client.disconnect();
	serialDebug("client.disconnect()");
	runner.disableAll();
	serialDebug("runner.disableAll()");
	WiFi.disconnect();
	serialDebug("WiFi.disconnect()");
	while (WiFi.isConnected() || client.connected()) {
		yield();
		timeout++;
		if (timeout == SHUTDOWN_TIMEOUT) {
			return;
		}
	}
	serialDebug(String(timeout) + ": all processes terminated.");
}

void subscribe() {

	if (!client.connected()) {
		return;
	}

	int fail = 0;
	Serial.print("Subscribing topics..");

	fail += !client.subscribe((device_topic + "/outputs").c_str(), 1);
	fail += !client.subscribe((device_topic + "/config").c_str(), 1);


	if (fail) {
		serialDebug("failed.");
	}
	else {
		serialDebug("ok!");
	}
}

void publish() {

	if (!client.connected()) {
		return;
	}
	digitalWrite(BUILTIN_LED, HIGH);
	String payload;
	int fail = 0;
    int analogValue;
	Serial.print("Publishing messages..");


	payload = String(WiFi.RSSI());
	fail += !client.publish((device_topic + "/signal").c_str(), payload.c_str(), true);
	/*
	payload = String((float)ESP.getFreeHeap() / 1024);
	fail += !client.publish((device_topic + "/memory").c_str(), payload.c_str(), true);*/
    analogValue = thisDevice.getInput(ANALOG_IN_0);
	fail += !client.publish((device_topic + "/sensor_0").c_str(), String(analogValue).c_str(), true);
    if ((analogValue >= thisDevice.getOutputTrigger[0]) && (thisDevice.getOutput(0) == false)) {
        thisDevice.setOutput(DIGITAL_OUT_0, HIGH);
		fail += !client.publish((device_topic + "/out_0").c_str(), "1", true);
    }
    analogValue = thisDevice.getInput(ANALOG_IN_1);
    fail += !client.publish((device_topic + "/sensor_1").c_str(), String(analogValue).c_str(), true);
    if ((analogValue >= thisDevice.getOutputTrigger[1]) && (thisDevice.getOutput(1) == false)) {
        thisDevice.setOutput(DIGITAL_OUT_1, HIGH);
        fail += !client.publish((device_topic + "/out_1").c_str(), "1", true);
    }
    analogValue = thisDevice.getInput(ANALOG_IN_2);
    fail += !client.publish((device_topic + "/sensor_2").c_str(), String(analogValue).c_str(), true);
    if ((analogValue >= thisDevice.getOutputTrigger[2]) && (thisDevice.getOutput(2) == false)) {
        thisDevice.setOutput(DIGITAL_OUT_2, HIGH);
        fail += !client.publish((device_topic + "/out_2").c_str(), "1", true);
    }
    analogValue = thisDevice.getInput(ANALOG_IN_3);
    fail += !client.publish((device_topic + "/sensor_3").c_str(), String(analogValue).c_str(), true);
    if ((analogValue >= thisDevice.getOutputTrigger[3]) && (thisDevice.getOutput(3) == false)) {
        thisDevice.setOutput(DIGITAL_OUT_3, HIGH);
        fail += !client.publish((device_topic + "/out_3").c_str(), "1", true);
    }



	if (fail) {
		serialDebug("failed.");
		FAULT = 1;
	}
	else {
		serialDebug("ok!");
	}
	digitalWrite(BUILTIN_LED, LOW);
}

void callback(char* in_topic, byte* in_payload, unsigned int length) {
	
    String request = PtrToString(in_payload, length);
    String topic = String(in_topic);

    serialDebug("Last Message on Topic [" + topic + "] :: Payload [" + request + "]");

	if (request == "") return;



    if (topic.compareTo(device_topic + "/outputs/out_0") && textToBool(request)) {
        thisDevice.setOutput(0, HIGH);
    }
    if (topic.compareTo(device_topic + "/outputs/out_1") && textToBool(request)) {
        thisDevice.setOutput(1, HIGH);
    }
    if (topic.compareTo(device_topic + "/outputs/out_2") && textToBool(request)) {
        thisDevice.setOutput(2, HIGH);
    }
    if (topic.compareTo(device_topic + "/outputs/out_3") && textToBool(request)) {
        thisDevice.setOutput(3, HIGH);
    }
    if (topic.compareTo(device_topic + "/config/reset") && textToBool(request)) {
        thisDevice.resetOutputs();
    }
}

bool textToBool(String text) {

	if (text.endsWith("false") || text.endsWith("0") || text.endsWith("FALSE") || text.endsWith("off")) {
		return false;
	}
	return true;
}



String PtrToString(uint8_t *str, unsigned int len) {
	String result;
	for (int i = 0; i < len; i++) {
		result += ((char)str[i]);
	}
	return result;
}

void mqtt_connect() {

	if (!WiFi.isConnected()) {
		return;
	}

	serialDebug("Attempting MQTT connection...");
	// Create a random client ID
	String clientId = "ESP32_";
	clientId += String(random(0xffff), HEX);
	serialDebug("client ID: " + clientId);
	bool success = false;

	device_topic = String(main_topic) + "/" + thisDevice.getLocation() + "/" + String(WiFi.macAddress());
	serialDebug("device_topic: " + device_topic);
	// Attempt to connect
	// boolean connect (clientID, username, password, willTopic, willQoS, willRetain, willMessage)

	success = client.connect(clientId.c_str(), mqtt_user, mqtt_password, (device_topic + "/fault").c_str(), 2, true, will_message);
	//success = client.connect(clientId.c_str(), mqtt_user, mqtt_password);

	if (success) {
		serialDebug("connection ok!");
		subscribe();
	}
	else {
		Serial.print("connection fail, code: ");
		print_mqtt_reason(client.state());

	}

}

void setupWifi() {

	Serial.print("Connecting to: ");
	serialDebug(ssid);
	WiFi.begin(ssid, psk);


	if (!WiFi.isConnected()) {	//attempt #1
		for (size_t attempt = 0; (WiFi.status() != WL_CONNECTED && attempt < 10); attempt++) {
			delay(1000);
			Serial.print(".");
		}
	}

	if (!WiFi.isConnected()) {	//attempt #2
		ESP.restart();
		/*
		Serial.println("Failed.");
		Serial.println("Attempting WPS connection..");
		WiFi.beginWPSConfig();
		for (size_t attempt = 0; (WiFi.status() != WL_CONNECTED && attempt < 10); attempt++) {
		delay(4000);
		Serial.print(".");
		if (Serial.read() == 'c')
		break;
		}*/
	}

	//Serial.println("");
	serialDebug("WiFi connected!");
	Serial.print("IP address: ");
	serialDebug(WiFi.localIP());
	Serial.print("Signal: ");
	serialDebug(WiFi.RSSI());
	Serial.print("MAC address: ");
	serialDebug(WiFi.macAddress());


}

template <typename T>
void serialDebug(T message) {
    Serial.println(millis() + "> " + String(message));
}


void setup() {

	Serial.begin(SERIAL_BAUDRATE);
	Serial.println();

    serialDebug("--Boot Startup--");
    serialDebug("Serial Initialized @ 115200");

    serialDebug("Boot reason: ");
    serialDebug("CPU0: ");	print_reset_reason(rtc_get_reset_reason(0));
    serialDebug("CPU1: ");	print_reset_reason(rtc_get_reset_reason(1));
	serialDebug("-------------");

	Serial.println(ESP.getChipRevision());
	serialDebug(ESP.getSdkVersion());
	serialDebug(ESP.getCpuFreqMHz());
	serialDebug(ESP.getCycleCount());
	serialDebug("-------------");

	serialDebug(ESP.getHeapSize());
	serialDebug(ESP.getFreeHeap());
	serialDebug(ESP.getFreePsram());
	serialDebug(ESP.getFlashChipMode());
	serialDebug(ESP.getFlashChipSize());
	serialDebug(ESP.getFlashChipSpeed());
	serialDebug("-------------");

	serialDebug("Initializing GPIOs..");

	pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
	pinMode(DIGITAL_OUT_0, OUTPUT);
	pinMode(DIGITAL_OUT_1, OUTPUT);
	pinMode(DIGITAL_OUT_2, OUTPUT);
	pinMode(DIGITAL_OUT_3, OUTPUT);

	digitalWrite(BUILTIN_LED, LOW);  // active low
	digitalWrite(DIGITAL_OUT_0, HIGH);
	digitalWrite(DIGITAL_OUT_1, HIGH);
	digitalWrite(DIGITAL_OUT_2, HIGH);
	digitalWrite(DIGITAL_OUT_3, LOW);

	serialDebug("Initializing MQTT Server..");
	client.setServer(mqtt_server, mqtt_port);
	serialDebug("mqtt_server: " + String(mqtt_server) + ":" + String(mqtt_port));
	client.setCallback(callback);

	serialDebug("initializing scheduler..");
	runner.init();

	serialDebug("Adding tasks..");
	runner.addTask(t1);
	runner.addTask(t2);
	runner.addTask(t3);
	runner.addTask(t4);
	serialDebug("Enabling tasks..");
	delay(5000);
	t1.enable();
	t2.enable();
	t3.enable();
	t4.disable();
	serialDebug("--Boot Complete--");
	serialDebug("");
}


void loop() {

	client.loop();
	runner.execute();
	yield();
}
