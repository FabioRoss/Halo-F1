// Tries once to fetch the latest news
bool fetchLatestNews(String &title, String &link, String &desc) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, "https://www.the-race.com/category/formula-1/rss/");

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP error: %d\n", httpCode);
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();

  String line;
  bool inItem = false;
  int lineNum = 0, itemLine = 0;
  int itemPos = -1;

  title = "";
  link = "";
  desc = "";

  bool title_parsed = false, link_parsed = false, desc_parsed = false;

  while (stream->connected() && stream->available()) {
    line = stream->readStringUntil('\n');
    lineNum++;
    itemPos = 0;

    Serial.printf("Line %d: %s\n", lineNum, line.c_str());

    if (line.indexOf("<item>") >= 0) {
      inItem = true;
      itemLine = lineNum;
      itemPos = line.indexOf("<item>");
      Serial.printf("Line of Item: %d, Index Of Item: %d\n", itemLine, itemPos);
    }

    if (inItem) {
      if (line.indexOf("<title>", itemPos) >= 0 && title.length() == 0) {
        int start = line.indexOf("<title><![CDATA[", itemPos) + 16;
        int end   = line.indexOf("]]></title>", itemPos);
        if (end > start) title = line.substring(start, end);
        title_parsed = true;
        Serial.println("Title parsed");
      }

      if (line.indexOf("<description>", itemPos) >= 0 && desc.length() == 0) {
        int start = line.indexOf("<description><![CDATA[", itemPos) + 22;
        int end   = line.indexOf("]]></description>", itemPos);
        if (end > start) {
          desc = line.substring(start, end);
          if (desc.length() > 300) desc = desc.substring(0, 300) + "...";
        }
        desc_parsed = true;
        Serial.println("Desc parsed");
      }

      if (line.indexOf("<link>", itemPos) >= 0 && link.length() == 0) {
        int start = line.indexOf("<link>", itemPos) + 6;
        int end   = line.indexOf("</link>", itemPos);
        if (end > start) link = line.substring(start, end);
        link_parsed = true;
        Serial.println("Link parsed");
      }

      if (link_parsed && desc_parsed && title_parsed) {
        break; // Stop after the first item
      }

    }
  }

  http.end();

  if (link == "") return false;
  if (title == "" && desc == "") return false;

  Serial.println("Title: " + title);
  Serial.println("Link: " + link);
  Serial.println("Desc: " + desc);

  return true;
}

// Tries for 30 seconds to fetch the latest news (sometimes it takes more tries to get a reliable connection)
// @TODO -- find a better way, maybe with a timer instead of while loop because it is the only blocking code we are doing
bool getLatestNews(String &title, String &link, String &desc) {
  unsigned long long startTimestamp = millis();
  Serial.println("Fetching News");

  while (!fetchLatestNews(title, link, desc)) {
    Serial.println("Another cicle of news fetching");
    delay(1000);
    if (startTimestamp + 30000 < millis()) return false;
  }

  return true;
}

// Fetch latest session result, returns false if failed or not yet available (session ongoing or not enough time passed since the end)
bool getLastSessionResults(SessionResults results[DRIVERS_NUMBER]) {
  got_new_results = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return false;
  }

  WiFiClientSecure secureClient;
  secureClient.setInsecure();          // same approach as fetchLatestNews

  HTTPClient http;

  String url = "https://api.openf1.org/v1/session_result?session_key=latest&position%3C=" + (String)DRIVERS_NUMBER;
  //http.begin("https://api.openf1.org/v1/session_result?session_key=7782&position%3C=20"); // debug

  http.begin(secureClient, url);       // explicit TLS client passed
  http.setTimeout(10000); 

  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("HTTP request failed for Last Session Results, code: %d\n", httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  if (payload.substring(2,8) == "detail") return false;

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("JSON parsing failed for Last Session Results: ");
    Serial.println(error.c_str());
    return false;
  }

  Serial.print("Payload: ");
  Serial.println(payload);

  if (payload == "[]") return false;

  JsonArray arr = doc.as<JsonArray>();
  int i = 0;

  for (JsonObject obj : arr) {
    if (i >= TOTAL_DRIVERS) break;

    if(!obj["position"]) return false;

    results[i].position = obj["position"].as<String>();
    results[i].driver_number = obj["driver_number"].as<String>();

    // Handle "duration" (could be number or array)
    if (obj["duration"].is<JsonArray>()) {
      results[i].isQualifying = true;
      int j = 0;
      for (JsonVariant v : obj["duration"].as<JsonArray>()) {
        if (j < 3) results[i].quali[j] = v.as<float>();
        j++;
      }

      j = 0;
      for (JsonVariant v : obj["gap_to_leader"].as<JsonArray>()) {
        if (j < 3) results[i].gap_to_leader_quali[j] = v.as<float>();
        j++;
      }

    } else {
      results[i].isQualifying = false;
      results[i].duration = obj["duration"].as<float>();
      results[i].gap_to_leader = obj["gap_to_leader"].as<float>();
      results[i].dns = obj["dns"].as<bool>();
      results[i].dnf = obj["dnf"].as<bool>();
    }

    i++;
  }

  results_loaded_once = true;
  got_new_results = true;
  
  return true;
}

bool fetch_f1_driver_standings() {
  HTTPClient client;
  JsonDocument doc;
  DeserializationError error;
  int statusCode;
  bool preSeasonFallback = false;

  // ── Driver Standings ────────────────────────────────────────────────────────
  std::string url = "https://api.jolpi.ca/ergast/f1/current/driverstandings/";
  client.begin(url.c_str());
  statusCode = client.GET();
  if (statusCode != 200) { client.end(); return false; }
  error = deserializeJson(doc, client.getStream());
  client.end();
  if (error) { Serial.printf("JSON error: %s\n", error.c_str()); return false; }

  JsonArray driverStandingsLists = doc["MRData"]["StandingsTable"]["StandingsLists"].as<JsonArray>();

  if (!driverStandingsLists.isNull() && driverStandingsLists.size() > 0) {
    // ── Normal path ──────────────────────────────────────────────────────────
    JsonObject standingsList = driverStandingsLists[0];
    current_season.season = standingsList["season"].as<String>();
    current_season.round  = standingsList["round"].as<String>();
    JsonArray standings = standingsList["DriverStandings"].as<JsonArray>();
    current_season.driver_count = standings.size();
    for (size_t i = 0; i < current_season.driver_count && i < 30; i++) {
      JsonObject item   = standings[i];
      JsonObject driver = item["Driver"];
      JsonArray  constructors = item["Constructors"].as<JsonArray>();
      JsonObject constructor;
      if (!constructors.isNull() && constructors.size() > 0)
        constructor = constructors[constructors.size() - 1];
      current_season.driver_standings[i].position      = item["positionText"].as<String>();
      current_season.driver_standings[i].points        = item["points"].as<String>();
      current_season.driver_standings[i].number        = driver["permanentNumber"].as<String>();
      current_season.driver_standings[i].name          = driver["givenName"].as<String>();
      current_season.driver_standings[i].surname       = driver["familyName"].as<String>();
      current_season.driver_standings[i].constructor   = constructor["name"].as<String>();
      current_season.driver_standings[i].constructorId = constructor["constructorId"].as<String>();
      if (current_season.driver_standings[i].name == "Andrea Kimi")
        current_season.driver_standings[i].name = "A. Kimi";
    }
  } else {
    // ── Pre-season fallback ──────────────────────────────────────────────────
    preSeasonFallback = true;

    // Simple lookup table: driverId → {constructorId, constructorName}
    String mapDriverId[30], mapCtorId[30], mapCtorName[30];
    int mapSize = 0;

    // Step 1: fetch constructor list.
    // For each constructor, fetch its driver roster to build the lookup map.
    // Also populate team_standings here so we don't need to fetch it again later.
    doc.clear();
    url = "https://api.jolpi.ca/ergast/f1/current/constructors/";
    client.begin(url.c_str());
    statusCode = client.GET();
    if (statusCode != 200) { client.end(); return false; }
    error = deserializeJson(doc, client.getStream());
    client.end();
    if (error) { Serial.printf("JSON error: %s\n", error.c_str()); return false; }

    JsonArray constructors = doc["MRData"]["ConstructorTable"]["Constructors"].as<JsonArray>();
    current_season.team_count = min((int)constructors.size(), 12);

    for (size_t i = 0; i < (size_t)current_season.team_count; i++) {
      JsonObject ctor     = constructors[i];
      String     ctorId   = ctor["constructorId"].as<String>();
      String     ctorName = ctor["name"].as<String>();

      // Populate team standings with zero points while we're here
      current_season.team_standings[i].position = String(i + 1);
      current_season.team_standings[i].points   = "0";
      current_season.team_standings[i].name     = ctorName;
      current_season.team_standings[i].id       = ctorId;

      // Fetch the driver roster for this constructor
      JsonDocument ctorDriverDoc;
      std::string  ctorDriverUrl = "https://api.jolpi.ca/ergast/f1/current/constructors/"
                                   + std::string(ctorId.c_str()) + "/drivers/";
      client.begin(ctorDriverUrl.c_str());
      if (client.GET() == 200) {
        deserializeJson(ctorDriverDoc, client.getStream());
        JsonArray ctorDrivers = ctorDriverDoc["MRData"]["DriverTable"]["Drivers"].as<JsonArray>();
        for (JsonObject d : ctorDrivers) {
          if (mapSize < 30) {
            mapDriverId[mapSize] = d["driverId"].as<String>();
            mapCtorId[mapSize]   = ctorId;
            mapCtorName[mapSize] = ctorName;
            mapSize++;
          }
        }
      }
      client.end();
    }

    // Step 2: fetch the full driver list and resolve constructor via the map
    doc.clear();
    url = "https://api.jolpi.ca/ergast/f1/current/drivers/";
    client.begin(url.c_str());
    statusCode = client.GET();
    if (statusCode != 200) { client.end(); return false; }
    error = deserializeJson(doc, client.getStream());
    client.end();
    if (error) { Serial.printf("JSON error: %s\n", error.c_str()); return false; }

    current_season.season = doc["MRData"]["DriverTable"]["season"].as<String>();
    current_season.round  = "0";

    JsonArray drivers = doc["MRData"]["DriverTable"]["Drivers"].as<JsonArray>();
    current_season.driver_count = drivers.size();
    for (size_t i = 0; i < current_season.driver_count && i < 30; i++) {
      JsonObject driver   = drivers[i];
      String     driverId = driver["driverId"].as<String>();

      // Resolve constructor from lookup map
      String ctorId = "", ctorName = "";
      for (int m = 0; m < mapSize; m++) {
        if (mapDriverId[m] == driverId) { ctorId = mapCtorId[m]; ctorName = mapCtorName[m]; break; }
      }

      current_season.driver_standings[i].position      = String(i + 1);
      current_season.driver_standings[i].points        = "0";
      current_season.driver_standings[i].name          = driver["givenName"].as<String>();
      current_season.driver_standings[i].surname       = driver["familyName"].as<String>();
      current_season.driver_standings[i].number        = driver["familyName"].as<String>() == "Lindblad" ? "41" : driver["permanentNumber"].as<String>();
      current_season.driver_standings[i].constructor   = ctorName;
      current_season.driver_standings[i].constructorId = ctorId;
      if (current_season.driver_standings[i].name == "Andrea Kimi")
        current_season.driver_standings[i].name = "A. Kimi";
    }
  }

  // ── Constructor Standings ───────────────────────────────────────────────────
  if (preSeasonFallback) {
    // team_standings already populated during the driver fallback above — skip
  } else {
    doc.clear();
    url = "https://api.jolpi.ca/ergast/f1/current/constructorstandings/";
    client.begin(url.c_str());
    statusCode = client.GET();
    if (statusCode != 200) { client.end(); return false; }
    error = deserializeJson(doc, client.getStream());
    client.end();
    if (error) { Serial.printf("JSON error: %s\n", error.c_str()); return false; }

    JsonArray constructorStandingsLists = doc["MRData"]["StandingsTable"]["StandingsLists"].as<JsonArray>();

    if (!constructorStandingsLists.isNull() && constructorStandingsLists.size() > 0) {
      // ── Normal path ────────────────────────────────────────────────────────
      JsonObject standingsList = constructorStandingsLists[0];
      JsonArray  standings     = standingsList["ConstructorStandings"].as<JsonArray>();
      current_season.team_count = standings.size();
      for (size_t i = 0; i < current_season.team_count && i < 12; i++) {
        JsonObject item = standings[i];
        JsonObject team = item["Constructor"];
        current_season.team_standings[i].position = item["position"].as<String>();
        current_season.team_standings[i].points   = item["points"].as<String>();
        current_season.team_standings[i].name     = team["name"].as<String>();
        current_season.team_standings[i].id       = team["constructorId"].as<String>();
      }
    } else {
      // ── Fallback (edge case: driver standings exist but constructor don't) ─
      doc.clear();
      url = "https://api.jolpi.ca/ergast/f1/current/constructors/";
      client.begin(url.c_str());
      statusCode = client.GET();
      if (statusCode != 200) { client.end(); return false; }
      error = deserializeJson(doc, client.getStream());
      client.end();
      if (error) { Serial.printf("JSON error: %s\n", error.c_str()); return false; }
      JsonArray constructors = doc["MRData"]["ConstructorTable"]["Constructors"].as<JsonArray>();
      current_season.team_count = constructors.size();
      for (size_t i = 0; i < current_season.team_count && i < 12; i++) {
        JsonObject team = constructors[i];
        current_season.team_standings[i].position = String(i + 1);
        current_season.team_standings[i].points   = "0";
        current_season.team_standings[i].name     = team["name"].as<String>();
        current_season.team_standings[i].id       = team["constructorId"].as<String>();
      }
    }
  }

  standings_loaded_once = true;
  return true;
}


// Fetch next race infos and loads them into the given "NextRaceInfo" type struct or returns false on fail
bool getNextRaceInfo(NextRaceInfo &info) {
    HTTPClient http;
    //http.begin("https://api.jolpi.ca/ergast/f1/2026/2/races/"); //sprint weekend for testing purposes
    http.begin("https://api.jolpi.ca/ergast/f1/current/next/races/");
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
      String newUrl = http.getLocation(); // this gives the "Location" header from the redirect
      Serial.println("Redirect to: " + newUrl);
      http.end(); // close the previous connection
      http.begin(newUrl);
      httpCode = http.GET();
    }

    if (httpCode != 200) {
        Serial.println("HTTP Error: " + String(httpCode));
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.println("JSON parse failed");
        return false;
    }

    JsonObject race = doc["MRData"]["RaceTable"]["Races"][0];
    info.raceName    = race["raceName"].as<String>();
    info.circuitName = race["Circuit"]["circuitName"].as<String>();
    info.country     = race["Circuit"]["Location"]["country"].as<String>();
    info.lat         = race["Circuit"]["Location"]["lat"].as<float>();
    info.lon         = race["Circuit"]["Location"]["long"].as<float>();

    info.sessionCount = 0;
    info.isSprintWeekend = race["Sprint"].is<JsonVariant>(); //checking if key exists, maybe better to do as race["Sprint"] ? true : false; ??

    // List of possible sessions in API
    const char* sessionKeys[] = { "FirstPractice", "SecondPractice", "ThirdPractice", "SprintQualifying", "Sprint", "Qualifying"};
    const char* sessionNames[] = { "FP1", "FP2", "FP3", "Sprint Qualifying", "Sprint Race", "Qualifying" };
    int sessionTotal = sizeof(sessionKeys) / sizeof(sessionKeys[0]);

    for (int i = 0; i < sessionTotal; i++) {
      if (race[sessionKeys[i]]) {
        String dateUTC = race[sessionKeys[i]]["date"].as<String>();
        String timeUTC = race[sessionKeys[i]]["time"].as<String>();

        info.sessions[info.sessionCount].name = sessionNames[i];
        info.sessions[info.sessionCount].date = dateUTC;
        info.sessions[info.sessionCount].time = timeUTC;
        info.sessionCount++;
      }
    }

    String dateUTC = race["date"].as<String>();
    String timeUTC = race["time"].as<String>();

    info.sessions[info.sessionCount].name = "Race";
    info.sessions[info.sessionCount].date = dateUTC;
    info.sessions[info.sessionCount].time = timeUTC;
    info.sessionCount++;

    return true;
}

// Runs with a lvgl timer, fetches driver standings and next race infos (baseline F1 APIs)
void update_f1_api(lv_timer_t *timer) {
  if (!fetch_f1_driver_standings()) {
    return;
  }

  if (getNextRaceInfo(next_race)) {    
      Serial.println("Race: " + next_race.raceName);
      Serial.println("Circuit: " + next_race.circuitName);
      Serial.println("Country: " + next_race.country);
      Serial.println(next_race.isSprintWeekend ? "Sprint Weekend" : "Normal Weekend");

      for (int i = 0; i < next_race.sessionCount; i++) {
        String has_started = "No";
        if (hasSessionStarted(next_race.sessions[i].date, next_race.sessions[i].time)) has_started = "Yes";
          Serial.printf("%s - %s %s - Has already started: %s\n",
                        next_race.sessions[i].name.c_str(),
                        next_race.sessions[i].date.c_str(),
                        next_race.sessions[i].time.c_str(),
                        has_started.c_str());
      }

      // ── Weather forecast for each session (Open-Meteo, no API key) ─────────
      // fetchWeatherForRace() is self-throttled (WEATHER_REFRESH_MS = 1 h) so
      // calling it here every time update_f1_api fires (hourly) is safe.
      fetchWeatherForRace(next_race);
  }

  //update_driver_standings_ui();
}

void sendStatisticData(lv_timer_t *timer) {
  String UUID = getDeviceUUID();
  String current_language = localized_text->language_name_eng;
  String offset = (String)UTCoffset;

  HTTPClient http;
  String url = "https://www.we-race.it/wp-json/f1-halo/v2/sendstats?uuid=" + UUID + "&language=" + current_language + "&offset=" + offset + "&version=" + fw_version + "&flush=" + random(0, millis());
  http.begin(url.c_str());

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP error: %d\n", httpCode);
    http.end();
    return;
  }

  String payload = http.getString();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (!error) {
    // check for updates
    updateAvailable = doc["update_available"];
    latestVersionString = doc["latest_version"].as<String>();
    update_link = doc["update_link"].as<String>();

    // populate notifications
    notificationQueue.clear();
    JsonArray notifications = doc["notifications"];
    
    for (JsonObject notification : notifications) {
      NotificationItem newItem;
      newItem.title = notification["title"].as<String>();
      newItem.text = notification["text"].as<String>();
      newItem.qrLink = notification["qr"].as<String>();
      notificationQueue.push_back(newItem);
    }
    Serial.printf("Synced: %d notifications available.\n", notificationQueue.size());
  }

  http.end();

  //Serial.printf("Stats response: %s\n", payload.c_str()); // debug
  return;
}

//flag for saving data, needed for WiFiManager
bool shouldSaveConfig = false;

// WiFiManager callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// Called when WiFi Access Point is activated for connection setup (first setup or connection failed)
void configModeCallback (WiFiManager *myWiFiManager) {
  lv_screen_load(screen.wifi);
  lv_obj_t * label3 = lv_label_create(screen.wifi);
  lv_label_set_text(label3, localized_text->wifi_connection_failed);
  lv_obj_align(label3, LV_ALIGN_CENTER, 0, 50);
  lv_label_set_long_mode(label3, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_width(label3, 250, 0);
  lv_timer_periodic_handler();
}

// WiFi Manager and WiFi Handler, runs in setup once. If connection success sets up a bunch of lvgl timers for API update (clock, F1 baseline, News)
void setupWiFiManager(bool forceConfig) {
  //set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);
  wm.setClass("invert"); // dark theme

  if (forceConfig) {
    if (!wm.startConfigPortal("Halo-F1")) {
      if (clock_timer) lv_timer_del(clock_timer);
      clock_timer = NULL;
      if (f1_api_timer) lv_timer_del(f1_api_timer);
      f1_api_timer = NULL;
      if (news_timer) lv_timer_del(news_timer);
      news_timer = NULL;
      if (statistics_timer) lv_timer_del(statistics_timer);
      statistics_timer = NULL;
      if (notifications_timer) lv_timer_del(notifications_timer);
      notifications_timer = NULL;
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    } else {
      update_internal_clock();
      update_f1_api(nullptr);
      update_ui(nullptr);
      create_or_reload_news_ui(nullptr);
      if (!clock_timer) clock_timer = lv_timer_create(update_ui, 60000, NULL);
      if (!f1_api_timer) f1_api_timer = lv_timer_create(update_f1_api, 3600000, NULL);
      if (!news_timer) news_timer = lv_timer_create(create_or_reload_news_ui, 5*60000, NULL);
      if (!statistics_timer) statistics_timer = lv_timer_create(sendStatisticData, 59*60000, NULL);
      if (!notifications_timer) notifications_timer = lv_timer_create(notification_scheduler_task, NOTIFICATION_INTERVAL_MS, NULL);
      lv_screen_load(screen.home);
    }
  } else {
    if (!wm.autoConnect("Halo-F1")) {
      if (clock_timer) lv_timer_del(clock_timer);
      clock_timer = NULL;
      if (f1_api_timer) lv_timer_del(f1_api_timer);
      f1_api_timer = NULL;
      if (news_timer) lv_timer_del(news_timer);
      news_timer = NULL;
      if (statistics_timer) lv_timer_del(statistics_timer);
      statistics_timer = NULL;
      if (notifications_timer) lv_timer_del(notifications_timer);
      notifications_timer = NULL;
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // if we still have not connected restart and try all over again
      ESP.restart();
      delay(5000);
    } else {
      update_internal_clock();
      update_f1_api(nullptr);
      update_ui(nullptr);
      create_or_reload_news_ui(nullptr);
      if (!clock_timer) clock_timer = lv_timer_create(update_ui, 60000, NULL);
      if (!f1_api_timer) f1_api_timer = lv_timer_create(update_f1_api, 3600000, NULL);
      if (!news_timer) news_timer = lv_timer_create(create_or_reload_news_ui, 5*60000, NULL);
      if (!statistics_timer) statistics_timer = lv_timer_create(sendStatisticData, 59*60000, NULL);
      if (!notifications_timer) notifications_timer = lv_timer_create(notification_scheduler_task, NOTIFICATION_INTERVAL_MS, NULL);
      lv_screen_load(screen.home);
    }
  }
  lv_timer_periodic_handler();


  //save the custom parameters to FS (not used for now)
  if (shouldSaveConfig)
  {
    ESP.restart();
    delay(5000);
  }

}