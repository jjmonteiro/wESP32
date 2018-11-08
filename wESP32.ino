/*
Name:		wESP32.ino
Created:	08-Nov-18 12:44:00
Author:		Joaquim Monteiro
*/

#include <EEPROM.h>
#include <TaskScheduler.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <rom/rtc.h>
#include "error_lib.h"

//**** DEFINE CONSTANTS 
#define SLEEP_PERIOD	30e6	//20 seconds
#define SHUTDOWN_TIMEOUT 500
#define TASK_RUN_TWICE 2
#define BUILTIN_LED		2
#define SERIAL_BAUDRATE	115200
#define FIRE_THRESHOLD 3000

//**** PORTS SETUP
#define ANALOG_IN_0 36
#define ANALOG_IN_1 39
#define ANALOG_IN_2 34
#define ANALOG_IN_3 35
#define ANALOG_IN_4 32
#define ANALOG_IN_5 33

#define DIGITAL_OUT_0 14	//r led
#define DIGITAL_OUT_1 27	//y led
#define DIGITAL_OUT_2 26
#define DIGITAL_OUT_3 25	//buzzer

//**** DEVICE INFORMATION
const char* device_type = "PH850";
const char* device_location = "ZONE_1";
String device_topic;
bool ALARM, FAULT, BUZZER, RESET;
int an_in_0, an_in_1, an_in_2;

//**** NETWORK CREDENTIALS
const char* ssid = "(-_-)";
const char* psk = "monteiro";

//**** SERVER INFO [cloudmqtt.com] ****
const char* mqtt_server = "m20.cloudmqtt.com";
const char* mqtt_user = "lgnnlude";
const char* mqtt_password = "iWg6ghr1pMLO";
const int   mqtt_port = 17283;
const char* will_message = "CONNECTION_LOST";
const char* main_topic = "MAIN";

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

Scheduler runner;
//ADC_MODE(ADC_VCC);

//***************************************************//

void t1Callback() {
	Serial.print("t1: ");
	Serial.println(millis());

	if (WiFi.status() != WL_CONNECTED) {
		setupWifi();
	}

	if (!client.connected()) {
		mqtt_connect();
	}

}

void t2Callback() {
	Serial.print("t2: ");
	Serial.println(millis());

	an_in_0 = analogRead(ANALOG_IN_0);
	an_in_1 = analogRead(ANALOG_IN_1);
	an_in_2 = analogRead(ANALOG_IN_2);

	if (an_in_0 > FIRE_THRESHOLD) {
		ALARM = 1;
		BUZZER = 1;
	}

	if (client.connected()) {
		publish();
	}

}

void t3Callback() {
	Serial.print("t3: ");
	Serial.println(millis());

}

void t4Callback() {
	Serial.print("t4: ");
	Serial.println(millis());


	if (t4.isLastIteration()) {
		publish();
		terminateAll();
		ESP.deepSleep(SLEEP_PERIOD);
	}
}

void terminateAll() {
	int timeout = 0;

	client.disconnect();
	Serial.println("client.disconnect()");
	runner.disableAll();
	Serial.println("runner.disableAll()");
	WiFi.disconnect();
	Serial.println("WiFi.disconnect()");
	while (WiFi.isConnected() || client.connected()) {
		yield();
		timeout++;
		if (timeout == SHUTDOWN_TIMEOUT) {
			return;
		}
	}
	Serial.println(String(timeout) + ": all processes terminated.");
}

void subscribe() {

	if (!client.connected()) {
		return;
	}

	int fail = 0;
	Serial.print("Subscribing topics..");

	fail += !client.subscribe((device_topic + "/request").c_str(), 1);
	fail += !client.subscribe((device_topic + "/alarm").c_str(), 1);
	fail += !client.subscribe((device_topic + "/fault").c_str(), 1);
	fail += !client.subscribe((device_topic + "/out_2").c_str(), 1);
	fail += !client.subscribe((device_topic + "/buzzer").c_str(), 1);

	if (fail) {
		Serial.println("failed.");
	}
	else {
		Serial.println("ok!");
	}
}

void publish() {

	if (!client.connected()) {
		return;
	}
	digitalWrite(BUILTIN_LED, HIGH);
	String payload;
	int fail = 0;
	Serial.print("Publishing messages..");


	payload = String(WiFi.RSSI());
	fail += !client.publish((device_topic + "/signal").c_str(), payload.c_str(), true);
	/*
	payload = String((float)ESP.getFreeHeap() / 1024);
	fail += !client.publish((device_topic + "/memory").c_str(), payload.c_str(), true);*/

	payload = String(an_in_0);
	fail += !client.publish((device_topic + "/sensor_00").c_str(), payload.c_str(), true);

	payload = String(an_in_1);
	fail += !client.publish((device_topic + "/sensor_01").c_str(), payload.c_str(), true);

	payload = String(an_in_2);
	fail += !client.publish((device_topic + "/sensor_02").c_str(), payload.c_str(), true);


	if (ALARM) {
		digitalWrite(DIGITAL_OUT_0, LOW);
		fail += !client.publish((device_topic + "/alarm").c_str(), String(ALARM).c_str(), true);
	}
	if (FAULT) {
		digitalWrite(DIGITAL_OUT_1, LOW);
		fail += !client.publish((device_topic + "/fault").c_str(), String(FAULT).c_str(), true);
	}
	if (BUZZER) {
		digitalWrite(DIGITAL_OUT_2, HIGH);
		fail += !client.publish((device_topic + "/out_02").c_str(), String(BUZZER).c_str(), true);
	}
	if (BUZZER) {
		digitalWrite(DIGITAL_OUT_3, HIGH);
		fail += !client.publish((device_topic + "/buzzer").c_str(), String(BUZZER).c_str(), true);
	}
	if (RESET) {
		digitalWrite(DIGITAL_OUT_0, !LOW);
		digitalWrite(DIGITAL_OUT_1, !LOW);
		digitalWrite(DIGITAL_OUT_2, LOW);
		digitalWrite(DIGITAL_OUT_3, LOW);
		fail += !client.publish((device_topic + "/alarm").c_str(), "0", true);
		fail += !client.publish((device_topic + "/fault").c_str(), "0", true);
		fail += !client.publish((device_topic + "/out_02").c_str(), "0", true);
		fail += !client.publish((device_topic + "/buzzer").c_str(), "0", true);
		ALARM, FAULT, BUZZER, RESET = 0;
		delay(1000);
	}

	fail += !client.publish((device_topic + "/request").c_str(), "", true);	//clear last request

	if (fail) {
		Serial.println("failed.");
		FAULT = 1;
	}
	else {
		Serial.println("ok!");
	}
	digitalWrite(BUILTIN_LED, LOW);
}

void callback(char* in_topic, byte* in_payload, unsigned int length) {
	String request = PtrToString(in_payload, length);

	Serial.print("Last Message on Topic [");
	Serial.print(in_topic);
	Serial.print("] :: Payload [");
	Serial.print(request);
	Serial.println("]");

	if (request == "") {
		return;
	}

	if (String(in_topic).endsWith("request")) {
		Serial.println(cmdParser(request));
	}
	if (String(in_topic).endsWith("alarm")) {
		ALARM = textToBool(request);
	}
	if (String(in_topic).endsWith("fault")) {
		FAULT = textToBool(request);
	}
	if (String(in_topic).endsWith("out_2")) {
		RESET = textToBool(request);
	}
	if (String(in_topic).endsWith("buzzer")) {
		BUZZER = textToBool(request);
	}

}

bool textToBool(String text) {

	if (text.endsWith("false") || text.endsWith("0") || text.endsWith("FALSE") || text.endsWith("off")) {
		return false;
	}
	return true;
}

String cmdParser(String command) {

	String reply = "> Unknown command.";

	if (command.startsWith("bled")) {
		digitalWrite(BUILTIN_LED, textToBool(command));
		reply = command;
	}
	if (command.startsWith("out0")) {
		digitalWrite(DIGITAL_OUT_0, textToBool(command));
		reply = command;
	}
	if (command.startsWith("out1")) {
		digitalWrite(DIGITAL_OUT_1, textToBool(command));
		reply = command;
	}
	if (command.startsWith("out2")) {
		digitalWrite(DIGITAL_OUT_2, textToBool(command));
		reply = command;
	}
	if (command.startsWith("out3")) {
		digitalWrite(DIGITAL_OUT_3, textToBool(command));
		reply = command;
	}


	if (command == "ip_addr") {
		reply = WiFi.localIP().toString();
	}
	if (command == "sleep_off") {
		t4.disable();
		reply = "Deepsleep: OFF";
	}
	if (command == "sleep_on") {
		t4.enable();
		reply = "Deepsleep: ON";
	}
	if (command == "restart") {
		terminateAll();
		ESP.restart();
	}
	if (command == "reset") {
		RESET = 1;
	}

	publish();
	return (reply);
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

	Serial.println("Attempting MQTT connection...");
	// Create a random client ID
	String clientId = "ESP32_";
	clientId += String(random(0xffff), HEX);
	Serial.println("client ID: " + clientId);
	bool success = false;

	device_topic = String(main_topic) + "/" + device_location + "/" + String(WiFi.macAddress());
	Serial.println("device_topic: " + device_topic);
	// Attempt to connect
	// boolean connect (clientID, username, password, willTopic, willQoS, willRetain, willMessage)

	success = client.connect(clientId.c_str(), mqtt_user, mqtt_password, (device_topic + "/fault").c_str(), 2, true, will_message);
	//success = client.connect(clientId.c_str(), mqtt_user, mqtt_password);

	if (success) {
		Serial.println("connection ok!");
		subscribe();
	}
	else {
		Serial.print("connection fail, code: ");
		print_mqtt_reason(client.state());

	}

}

void setupWifi() {

	Serial.print("Connecting to: ");
	Serial.println(ssid);
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
	Serial.println("WiFi connected!");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	Serial.print("Signal: ");
	Serial.println(WiFi.RSSI());
	Serial.print("MAC address: ");
	Serial.println(WiFi.macAddress());


}



void setup() {

	Serial.begin(SERIAL_BAUDRATE);
	Serial.println();
	Serial.println("--Boot Startup--");
	Serial.println("Serial Initialized @ 115200");

	Serial.println("Boot reason: ");
	Serial.print("CPU0: ");	print_reset_reason(rtc_get_reset_reason(0));
	Serial.print("CPU1: ");	print_reset_reason(rtc_get_reset_reason(1));
	Serial.println("-------------");

	Serial.println(ESP.getChipRevision());
	Serial.println(ESP.getSdkVersion());
	Serial.println(ESP.getCpuFreqMHz());
	Serial.println(ESP.getCycleCount());
	Serial.println("-------------");

	Serial.println(ESP.getHeapSize());
	Serial.println(ESP.getFreeHeap());
	Serial.println(ESP.getFreePsram());
	Serial.println(ESP.getFlashChipMode());
	Serial.println(ESP.getFlashChipSize());
	Serial.println(ESP.getFlashChipSpeed());
	Serial.println("-------------");

	Serial.println("Initializing GPIOs..");
	ALARM, FAULT, BUZZER, RESET = 0;
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

	Serial.println("Initializing MQTT Server..");
	client.setServer(mqtt_server, mqtt_port);
	Serial.println("mqtt_server: " + String(mqtt_server) + ":" + String(mqtt_port));
	client.setCallback(callback);

	Serial.println("initializing scheduler..");
	runner.init();

	Serial.println("Adding tasks..");
	runner.addTask(t1);
	runner.addTask(t2);
	runner.addTask(t3);
	runner.addTask(t4);
	Serial.println("Enabling tasks..");
	delay(5000);
	t1.enable();
	t2.enable();
	t3.enable();
	t4.disable();
	Serial.println("--Boot Complete--");
	Serial.println();
}


void loop() {

	client.loop();
	runner.execute();
	yield();
}
