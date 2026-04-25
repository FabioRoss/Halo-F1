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

static void applyLoadedSettingsCommon(
    uint8_t brightnessValue,
    uint8_t nightBrightnessValue,
    bool nightModeValue,
    uint8_t nightStartH,
    uint8_t nightStartM,
    uint8_t nightStopH,
    uint8_t nightStopM,
    bool noSpoilerValue,
    bool timezoneOverrideValue,
    int32_t utcOffsetHoursValue
) {
    brightness                   = brightnessValue;
    night_brightness             = nightBrightnessValue;
    nightModeActive              = nightModeValue;
    nightModeTimes.start_hours   = nightStartH;
    nightModeTimes.start_minutes = nightStartM;
    nightModeTimes.stop_hours    = nightStopH;
    nightModeTimes.stop_minutes  = nightStopM;
    noSpoilerModeActive          = noSpoilerValue;
    timezoneOverrideActive       = timezoneOverrideValue;
    UTCoffsetHours               = utcOffsetHoursValue;
    UTCoffset                    = (long)UTCoffsetHours * 3600;
}

static void applyLanguageAndFeed(uint8_t languageIndex, uint8_t newsFeedIndex) {
    if (languageIndex < languageCount)
        localized_text = languages[languageIndex].strings;
    if (newsFeedIndex < NEWS_FEED_COUNT)
        selectedNewsFeed = newsFeedIndex;
}

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
    const size_t maxSupportedSize = sizeof(SavedSettings);
    if (len == 0 || len > maxSupportedSize) {
        preferences.end();
        Serial.println("[Preferences] No valid saved settings found, using defaults.");
        return;
    }

    uint8_t raw[maxSupportedSize];
    memset(raw, 0, sizeof(raw));

    size_t read = preferences.getBytes("cfg", raw, len);
    preferences.end();
    if (read != len || len < 1) {
        Serial.println("[Preferences] No valid saved settings found, using defaults.");
        return;
    }

    uint8_t rawVersion = raw[0];

    if (rawVersion == SETTINGS_VERSION && len >= sizeof(SavedSettings)) {
        SavedSettings s{};
        memcpy(&s, raw, sizeof(SavedSettings));
        applyLoadedSettingsCommon(
            s.brightness, s.night_brightness, s.nightModeActive,
            s.nightStart_h, s.nightStart_m, s.nightStop_h, s.nightStop_m,
            s.noSpoilerModeActive, s.timezoneOverrideActive, s.UTCoffsetHours
        );
        applyLanguageAndFeed(s.languageIndex, s.newsFeedIndex);
        newsPulseEnabled = s.newsPulseEnabled;
        Serial.println("[Preferences] Settings loaded from flash.");
        return;
    }

    if (rawVersion == 2 && len >= sizeof(SavedSettingsV2)) {
        SavedSettingsV2 s{};
        memcpy(&s, raw, sizeof(SavedSettingsV2));
        applyLoadedSettingsCommon(
            s.brightness, s.night_brightness, s.nightModeActive,
            s.nightStart_h, s.nightStart_m, s.nightStop_h, s.nightStop_m,
            s.noSpoilerModeActive, s.timezoneOverrideActive, s.UTCoffsetHours
        );
        applyLanguageAndFeed(s.languageIndex, s.newsFeedIndex);
        newsPulseEnabled = true;
        Serial.println("[Preferences] Loaded legacy v2 settings from flash.");
        return;
    }

    if (rawVersion == 1 && len >= sizeof(SavedSettingsV1)) {
        SavedSettingsV1 s{};
        memcpy(&s, raw, sizeof(SavedSettingsV1));
        applyLoadedSettingsCommon(
            s.brightness, s.night_brightness, s.nightModeActive,
            s.nightStart_h, s.nightStart_m, s.nightStop_h, s.nightStop_m,
            s.noSpoilerModeActive, s.timezoneOverrideActive, s.UTCoffsetHours
        );
        applyLanguageAndFeed(s.languageIndex, 0);
        newsPulseEnabled = true;
        Serial.println("[Preferences] Loaded legacy v1 settings from flash.");
        return;
    }

    Serial.println("[Preferences] No valid saved settings found, using defaults.");
}