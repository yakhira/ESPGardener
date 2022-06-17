#include "setup/wifi.h"
#include <NTPClient.h>
#include <ESPAsyncWebServer.h>

// -------- DEFAULT SKETCH PARAMETERS --------
const int SKETCH_VERSION = 60;
//--------------------------------------------

const int WATER_PUMPS[] = {12, 4};
const long unsigned int DEFAULT_DURATION = 300;
const unsigned int TIME_OFFSET = 7200;
unsigned long WATER_PUMPS_DURATION[] = {0, 0};

int systemAction = 0;
String lastStatus;

ESPWiFi espwifi("ESP12-F");
WiFiUDP ntpUDP;
AsyncWebServer gardener(80);
NTPClient timeClient(ntpUDP);

String getFullFormattedTime() {
   time_t rawtime = timeClient.getEpochTime();
   struct tm * ti;
   ti = localtime (&rawtime);

   uint16_t year = ti->tm_year + 1900;
   String yearStr = String(year);

   uint8_t month = ti->tm_mon + 1;
   String monthStr = month < 10 ? "0" + String(month) : String(month);

   uint8_t day = ti->tm_mday;
   String dayStr = day < 10 ? "0" + String(day) : String(day);

   uint8_t hours = ti->tm_hour;
   String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

   uint8_t minutes = ti->tm_min;
   String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

   uint8_t seconds = ti->tm_sec;
   String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

   return yearStr + "-" + monthStr + "-" + dayStr + " " +
          hoursStr + ":" + minuteStr + ":" + secondStr;
}

void update_static_content() {
	String data;
	String htmlFiles[] = {
		"/html/index.html",
		"/html/api.html",
		"/html/system.html",
		"/html/css/w3.css"
	};

	for (unsigned int i = 0; i < sizeof(htmlFiles)/sizeof(String); i++){
		if(espwifi.downloadFile(espwifi.dataUrl + htmlFiles[i], htmlFiles[i])) {
			lastStatus += " " + htmlFiles[i];
		}
	}
}

void saveLogs(String pump, String duration, String action) {
	JSONVar data;

	int records = 0;

	espwifi.readFile("/logs.json", data);
	
	records = data.length();
	
	if (records > 30){
		JSONVar tempData;

		for (int i = 1; i < data.length(); i++){
			tempData[i-1] = data[i];
		}
		data = tempData;
	}

	data[records] = getFullFormattedTime() + ";" + pump + ";" + duration + ";" + action;
	espwifi.saveFile("/logs.json", data);
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
					saveLogs(request->arg("pump"), request->arg("duration"), "Started");

					request->send(200, "application/json", "{\"is_active\": \"true\"}");
				} else {
					digitalWrite(WATER_PUMPS[pumpNumber], LOW);
					saveLogs(request->arg("pump"), "", "Stopped");

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
		request->send(LittleFS, "/logs.json", "application/json");
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
		timeClient.begin();
		timeClient.setTimeOffset(TIME_OFFSET);
		espwifi.removeFile("/logs.json");

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
				saveLogs(String(i + 1), "", "Stopped");
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
	timeClient.update();
	espwifi.stateCheck();
}