#include "setup/wifi.h"
#include <ESPAsyncWebServer.h>

// -------- DEFAULT SKETCH PARAMETERS --------
const int SKETCH_VERSION = 50;
const int WATER_PUMPS[] = {12, 4};
const long unsigned int DEFAULT_DURATION = 300;

unsigned long WATER_PUMPS_DURATION[] = {0, 0};

int systemAction = 0;
String lastStatus;

ESPWiFi espwifi("ESP12-F");
AsyncWebServer gardener(80);

void update_static_content() {
	String data;
	String htmlFiles[] = {
		"/html/index.html",
		"/html/api.html",
		"/html/system.html",
		"/html/css/w3.css"
	};

	for (unsigned int i = 0; i < sizeof(htmlFiles)/sizeof(String); i++){
		if(espwifi.getHTTPData(espwifi.dataUrl + htmlFiles[i], data)) {
			if (!data.isEmpty()) {
				espwifi.saveFile(htmlFiles[i], data);
				lastStatus += " " + htmlFiles[i];
			}
		}
	}
}

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
		systemAction = 3;
		lastStatus = "Restarting ESP...";
		request->send(200, "application/json", "{\"status\": \"" + lastStatus + "\"}"); 
	});

	gardener.on("/api/update/firmware", HTTP_GET, [](AsyncWebServerRequest *request){
		systemAction = 2;
		lastStatus = "Updating firmware...";
		request->send(200, "application/json", "{\"status\": \"" + lastStatus + "\"}"); 
	});

	gardener.on("/api/update/content", HTTP_GET, [](AsyncWebServerRequest *request){
		systemAction = 1;
		lastStatus = "Updating html files...";
		request->send(200, "application/json", "{\"status\": \"" + lastStatus + "\"}"); 
	});

	gardener.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request){ 
		String version;
		String updateUrl;
		String dataUrl;

		espwifi.removeString(espwifi.otaUpdateUrl, "//", "@", updateUrl);
		espwifi.removeString(espwifi.dataUrl, "//", "@", dataUrl);

		espwifi.readFile("/version.txt", version);
		version.trim();

		request->send(
			200, 
			"application/json", 
			"{\"last_status\": \"" + lastStatus + "\", \"version\": \"" + version + "\", \"data_url\": \"" + dataUrl + "\", \"update_url\": \"" + updateUrl + "\"}"
		); 
	});

	gardener.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *request){ 
		request->send(LittleFS, "/logs.txt", "text/plain"); 
	});

	gardener.begin();
}

void setup()
{
	Serial.begin(74880);

	for (unsigned int i = 0; i < sizeof(WATER_PUMPS)/sizeof(int); i ++) {
		pinMode(WATER_PUMPS[i], OUTPUT);
		digitalWrite(WATER_PUMPS[i], LOW);
	}

	espwifi.wifiConnect();
	if (WiFi.getMode() == WIFI_STA)
	{
		espwifi.updateSketch(SKETCH_VERSION);
		espwifi.saveFile("/version.txt", String(SKETCH_VERSION));
		lastStatus = "Boot";
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
	for (unsigned int i = 0; i < sizeof(WATER_PUMPS)/sizeof(int); i++) {
		if(millis() >= WATER_PUMPS_DURATION[i]) {
			digitalWrite(WATER_PUMPS[i], LOW);

			if (WATER_PUMPS_DURATION[i] != 0) {
				espwifi.appendFile(
					"/logs.txt", 
					"0;" + String(i + 1) + ";" + ";" + "Stopped"
				);
			}
			WATER_PUMPS_DURATION[i] = 0;
		}
	}
	switch(systemAction) {
		case 1:  
			update_static_content();
			systemAction = 0;
			break;
		case 2:
			espwifi.updateSketch(SKETCH_VERSION);
			systemAction = 0;
			break;
		case 3:
			delay(3000);
			ESP.reset();
			systemAction = 0;
			break; 
	}
	espwifi.stateCheck();
}