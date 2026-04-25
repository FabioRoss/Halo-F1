// ============================================================
//  weather.h  –  Race-session weather via Open-Meteo (free, no key)
//
//  Include AFTER the global struct definitions in Halo-F1.ino
//  and BEFORE ui.h and wifi_handler.h.
//
//  Public surface used by ui.h:
//    session_weather[]   – per-session data array (indexed like next_race.sessions[])
//    weather_fetched     – true once at least one successful fetch happened
//    getWeatherIcon      – returns the glyph for the WMO code
//    getWeatherColor     – returns an lv_color_t matching the condition
//
//  Public surface used by wifi_handler.h:
//    fetchWeatherForRace(NextRaceInfo&)  – single HTTP call, fills session_weather[]
// ============================================================

#pragma once

// ── Per-session weather data ────────────────────────────────────────────────
struct SessionWeather {
    uint8_t  wmo_code;  // WMO weather interpretation code (0 = unknown)
    int8_t   temp_c;    // Temperature in °C at session start hour
    bool     valid;     // true = data was successfully fetched for this slot
};

// Indexed 1-to-1 with next_race.sessions[] (max 10 slots)
static SessionWeather session_weather[10];
static bool           weather_fetched      = false;
static unsigned long  last_weather_fetch_ms = 0;

// How often we re-fetch (1 h in normal running; the f1_api_timer fires every hour
// and calls fetchWeatherForRace, which guards with this value so the real cadence
// is "once per hour" without an extra timer)
#define WEATHER_REFRESH_MS (3600000UL)

// ── Circuit coordinate lookup ───────────────────────────────────────────────
// Coordinates come directly from the Jolpi/Ergast API (Circuit.Location.lat /
// .long), stored in NextRaceInfo.lat / .lon after getNextRaceInfo() runs.
// No hardcoded table needed.

// ── WMO weather code helpers ────────────────────────────────────────────────
// Open-Meteo uses WMO Weather Interpretation Codes (WW):
//   0        → clear sky
//   1-3      → mainly/partly cloudy
//   45,48    → fog
//   51-57    → drizzle
//   61-67    → rain
//   71-77    → snow
//   80-82    → rain showers
//   85-86    → snow showers
//   95,96,99 → thunderstorm

// Returns the UTF-8 FontAwesome glyph string for a WMO weather code.
// Symbols are defined in Halo-F1.ino and rendered with weather_icons_16.
const char* getWeatherIcon(uint8_t code) {
    if (code == 0)        return WX_SYMBOL_SUN;
    if (code <= 2)        return WX_SYMBOL_CLOUD_SUN;
    if (code <= 3)        return WX_SYMBOL_CLOUD;
    if (code <= 48)       return WX_SYMBOL_SMOG;
    if (code <= 57)       return WX_SYMBOL_DRIZZLE;
    if (code <= 67)       return WX_SYMBOL_RAIN;
    if (code <= 77)       return WX_SYMBOL_SNOW;
    if (code <= 82)       return WX_SYMBOL_RAIN;
    if (code <= 86)       return WX_SYMBOL_SNOW;
    return                       WX_SYMBOL_STORM;  // 95-99 thunderstorm
}

lv_color_t getWeatherColor(uint8_t code) {
    if (code == 0)        return lv_color_hex(0xFFCC00); // clear – yellow
    if (code <= 3)        return lv_color_hex(0xAAAAAA); // cloudy – gray
    if (code <= 48)       return lv_color_hex(0x888888); // fog – dark gray
    if (code <= 57)       return lv_color_hex(0x88BBFF); // drizzle – light blue
    if (code <= 67)       return lv_color_hex(0x4488FF); // rain – blue
    if (code <= 77)       return lv_color_hex(0xCCEEFF); // snow – ice blue
    if (code <= 82)       return lv_color_hex(0x4488FF); // showers – blue
    if (code <= 86)       return lv_color_hex(0xCCEEFF); // snow showers – ice blue
    return                       lv_color_hex(0xFF8800); // storm – orange
}

// ── Core fetch function ─────────────────────────────────────────────────────
// Makes ONE HTTP request to Open-Meteo covering the full date range of the
// race weekend, then matches each session to its hour-slot in the response.
// Memory notes:
//   – The JSON filter limits what ArduinoJson allocates to the three hourly
//     arrays we actually need (time[], weather_code[], temperature_2m[]).
//   – JsonDocument is heap-allocated and freed when it goes out of scope,
//     so there is no persistent heap cost between calls.
//   – The session_weather[] array is a flat 30-byte global.
bool fetchWeatherForRace(NextRaceInfo& race) {
    if (race.sessionCount <= 0) return false;

    // ── Throttle: don't hit the API more than once per WEATHER_REFRESH_MS ──
    unsigned long now_ms = millis();
    if (weather_fetched &&
        (now_ms - last_weather_fetch_ms) < WEATHER_REFRESH_MS &&
        now_ms > last_weather_fetch_ms) {
        // Still fresh — nothing to do
        return true;
    }

    // ── Validate coordinates populated by getNextRaceInfo() ────────────────
    if (race.lat == 0.0f && race.lon == 0.0f) {
        Serial.println("[Weather] Circuit coordinates not yet available, skipping.");
        return false;
    }
    float lat = race.lat;
    float lon = race.lon;

    // ── Build URL ───────────────────────────────────────────────────────────
    // date range: first session → last session
    String startDate = race.sessions[0].date;
    String endDate   = race.sessions[race.sessionCount - 1].date;

    char url[300];
    snprintf(url, sizeof(url),
        "https://api.open-meteo.com/v1/forecast"
        "?latitude=%.4f&longitude=%.4f"
        "&hourly=weather_code,temperature_2m"
        "&timezone=UTC"
        "&start_date=%s&end_date=%s",
        lat, lon,
        startDate.c_str(),
        endDate.c_str());

    Serial.printf("[Weather] Fetching: %s\n", url);

    // ── HTTP request ────────────────────────────────────────────────────────
    WiFiClientSecure secureClient;
    secureClient.setInsecure();   // same pattern as getLastSessionResults()

    HTTPClient http;
    http.begin(secureClient, url);
    http.setTimeout(12000);

    int httpCode = http.GET();
    if (httpCode != 200) {
        Serial.printf("[Weather] HTTP error: %d — %s\n",
                      httpCode, http.getString().c_str());
        http.end();
        return false;
    }

    // ── Read full payload then parse ────────────────────────────────────────
    // Open-Meteo's weekend response is ~5 KB — well within ESP32 heap.
    // Parsing from a String is more reliable than streaming with a filter,
    // which can silently discard content if the filter doc is under-sized.
    String payload = http.getString();
    http.end();

    Serial.printf("[Weather] Payload length: %d\n", payload.length());

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err) {
        Serial.printf("[Weather] JSON parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray times  = doc["hourly"]["time"].as<JsonArray>();
    JsonArray codes  = doc["hourly"]["weather_code"].as<JsonArray>();
    JsonArray temps  = doc["hourly"]["temperature_2m"].as<JsonArray>();

    if (times.isNull() || codes.isNull() || temps.isNull()) {
        Serial.printf("[Weather] Missing hourly arrays. Keys present: %s\n",
                      payload.substring(0, 120).c_str());
        return false;
    }

    // ── Reset all slots then fill matched ones ──────────────────────────────
    for (int s = 0; s < 10; s++) {
        session_weather[s] = { 0, 0, false };
    }

    // Pre-build a flat array of time strings + indices once for O(n) matching
    // instead of searching the JsonArray on every session (O(n*m)).
    // We cap at 200 hourly entries (covers an 8-day window comfortably).
    const int MAX_HOURS = 200;
    static char hour_strs[MAX_HOURS][14]; // "YYYY-MM-DDTHH"  = 13 chars + NUL
    int total_hours = 0;

    for (JsonVariant t : times) {
        if (total_hours >= MAX_HOURS) break;
        // Each element looks like "2025-03-28T14:00" – keep only first 13 chars
        const char* raw = t.as<const char*>();
        if (!raw) break;
        strncpy(hour_strs[total_hours], raw, 13);
        hour_strs[total_hours][13] = '\0';
        total_hours++;
    }

    // Match sessions to hour slots
    for (int s = 0; s < race.sessionCount && s < 10; s++) {
        // Build the target "YYYY-MM-DDTHH" key from the session's UTC date+time
        String sessionTime = race.sessions[s].time;
        if (sessionTime.endsWith("Z")) {
            sessionTime.remove(sessionTime.length() - 1);
        }
        // sessionTime is now "HH:MM:SS"; we only need "HH"
        String target = race.sessions[s].date + "T" + sessionTime.substring(0, 2);
        const char* target_cstr = target.c_str();

        for (int h = 0; h < total_hours; h++) {
            if (strcmp(hour_strs[h], target_cstr) == 0) {
                session_weather[s].wmo_code = (uint8_t)codes[h].as<int>();
                session_weather[s].temp_c   = (int8_t) temps[h].as<float>();
                session_weather[s].valid    = true;
                Serial.printf("[Weather] %s → %s  WMO=%d  %d°C\n",
                    race.sessions[s].name.c_str(),
                    target_cstr,
                    session_weather[s].wmo_code,
                    (int)session_weather[s].temp_c);
                break;
            }
        }

        if (!session_weather[s].valid) {
            Serial.printf("[Weather] No match for session %s at %s\n",
                race.sessions[s].name.c_str(), target_cstr);
        }
    }

    weather_fetched       = true;
    last_weather_fetch_ms = millis();
    Serial.println("[Weather] Fetch complete.");
    return true;
}

// ── Called by update_f1_api (wifi_handler.h) – see the comment there ────────
// Exposed as a named function so it can also be called on demand (e.g. after
// a manual clock refresh) without additional timer overhead.
void update_weather(lv_timer_t* /*timer*/) {
    fetchWeatherForRace(next_race);
}
