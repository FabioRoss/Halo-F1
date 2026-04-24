#define SETTINGS_VERSION 3

struct SavedSettingsV1 {
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

struct SavedSettingsV2 {
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
    uint8_t  newsFeedIndex;
};

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
    uint8_t  newsFeedIndex;
    bool     newsPulseEnabled;
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
    s.newsFeedIndex        = selectedNewsFeed;
    s.newsPulseEnabled     = newsPulseEnabled;

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

    if (len == sizeof(SavedSettings) && read == sizeof(SavedSettings) && s.version == SETTINGS_VERSION) {
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

        if (s.newsFeedIndex < NEWS_FEED_COUNT)
            selectedNewsFeed = s.newsFeedIndex;
        newsPulseEnabled = s.newsPulseEnabled;

        Serial.println("[Preferences] Settings loaded from flash.");
        return;
    }

    if (len == sizeof(SavedSettingsV2)) {
        SavedSettingsV2 legacyV2{};
        if (preferences.begin("halo", true)) {
            size_t legacyRead = preferences.getBytes("cfg", &legacyV2, sizeof(legacyV2));
            preferences.end();
            if (legacyRead == sizeof(legacyV2) && legacyV2.version == 2) {
                brightness             = legacyV2.brightness;
                night_brightness       = legacyV2.night_brightness;
                nightModeActive        = legacyV2.nightModeActive;
                nightModeTimes.start_hours   = legacyV2.nightStart_h;
                nightModeTimes.start_minutes = legacyV2.nightStart_m;
                nightModeTimes.stop_hours    = legacyV2.nightStop_h;
                nightModeTimes.stop_minutes  = legacyV2.nightStop_m;
                noSpoilerModeActive    = legacyV2.noSpoilerModeActive;
                timezoneOverrideActive = legacyV2.timezoneOverrideActive;
                UTCoffsetHours         = legacyV2.UTCoffsetHours;
                UTCoffset              = (long)UTCoffsetHours * 3600;
                newsPulseEnabled       = true;

                if (legacyV2.languageIndex < languageCount)
                    localized_text = languages[legacyV2.languageIndex].strings;
                if (legacyV2.newsFeedIndex < NEWS_FEED_COUNT)
                    selectedNewsFeed = legacyV2.newsFeedIndex;

                Serial.println("[Preferences] Loaded legacy v2 settings from flash.");
                return;
            }
        }
    }

    if (len == sizeof(SavedSettingsV1)) {
        SavedSettingsV1 legacy{};
        if (preferences.begin("halo", true)) {
            size_t legacyRead = preferences.getBytes("cfg", &legacy, sizeof(legacy));
            preferences.end();
            if (legacyRead == sizeof(legacy) && legacy.version == 1) {
                brightness             = legacy.brightness;
                night_brightness       = legacy.night_brightness;
                nightModeActive        = legacy.nightModeActive;
                nightModeTimes.start_hours   = legacy.nightStart_h;
                nightModeTimes.start_minutes = legacy.nightStart_m;
                nightModeTimes.stop_hours    = legacy.nightStop_h;
                nightModeTimes.stop_minutes  = legacy.nightStop_m;
                noSpoilerModeActive    = legacy.noSpoilerModeActive;
                timezoneOverrideActive = legacy.timezoneOverrideActive;
                UTCoffsetHours         = legacy.UTCoffsetHours;
                UTCoffset              = (long)UTCoffsetHours * 3600;
                selectedNewsFeed       = 0;

                if (legacy.languageIndex < languageCount)
                    localized_text = languages[legacy.languageIndex].strings;

                Serial.println("[Preferences] Loaded legacy v1 settings from flash.");
                return;
            }
        }
    }

    Serial.println("[Preferences] No valid saved settings found, using defaults.");
}