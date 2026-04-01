// forward declaration
String current_f1_champion = "4"; // Norris
void create_or_reload_race_sessions(bool force_reload = false);
void adjustBrightness(uint8_t brightness);
void create_or_reload_settings_ui();
void sendStatisticData(lv_timer_t *timer);

String getDeviceUUID() {
  uint64_t chipid = ESP.getEfuseMac();  // unique 48-bit ID from eFuse

  char uuid[37]; // 36 chars + null terminator

  // Format into UUID v4 style string
  snprintf(uuid, sizeof(uuid),
           "%08lx-%04lx-%04lx-%04lx-%012llx",
           (unsigned long)((chipid >> 32) & 0xFFFFFFFF),
           (unsigned long)((chipid >> 16) & 0xFFFF),
           (unsigned long)(0x4000 | ((chipid >> 0) & 0x0FFF)), // version 4 marker
           (unsigned long)(0x8000 | ((chipid >> 48) & 0x3FFF)), // variant marker
           (unsigned long long)(chipid & 0xFFFFFFFFFFFFULL));

  return String(uuid);
}

// Formats Lap Time from seconds to M:SS.sss
String formatLapTime(float seconds) {
  int mins = (int)(seconds / 60);          // whole minutes
  float secs = fmod(seconds, 60.0);        // remainder seconds
  char buffer[16];

  // Format: M:SS.sss (zero-pad seconds to 2 digits, milliseconds to 3 digits)
  sprintf(buffer, "%d:%06.3f", mins, secs);

  return String(buffer);
}

// API call to get current timezone offset
int64_t getUtcOffsetInSeconds() {
  if (timezoneOverrideActive) {
    return UTCoffset; // just echo back whatever the rollers set
  }

  HTTPClient client;

  //debug->println("Getting local time zone");
  std::string url = "https://ipapi.co/utc_offset/";
  client.begin(url.c_str());
  int statusCode = client.GET();
  //debug->println("Status code: %i", statusCode);
  if (statusCode != 200 && statusCode != 429 && statusCode != -11) {
    //debug.println("Error getting local time zone, status code: %i\n", statusCode);
    return 0;
  }

  std::string offset;

  if (statusCode == 429 || statusCode == -11) {
    offset = "+0000"; // defaults to UTC
  } else {
    offset = std::string(client.getString().c_str());
  }

  client.end();

  //debug->println("Offset: %s", offset.c_str());
  char sign = offset[0];
  uint32_t hours = std::stoi(offset.substr(1, 2));
  uint32_t minutes = std::stoi(offset.substr(3, 2));
  int64_t result = (hours * 3600) + minutes * 60.0;
  
  UTCoffsetHours = hours;
  UTCoffsetMinutes = minutes;

  if (sign == '-') {
    result = -result;
    UTCoffsetHours = -UTCoffsetHours;
    UTCoffsetMinutes = -UTCoffsetMinutes;
  }
  //debug->print("Offset in seconds: "); debug->println(result);

  return result;
}

// Arduino didn't have this basic one so we had to make do...
time_t timegm(struct tm* tm) {
  // Save the current TZ
  char* oldTZ = getenv("TZ");
  setenv("TZ", "", 1); // Empty string means UTC
  tzset();

  time_t result = mktime(tm);

  // Restore the previous TZ
  if (oldTZ)
    setenv("TZ", oldTZ, 1);
  else
    unsetenv("TZ");
  tzset();

  return result;
}

// Tells if a session has already started given the strings for date and time retrieved from API
bool hasSessionStarted(String utcSessionDate, String utcSessionTime) {
    struct tm tmUTC = {};

    int length = strlen(utcSessionTime.c_str());
    utcSessionTime[length-1] = '\0';

    String utcSessionDateTime = utcSessionDate + "T" + (String)utcSessionTime;
    strptime(utcSessionDateTime.c_str(), "%Y-%m-%dT%H:%M:%S", &tmUTC);
    
    time_t sessionEpoch = timegm(&tmUTC); // UTC epoch
    time_t nowUTC = time(nullptr); // Already UTC because gmtOffset_sec = 0

    //Serial.print("[Utils] Epoch now: "); Serial.println(nowUTC);
    //Serial.print("[Utils] Epoch session: "); Serial.println(sessionEpoch);

    return sessionEpoch <= nowUTC ? true : false;
}

// Tells if FP finished, used for avoiding unnecessary API calls, it waits a few minutes longer than an hour
bool hasFreePracticeFinished(String utcSessionDate, String utcSessionTime) {
    struct tm tmUTC = {};

    int length = strlen(utcSessionTime.c_str());
    utcSessionTime[length-1] = '\0';

    String utcSessionDateTime = utcSessionDate + "T" + (String)utcSessionTime;
    strptime(utcSessionDateTime.c_str(), "%Y-%m-%dT%H:%M:%S", &tmUTC);
    
    time_t sessionEpoch = timegm(&tmUTC); // UTC epoch
    time_t nowUTC = time(nullptr); // Already UTC because gmtOffset_sec = 0

    //Serial.print("[Utils] Epoch now: "); Serial.println(nowUTC);
    //Serial.print("[Utils] Epoch session: "); Serial.println(sessionEpoch);

    return (nowUTC - sessionEpoch) >= 3900;
}

// Retrieves the next (unstarted) session from the race weekend sessions
RaceSession getNextSession(NextRaceInfo &race) {
  RaceSession session;

  for (int i=0; i<race.sessionCount; i++) {
    session = race.sessions[i];
    bool started = hasSessionStarted(session.date, session.time);

    if (!started) {
      return session;
    }
  }

  return session;
}

// Localizes session names (handy shortcut)
const char* getLocalizedSessionName(RaceSession &session) {
  if (session.name == "FP1") return localized_text->FP1;
  if (session.name == "FP2") return localized_text->FP2;
  if (session.name == "FP3") return localized_text->FP3;
  if (session.name == "Qualifying") return localized_text->qualifying;
  if (session.name == "Race") return localized_text->race;
  if (session.name == "Sprint Qualifying") return localized_text->sprint_q;
  if (session.name == "Sprint Race") return localized_text->sprint_race;

  return session.name.c_str();
}

// Results API doesn't give driver infos, this retrieves them from the saved standings to avoid another API call
DriverStanding* getDriverInfoByNumber(const String& driverNumber) {
  for (int i = 0; i < current_season.driver_count; i++) {
    //Serial.printf("Driver number: %s\n", current_season.driver_standings[i].number);
    if (current_season.driver_standings[i].number == driverNumber || current_season.driver_standings[i].number == current_f1_champion && driverNumber == "1") {
      return &current_season.driver_standings[i];  // return pointer to the matching struct
    }
  }
  return nullptr; // not found
}

// Formats session datetime 
// @param -> iWant = "all", "date", "time"
char* getSessionDateTimeFormatted(String utcSessionDate, String utcSessionTime, String iWant = "all") {
    static char formatted[50]; // static so it persists after return

    // Remove trailing 'Z' if present
    if (utcSessionTime.endsWith("Z")) {
        utcSessionTime.remove(utcSessionTime.length() - 1);
    }

    // Combine into full datetime string
    String utcSessionDateTime = utcSessionDate + "T" + utcSessionTime;

    // Parse into struct tm
    struct tm sessionTime = {};
    strptime(utcSessionDateTime.c_str(), "%Y-%m-%dT%H:%M:%S", &sessionTime);

    // Convert to time_t in UTC
    time_t sessionEpoch = timegm(&sessionTime);

    // Apply offset in seconds
    sessionEpoch += UTCoffset;

    // Convert back to local tm with rollover handled
    struct tm adjustedTime;
    gmtime_r(&sessionEpoch, &adjustedTime);

    if (iWant == "date") {
        snprintf(formatted, sizeof(formatted), "%s %d %s",
                 localized_text->short_days[adjustedTime.tm_wday],
                 adjustedTime.tm_mday,
                 localized_text->months[adjustedTime.tm_mon]);
    } else if (iWant == "time") {
        snprintf(formatted, sizeof(formatted), "%02d:%02d",
                 adjustedTime.tm_hour,
                 adjustedTime.tm_min);
    } else {
        snprintf(formatted, sizeof(formatted), "%s %d %s, %02d:%02d",
                 localized_text->short_days[adjustedTime.tm_wday],
                 adjustedTime.tm_mday,
                 localized_text->months[adjustedTime.tm_mon],
                 adjustedTime.tm_hour,
                 adjustedTime.tm_min);
    }

    return formatted;
}

// Calls NTP Server to update internal clock
void update_internal_clock() {
  UTCoffset = (long) getUtcOffsetInSeconds();
  configTime(0, 0, "pool.ntp.org");
  Serial.println("[Utils.h] Internal clock updated via NTP");
}

// Runs with lvgl timer, updates internal clock with offset, updates UI display of clock, date, race name and sessions
void update_ui(lv_timer_t *timer) {
  struct tm timeinfo;
  
  if (!getLocalTime(&timeinfo)) return;

  time_t timeEpoch = timegm(&timeinfo);

  if (timeEpoch <= 1000) return;

  // Apply offset in seconds
  timeEpoch += UTCoffset;
  //timeEpoch += 60; //time correction (clock is dragging a tiny bit)


  // Convert back to local tm with rollover handled
  struct tm adjustedTime;
  gmtime_r(&timeEpoch, &adjustedTime);

  if (adjustedTime.tm_hour == 2 && adjustedTime.tm_min % 60 == 0) update_internal_clock();
  Serial.println("[Utils.h] Updating Clock and shit");
  if (racetab_labels.clock) lv_label_set_text_fmt(racetab_labels.clock, "%02d:%02d", adjustedTime.tm_hour, adjustedTime.tm_min);
  if (racetab_labels.date) lv_label_set_text_fmt(racetab_labels.date, "%s %d, %s", localized_text->short_days[adjustedTime.tm_wday], adjustedTime.tm_mday, localized_text->months[adjustedTime.tm_mon]);
  if (racetab_labels.race_name) lv_label_set_text_fmt(racetab_labels.race_name, "%s", next_race.raceName.c_str());

  if (timezoneRoller.hours && lv_obj_is_valid(timezoneRoller.hours) && 
        !lv_obj_has_state(timezoneRoller.hours, LV_STATE_PRESSED) &&
        !lv_obj_has_state(timezoneRoller.hours, LV_STATE_FOCUSED)) {
      lv_roller_set_selected(timezoneRoller.hours, adjustedTime.tm_hour, LV_ANIM_OFF);
  }

  create_or_reload_race_sessions();
  Serial.println("[Utils.h] Race Sessions Updated");

  // NIGHT MODE
  if (nightModeActive) {
    if (adjustedTime.tm_hour == nightModeTimes.start_hours && adjustedTime.tm_min == nightModeTimes.start_minutes) {
      adjustBrightness(night_brightness);
    }
    if (adjustedTime.tm_hour == nightModeTimes.stop_hours && adjustedTime.tm_min == nightModeTimes.stop_minutes) {
      adjustBrightness(brightness);
    }
  }
}

void force_update_ui() {
  struct tm timeinfo;
  
  if (!getLocalTime(&timeinfo)) return;

  time_t timeEpoch = timegm(&timeinfo);

  if (timeEpoch <= 1000) return;

  // Apply offset in seconds
  timeEpoch += UTCoffset;
  //timeEpoch += 60; //time correction (clock is dragging a tiny bit)


  // Convert back to local tm with rollover handled
  struct tm adjustedTime;
  gmtime_r(&timeEpoch, &adjustedTime);

  if (adjustedTime.tm_hour == 2 && adjustedTime.tm_min % 60 == 0) update_internal_clock();
  Serial.println("[Utils.h] Updating Clock and shit");
  if (racetab_labels.clock) lv_label_set_text_fmt(racetab_labels.clock, "%02d:%02d", adjustedTime.tm_hour, adjustedTime.tm_min);
  if (racetab_labels.date) lv_label_set_text_fmt(racetab_labels.date, "%s %d, %s", localized_text->short_days[adjustedTime.tm_wday], adjustedTime.tm_mday, localized_text->months[adjustedTime.tm_mon]);
  if (racetab_labels.race_name) lv_label_set_text_fmt(racetab_labels.race_name, "%s", next_race.raceName.c_str());

  if (timezoneRoller.hours && lv_obj_is_valid(timezoneRoller.hours) && 
        !lv_obj_has_state(timezoneRoller.hours, LV_STATE_PRESSED) &&
        !lv_obj_has_state(timezoneRoller.hours, LV_STATE_FOCUSED)) {
      lv_roller_set_selected(timezoneRoller.hours, adjustedTime.tm_hour, LV_ANIM_OFF);
  }
  
  Serial.println("[Utils.h] Updating Race Sessions");
  create_or_reload_race_sessions( true );
  Serial.println("[Utils.h] Race Sessions Updated");

  // NIGHT MODE
  if (nightModeActive) {
    if (adjustedTime.tm_hour == nightModeTimes.start_hours && adjustedTime.tm_min == nightModeTimes.start_minutes) {
      adjustBrightness(night_brightness);
    }
    if (adjustedTime.tm_hour == nightModeTimes.stop_hours && adjustedTime.tm_min == nightModeTimes.stop_minutes) {
      adjustBrightness(brightness);
    }
  }
}

// Tells if the first session of the race weekend has started
bool hasRaceWeekendStarted() {
  return hasSessionStarted(next_race.sessions[0].date, next_race.sessions[0].time);
}


bool isNightTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return false;

  time_t timeEpoch = timegm(&timeinfo);

  if (timeEpoch <= 1000) return false;

  // Apply offset in seconds
  timeEpoch += UTCoffset;

  // Convert back to local tm
  struct tm adjustedTime;
  gmtime_r(&timeEpoch, &adjustedTime);

  int now   = adjustedTime.tm_hour * 60 + adjustedTime.tm_min;
  int start = nightModeTimes.start_hours * 60 + nightModeTimes.start_minutes;
  int stop  = nightModeTimes.stop_hours  * 60 + nightModeTimes.stop_minutes;

  bool in_range = false;
  if (start < stop) {
      // Normal case: same day
      in_range = (now >= start && now < stop);
  } else if (start > stop) {
      // Rollover case: spans midnight
      in_range = (now >= start || now < stop);
  } else {
      // start == stop → full 24h
      in_range = true;
  }

  return in_range;
}


void halo_set_switch_state(lv_obj_t* switch_obj, bool state) {
    if (state) {
        lv_obj_add_state(switch_obj, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(switch_obj, LV_STATE_CHECKED);
    }
}