#include "utils.h"


ESPUtils::ESPUtils(){
	clientSecure.setInsecure();
}

ESPUtils::~ESPUtils(){

}

void ESPUtils::mountFS(){
    if (!fsMounted) {
        LittleFS.begin();
    }
}

void ESPUtils::removeFile(String filename) {
	mountFS();
	LittleFS.remove(filename);
}

void ESPUtils::saveFile(String filename, JSONVar data) {
	mountFS();

	File file = LittleFS.open(filename, "w");
	if (file) {
      file.println(data); 
      file.close();
    }
}

void ESPUtils::saveFile(String filename, String data) {
	mountFS();

	File file = LittleFS.open(filename, "w");
	if (file) {
      file.println(data); 
      file.close();
    }
}

void ESPUtils::appendFile(String filename, String data) {
	mountFS();

	File file = LittleFS.open(filename, "a");
	if (file) {
      file.println(data); 
      file.close();
    }
}

void ESPUtils::readFile(String filename, JSONVar &data) {
	mountFS();

	File file = LittleFS.open(filename, "r");
	while (file.available()) {
		data = JSON.parse(file.readStringUntil('\n'));
    }
	file.close();

}

void ESPUtils::readFile(String filename, String &data) {
	mountFS();

	File file = LittleFS.open(filename, "r");
	while (file.available()) {
		data = file.readStringUntil('\n');
    }
	file.close();
}

void ESPUtils::listDir(const char * dirname, uint8_t levels){
	mountFS();

    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = LittleFS.open(dirname, "r");
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.print(file.name());
            time_t t= file.getLastWrite();
            struct tm * tmstruct = localtime(&t);
            Serial.printf(
				"  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",
				(tmstruct->tm_year)+1900,
				(tmstruct->tm_mon)+1,
				tmstruct->tm_mday,tmstruct->tm_hour, 
				tmstruct->tm_min, tmstruct->tm_sec
			);

            if(levels){
                listDir(file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.print(file.size());
            time_t t= file.getLastWrite();
            struct tm * tmstruct = localtime(&t);
            Serial.printf(
				"  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",
				(tmstruct->tm_year)+1900,
				(tmstruct->tm_mon)+1,
				tmstruct->tm_mday,
				tmstruct->tm_hour,
				tmstruct->tm_min,
				tmstruct->tm_sec
			);
        }
        file = root.openNextFile();
    }
}

bool ESPUtils::getHTTPData(String url, String &result) {
	HTTPClient http;

	if (url.indexOf("https") >= 0) {
		http.begin(clientSecure, url);
	} else {
		http.begin(client, url);
	}

	int httpCode = http.GET();

	if (httpCode == HTTP_CODE_OK) {
		result = http.getString();
	}
	http.end();
	return (httpCode == HTTP_CODE_OK);
}

bool ESPUtils::getHTTPJsonData(String url, JSONVar &result) {
	HTTPClient http;

	if (url.indexOf("https") >= 0) {
		http.begin(clientSecure, url);
	} else {
		http.begin(client, url);
	}

	int httpCode = http.GET();

	if (httpCode == HTTP_CODE_OK) {
		result = JSON.parse(http.getString());

		if (JSON.typeof(result) == "undefined") {
			return false;
		} 
	}
	http.end();
	return (httpCode == HTTP_CODE_OK);
}

bool ESPUtils::sendHTTPJsonData(String url, JSONVar data) {
	HTTPClient http;

	if (url.indexOf("https") >= 0) {
		http.begin(clientSecure, url);
	} else {
		http.begin(client, url);
	}

	http.addHeader("Content-Type", "application/json");

	int httpCode = http.POST(JSON.stringify(data));
	http.end();

	return (httpCode == HTTP_CODE_OK);
}

void ESPUtils::removeString(String string, String from, String to, String &result) {
	int patternFrom = string.indexOf(from);
	int patternTo = string.indexOf(to);

	if (patternTo >= 0 && patternFrom >= 0) {
		string.remove(
		patternFrom + from.length(),
		patternTo - patternFrom - to.length()
		);
	}

	result = string;
}

t_httpUpdate_return ESPUtils::updateSketch(String url){
  	return ESPhttpUpdate.update(client, url);
}