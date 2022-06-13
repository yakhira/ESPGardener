#include "setup/wifi.h"
#include <ESPAsyncWebServer.h>

// -------- DEFAULT SKETCH PARAMETERS --------
const int SKETCH_VERSION = 14;

const int WATER_PUMPS[] = {4, 12};
unsigned long WATER_PUMPS_DURATION[] = {0, 0};
const long unsigned int DEFAULT_DURATION = 300;

ESPWiFi espwifi("ESP12-F");
AsyncWebServer gardener(80);

void main_code() 
{
	gardener.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
		request->send(LittleFS, "/html/index.html", "text/html"); 
	});

	gardener.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){ 
		request->send(LittleFS, "/html/index.html", "text/html"); 
	});

	gardener.on("/api.html", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(LittleFS, "/html/api.html", "text/html");
	});

	gardener.on("/system.html", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(LittleFS, "/html/system.html", "text/html");
	});

	gardener.on("/css/w3.css", HTTP_GET, [](AsyncWebServerRequest *request){ 
		request->send(LittleFS, "/html/css/w3.css", "text/css"); 
	});

	gardener.on("/api/water", HTTP_POST, [](AsyncWebServerRequest *request){
		if (request->hasArg("pump") && request->hasArg("active")) {
			if (request->arg("pump") == "1" || request->arg("pump") == "2") {
				int pumpNumber = request->arg("pump").toInt() - 1;

				if (request->arg("active") == "true") {
					if (request->hasArg("duration")) {
						WATER_PUMPS_DURATION[pumpNumber] = millis() + request->arg("duration").toInt() * 1000;
					} else {
						WATER_PUMPS_DURATION[pumpNumber] = millis() + DEFAULT_DURATION * 1000;
					}
					digitalWrite(WATER_PUMPS[pumpNumber], HIGH);
					espwifi.appendFile("/logs.txt", "0;" + request->arg("pump") + ";" + request->arg("duration") + ";Started");

					request->send(200, "application/json", "{\"is_active\": \"true\"}");
				} else {
					digitalWrite(WATER_PUMPS[pumpNumber], LOW);
					espwifi.appendFile("/logs.txt", "0;" + request->arg("pump") + ";" + request->arg("duration") + ";Stoppped");

					WATER_PUMPS_DURATION[pumpNumber] = 0;
					request->send(200, "application/json", "{\"is_active\": \"false\"}");
				}
			} else {
				request->send(500, "application/json", "{\"status\": \"Wrong pump number\"}");
			}
		} else {
			request->send(500, "application/json", "{\"status\": \"pump or active parametr missing\"}");
		}
	});

	gardener.on("/api/water", HTTP_GET, [](AsyncWebServerRequest *request){
		if (request->hasArg("pump")) {
			if (request->arg("pump") == "1" || request->arg("pump") == "2") {
				int pumpNumber = request->arg("pump").toInt() - 1;

				if (WATER_PUMPS_DURATION[pumpNumber] != 0) {
					request->send(200, "application/json", "{\"is_active\": \"true\"}");
				} else {
					request->send(200, "application/json", "{\"is_active\": \"false\"}");
				}
			}
			else {
				request->send(200, "application/json", "{\"is_active\": \"false\"}");
			}
		} else {
			request->send(200, "application/json", "{\"is_active\": \"false\"}");
		} 
	});

	gardener.on("/api/restart", HTTP_GET, [](AsyncWebServerRequest *request){
		ESP.reset();
	});

	gardener.on("/api/version", HTTP_GET, [](AsyncWebServerRequest *request){ 
		request->send(LittleFS, "/version.txt", "text/plain"); 
	});

	gardener.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *request){ 
		request->send(LittleFS, "/logs.txt", "text/plain"); 
	});

	gardener.begin();
}
void setup()
{
	Serial.begin(74880);

	for (int i = 0; i < sizeof(WATER_PUMPS); i ++) {
		pinMode(WATER_PUMPS[i], OUTPUT);
		digitalWrite(WATER_PUMPS[i], LOW);
	}

	espwifi.wifiConnect();
	if (WiFi.getMode() == WIFI_STA)
	{
		espwifi.updateSketch(SKETCH_VERSION);
		espwifi.saveFile("/version.txt", String(SKETCH_VERSION));
		main_code();
	}
}

void loop()
{
	if (WiFi.getMode() == WIFI_STA)
	{
		if (WiFi.status() != WL_CONNECTED)
		{
			WiFi.reconnect();
		}
	}

	for (unsigned int i = 0; i < sizeof(WATER_PUMPS); i++) {
		if(millis() >= WATER_PUMPS_DURATION[i]) {
			digitalWrite(WATER_PUMPS[i], LOW);

			if (WATER_PUMPS_DURATION[i] != 0) {
				espwifi.appendFile(
					"/logs.txt", 
					"0;" + String(WATER_PUMPS[i]) + ";" + "0;" + ";Stoppped"
				);
			}
			WATER_PUMPS_DURATION[i] = 0;
		}
	}
	espwifi.stateCheck();
}