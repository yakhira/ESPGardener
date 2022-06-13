#ifndef WIFI_H_
#define WIFI_H_

#include "utils/utils.h"

class ESPWiFi: public ESPUtils {
    private:
        static const String defaultWifiPassword;
        static const String configFile;
        static String espChipName;
        static JSONVar wifiConfig;

        char *hostname;
        bool isWebServerRunning;
        int resetCount;

    public:
        ESPWiFi(String chipName);
        ~ESPWiFi();

        static void handleMain();
        static void handleSave();

        void wifiConnect();
        void stateCheck();
        void loadConfig();

        static String otaUpdateUrl;
        static String dataUrl;

        t_httpUpdate_return updateSketch(int sketch_version);
};

#endif