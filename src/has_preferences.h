#ifndef HAS_PREFERENCES_H
#define HAS_PREFERENCES_H

#include <Arduino.h>
#include <Preferences.h>
#include <nvs.h>

#include "util.h"

class HasPreferences {
   protected:
    char preferencesNamespace[NVS_KEY_NAME_MAX_SIZE] = {0};
    Preferences preferences;
    static constexpr const char* preferencesTag = "HasPrefs";
    bool preferencesReady = false;

    bool preferencesSetup(const char* ns, bool readOnly = false) {
        if (nullptr == ns) {
            ESP_LOGE(preferencesTag, "Namespace is null");
            return false;
        }
        if (strlen(ns) >= sizeof(preferencesNamespace)) {
            ESP_LOGE(preferencesTag, "Namespace too long");
            return false;
        }
        Util::copyString(preferencesNamespace, sizeof(preferencesNamespace), ns);
        if (!preferences.begin(preferencesNamespace, readOnly)) {
            ESP_LOGE(preferencesTag, "preferences.begin(\"%s\", %s) failed",
                     preferencesNamespace, readOnly ? "true" : "false");
            return false;
        }
        preferencesReady = true;
        return true;
    }

    void preferencesEnd() {
        if (!preferencesReady) return;
        preferences.end();
    }
};

#endif  // HAS_PREFERENCES_H