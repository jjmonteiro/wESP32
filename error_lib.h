#pragma once

void print_reset_reason(RESET_REASON reason)
{
	switch (reason)
	{
	case 1: Serial.println("POWERON_RESET"); break;          /**<1, Vbat power on reset*/
	case 3: Serial.println("SW_RESET"); break;               /**<3, Software reset digital core*/
	case 4: Serial.println("OWDT_RESET"); break;             /**<4, Legacy watch dog reset digital core*/
	case 5: Serial.println("DEEPSLEEP_RESET"); break;        /**<5, Deep Sleep reset digital core*/
	case 6: Serial.println("SDIO_RESET"); break;             /**<6, Reset by SLC module, reset digital core*/
	case 7: Serial.println("TG0WDT_SYS_RESET"); break;       /**<7, Timer Group0 Watch dog reset digital core*/
	case 8: Serial.println("TG1WDT_SYS_RESET"); break;       /**<8, Timer Group1 Watch dog reset digital core*/
	case 9: Serial.println("RTCWDT_SYS_RESET"); break;       /**<9, RTC Watch dog Reset digital core*/
	case 10: Serial.println("INTRUSION_RESET"); break;       /**<10, Instrusion tested to reset CPU*/
	case 11: Serial.println("TGWDT_CPU_RESET"); break;       /**<11, Time Group reset CPU*/
	case 12: Serial.println("SW_CPU_RESET"); break;          /**<12, Software reset CPU*/
	case 13: Serial.println("RTCWDT_CPU_RESET"); break;      /**<13, RTC Watch dog Reset CPU*/
	case 14: Serial.println("EXT_CPU_RESET"); break;         /**<14, for APP CPU, reseted by PRO CPU*/
	case 15: Serial.println("RTCWDT_BROWN_OUT_RESET"); break;/**<15, Reset when the vdd voltage is not stable*/
	case 16: Serial.println("RTCWDT_RTC_RESET"); break;      /**<16, RTC Watch dog reset digital core and rtc module*/
	default: Serial.println("NO_MEAN");
	}
}

void print_mqtt_reason(int reason)
{
	switch (reason)
	{
	case -4: Serial.println("MQTT_CONNECTION_TIMEOUT"); break;		//the server didn't respond within the keepalive time
	case -3: Serial.println("MQTT_CONNECTION_LOST"); break;			//the network connection was broken
	case -2: Serial.println("MQTT_CONNECT_FAILED"); break;			//the network connection failed
	case -1: Serial.println("MQTT_DISCONNECTED"); break;			//the client is disconnected cleanly
	case 0: Serial.println("MQTT_CONNECTED"); break;				//the client is connected
	case 1: Serial.println("MQTT_CONNECT_BAD_PROTOCOL"); break;		//the server doesn't support the requested version of MQTT
	case 2: Serial.println("MQTT_CONNECT_BAD_CLIENT_ID"); break;	//the server rejected the client identifier
	case 3: Serial.println("MQTT_CONNECT_UNAVAILABLE"); break;		//the server was unable to accept the connection
	case 4: Serial.println("MQTT_CONNECT_BAD_CREDENTIALS"); break;  //the username / password were rejected
	case 5: Serial.println("MQTT_CONNECT_UNAUTHORIZED"); break;		//the client was not authorized to connect
	}
}
