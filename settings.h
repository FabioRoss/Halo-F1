#define SETTINGS_VERSION 1

struct SavedSettings {
    uint8_t  version;
    uint8_t  brightness;
    uint8_t  night_brightness;
    bool     nightModeActive;
    uint8_t  nightStart_h;
    uint8_t  nightStart_m;
    uint8_t  nightStop_h;
    uint8_t  nightStop_m;
    bool     noSpoilerModeActive;
    bool     timezoneOverrideActive;
    int32_t  UTCoffsetHours;
    uint8_t  languageIndex;
};

void saveSettings() {
    SavedSettings s{};
    s.version              = SETTINGS_VERSION;
    s.brightness           = brightness;
    s.night_brightness     = night_brightness;
    s.nightModeActive      = nightModeActive;
    s.nightStart_h         = nightModeTimes.start_hours;
    s.nightStart_m         = nightModeTimes.start_minutes;
    s.nightStop_h          = nightModeTimes.stop_hours;
    s.nightStop_m          = nightModeTimes.stop_minutes;
    s.noSpoilerModeActive  = noSpoilerModeActive;
    s.timezoneOverrideActive = timezoneOverrideActive;
    s.UTCoffsetHours       = UTCoffsetHours;
    s.languageIndex        = 0;
    for (size_t i = 0; i < languageCount; i++) {
        if (languages[i].strings == localized_text) { s.languageIndex = i; break; }
    }

    Serial.println("[Preferences] Saving settings to flash...");

    if (!preferences.begin("halo", false)) {
        Serial.println("[Preferences] ERROR: Failed to open NVS for writing, settings not saved.");
        return;
    }

    size_t written = preferences.putBytes("cfg", &s, sizeof(s));
    if (written != sizeof(s)) {
        Serial.println("[Preferences] ERROR: Failed to write all settings to flash, settings may be incomplete.");
        preferences.end();
        return;
    }

    preferences.end();
    Serial.println("[Preferences] Settings saved.");
}

void loadSettings() {
    if (!preferences.begin("halo", true)) {
        Serial.println("[Preferences] ERROR: Failed to open NVS for reading, using defaults.");
        return;
    }

    size_t len = preferences.getBytesLength("cfg");
    size_t read = 0;
    SavedSettings s{};

    if (len == sizeof(SavedSettings)) {
        read = preferences.getBytes("cfg", &s, sizeof(s));
    }
    preferences.end();

    if (len != sizeof(SavedSettings) || read != sizeof(SavedSettings) || s.version != SETTINGS_VERSION) {
        Serial.println("[Preferences] No valid saved settings found, using defaults.");
        return;
    }

    brightness             = s.brightness;
    night_brightness       = s.night_brightness;
    nightModeActive        = s.nightModeActive;
    nightModeTimes.start_hours   = s.nightStart_h;
    nightModeTimes.start_minutes = s.nightStart_m;
    nightModeTimes.stop_hours    = s.nightStop_h;
    nightModeTimes.stop_minutes  = s.nightStop_m;
    noSpoilerModeActive    = s.noSpoilerModeActive;
    timezoneOverrideActive = s.timezoneOverrideActive;
    UTCoffsetHours         = s.UTCoffsetHours;
    UTCoffset              = (long)UTCoffsetHours * 3600;

    if (s.languageIndex < languageCount)
        localized_text = languages[s.languageIndex].strings;

    Serial.println("[Preferences] Settings loaded from flash.");
}