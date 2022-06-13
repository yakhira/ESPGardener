#include <Arduino.h>
#include <Arduino_JSON.h>

#include "wifi.h"

JSONVar ESPWiFi::wifiConfig;
String ESPWiFi::espChipName;
String ESPWiFi::otaUpdateUrl;
String ESPWiFi::dataUrl;

const String ESPWiFi::defaultWifiPassword = "ESPp@$$w0rd!";
const String ESPWiFi::configFile = "/config/wifi_config.json";

AsyncWebServer server(80);

ESPWiFi::ESPWiFi(String chipName){
    isWebServerRunning = false;
    espChipName = chipName;
    resetCount = 0;
}

ESPWiFi::~ESPWiFi(){

}

void ESPWiFi::loadConfig(){
    readFile(configFile, wifiConfig);

    otaUpdateUrl = JSON.stringify(wifiConfig["ota_update_url"]);
    dataUrl = JSON.stringify(wifiConfig["data_url"]);

    otaUpdateUrl.replace("\"", "");
    dataUrl.replace("\"", "");
}

void ESPWiFi::wifiConnect(){
    IPAddress staticIP;
    IPAddress dns;
    IPAddress gateway;
    IPAddress subnet;

    String macAddress = WiFi.macAddress();
    macAddress.replace(":", "");
    String hostname =  "ESP-AP-" + macAddress;

    loadConfig();

    if (wifiConfig.hasOwnProperty("wifi_ssid") && wifiConfig.hasOwnProperty("wifi_password"))
    {
        WiFi.begin(wifiConfig["wifi_ssid"], wifiConfig["wifi_password"]);

        if (wifiConfig.hasOwnProperty("staticIP")){
            staticIP.fromString(wifiConfig["staticIP"]);
            dns.fromString(wifiConfig["dns"]);
            gateway.fromString(wifiConfig["gateway"]);
            subnet.fromString(wifiConfig["subnet"]);
            WiFi.config(staticIP, dns, gateway, subnet);
        }

        if (!wifiConfig.hasOwnProperty("hostname")) {
            wifiConfig["hostname"] = hostname;
        }
        else if (wifiConfig["hostname"].length() <= 0)
        {
            wifiConfig["hostname"] = hostname;
        }

        WiFi.mode(WIFI_STA);
        WiFi.hostname(JSON.stringify(wifiConfig["hostname"]));
        WiFi.setAutoReconnect(true);

        pinMode(0, INPUT_PULLUP);

        while (WiFi.status() != WL_CONNECTED){
            stateCheck();
            delay(500);
        }

        if (!wifiConfig.hasOwnProperty("staticIP")){
            wifiConfig["staticIP"] = WiFi.localIP().toString();
            wifiConfig["dns"] = WiFi.dnsIP().toString();
            wifiConfig["gateway"] = WiFi.gatewayIP().toString();
            wifiConfig["subnet"] = WiFi.subnetMask().toString();

            saveFile(configFile, wifiConfig);
        }
    }
    else {
        WiFi.softAP(hostname, defaultWifiPassword);

        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
			request->send(LittleFS, "/html/config.html", "text/html");
		});

        server.on("/wifi/save", HTTP_POST, [](AsyncWebServerRequest *request){
            if (request->hasArg("wifi_ssid") && request->hasArg("wifi_password")){
                ESPUtils esputils;
                ESPWiFi::wifiConfig["wifi_ssid"] = request->arg("wifi_ssid");
                ESPWiFi::wifiConfig["wifi_password"] = request->arg("wifi_password");
                ESPWiFi::wifiConfig["ota_update_url"] = request->arg("ota_update_url");
                ESPWiFi::wifiConfig["data_url"] = request->arg("data_url");
                ESPWiFi::wifiConfig["hostname"] = request->arg("hostname");

                esputils.saveFile(ESPWiFi::configFile,  ESPWiFi::wifiConfig);

                if (request->arg("restart_device") == "on") {
                    ESP.reset();
                } else {
                    request->send(200, "text/html", "Accepted. Please restart device.");
                }
            } else {
                request->send(400, "text/html", "Missing fields WIFI SSID or WIFI PASSWORD.");
            }
		});

        server.begin();

        isWebServerRunning = true;
    }
}

void ESPWiFi::stateCheck(){
    if (digitalRead(0) == LOW){
        if (resetCount > 500) {
            removeFile(configFile);
            resetCount = 0;
        } else {
            resetCount+=1;
        }
    } else {
        resetCount = 0;
    }
}

t_httpUpdate_return ESPWiFi::updateSketch(int sketch_version){
    return ESPUtils::updateSketch(
        otaUpdateUrl + "?chip=" + espChipName + "&mac=" + WiFi.macAddress() + "&version=" + sketch_version
    );
}