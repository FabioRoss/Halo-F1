static void appendNormalizedCodepoint(String &out, int cp) {
  switch (cp) {
    case 228: out += "ae"; return; // ä
    case 246: out += "oe"; return; // ö
    case 252: out += "ue"; return; // ü
    case 196: out += "Ae"; return; // Ä
    case 214: out += "Oe"; return; // Ö
    case 220: out += "Ue"; return; // Ü
    case 223: out += "ss"; return; // ß
    default: break;
  }

  if (cp >= 32 && cp <= 126) out += (char)cp;
}

static bool decodeNamedEntity(const String &entity, String &decoded) {
  if (entity == "amp")  { decoded = "&";  return true; }
  if (entity == "lt")   { decoded = "<";  return true; }
  if (entity == "gt")   { decoded = ">";  return true; }
  if (entity == "quot") { decoded = "\""; return true; }
  if (entity == "apos") { decoded = "'";  return true; }
  if (entity == "nbsp") { decoded = " ";  return true; }
  if (entity == "auml") { decoded = "ae"; return true; }
  if (entity == "ouml") { decoded = "oe"; return true; }
  if (entity == "uuml") { decoded = "ue"; return true; }
  if (entity == "Auml") { decoded = "Ae"; return true; }
  if (entity == "Ouml") { decoded = "Oe"; return true; }
  if (entity == "Uuml") { decoded = "Ue"; return true; }
  if (entity == "szlig"){ decoded = "ss"; return true; }
  return false;
}

// Single-pass text normalization to limit reallocations and fragmentation.
static String normalizeNewsText(const String &text) {
  String out;
  out.reserve(text.length() + 8);

  bool inTag = false;
  bool lastWasSpace = true;
  const size_t n = text.length();

  for (size_t i = 0; i < n; i++) {
    uint8_t c = (uint8_t)text.charAt(i);

    if (inTag) {
      if (c == '>') inTag = false;
      continue;
    }

    if (c == '<') {
      inTag = true;
      continue;
    }

    // HTML entity decoding
    if (c == '&') {
      int semi = text.indexOf(';', i + 1);
      if (semi > (int)i && semi - (int)i <= 10) {
        String entity = text.substring(i + 1, semi);
        String decoded;
        bool ok = false;

        if (entity.startsWith("#")) {
          int cp = 0;
          if (entity.startsWith("#x") || entity.startsWith("#X")) {
            cp = (int)strtol(entity.substring(2).c_str(), nullptr, 16);
          } else {
            cp = entity.substring(1).toInt();
          }
          appendNormalizedCodepoint(out, cp);
          ok = true;
        } else {
          ok = decodeNamedEntity(entity, decoded);
          if (ok) out += decoded;
        }

        if (ok) {
          i = (size_t)semi;
          lastWasSpace = decoded == " ";
          continue;
        }
      }
    }

    // UTF-8: map German special chars, pass ASCII through, drop unknown multibyte.
    if (c < 0x80) {
      if (c == '\r' || c == '\n' || c == '\t') c = ' ';
      if (c == ' ') {
        if (!lastWasSpace) out += ' ';
        lastWasSpace = true;
      } else if (c >= 32) {
        out += (char)c;
        lastWasSpace = false;
      }
      continue;
    }

    if (c == 0xC3 && i + 1 < n) {
      uint8_t d = (uint8_t)text.charAt(i + 1);
      if      (d == 0xA4) out += "ae"; // ä
      else if (d == 0xB6) out += "oe"; // ö
      else if (d == 0xBC) out += "ue"; // ü
      else if (d == 0x84) out += "Ae"; // Ä
      else if (d == 0x96) out += "Oe"; // Ö
      else if (d == 0x9C) out += "Ue"; // Ü
      else if (d == 0x9F) out += "ss"; // ß
      i++;
      lastWasSpace = false;
      continue;
    }

    // Common mojibake sequences (Ã¤ etc.) represented as two UTF-8 codepoints.
    if (c == 0xC3 && i + 3 < n && (uint8_t)text.charAt(i + 1) == 0x83 && (uint8_t)text.charAt(i + 2) == 0xC2) {
      uint8_t d = (uint8_t)text.charAt(i + 3);
      if      (d == 0xA4) out += "ae";
      else if (d == 0xB6) out += "oe";
      else if (d == 0xBC) out += "ue";
      else if (d == 0x84) out += "Ae";
      else if (d == 0x96) out += "Oe";
      else if (d == 0x9C) out += "Ue";
      i += 3;
      lastWasSpace = false;
      continue;
    }
  }

  out.trim();
  return out;
}

// Tries once to fetch the latest news
bool fetchLatestNews(String &title, String &link, String &desc) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  uint8_t feedIndex = selectedNewsFeed < NEWS_FEED_COUNT ? selectedNewsFeed : 0;
  http.begin(client, newsFeedUrls[feedIndex]);
  http.setUserAgent("Mozilla/5.0 (compatible; Halo-F1/1.0; +https://halof1.com)");
  http.addHeader("Accept", "application/rss+xml, application/xml, text/xml;q=0.9, */*;q=0.8");
  http.addHeader("Accept-Language", "en-US,en;q=0.8,nl;q=0.7,de;q=0.6,it;q=0.5");
  // Request uncompressed response; ESP32 stream parser expects plain XML bytes.
  http.addHeader("Accept-Encoding", "identity");
  http.setTimeout(fastNewsFetchMode ? 4000 : 10000);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[News] HTTP error: %d (feed: %s)\n", httpCode, newsFeedNames[feedIndex]);
    http.end();
    return false;
  }

  title = "";
  link = "";
  desc = "";

  auto getTagContent = [](const String &src, const String &tag) -> String {
    String open = "<" + tag;
    String close = "</" + tag + ">";
    int start = src.indexOf(open);
    if (start < 0) return "";
    int openEnd = src.indexOf(">", start);
    if (openEnd < 0) return "";
    start = openEnd + 1;
    int end = src.indexOf(close, start);
    if (end < 0 || end <= start) return "";
    String out = src.substring(start, end);
    out.trim();
    if (out.startsWith("<![CDATA[")) {
      out = out.substring(9);
      int cdataEnd = out.indexOf("]]>");
      if (cdataEnd >= 0) out = out.substring(0, cdataEnd);
    }
    out.trim();
    return out;
  };

  // Stream-based parsing to avoid loading the full feed in RAM.
  // We buffer only part of the first <item>/<entry>, and exit early when
  // we have enough data to render the card.
  WiFiClient *stream = http.getStreamPtr();
  String firstItem = "";
  const size_t MAX_FIRST_ITEM_BUFFER = 24 * 1024; // safety cap for ESP32 RAM
  firstItem.reserve(MAX_FIRST_ITEM_BUFFER);

  bool inItem = false;
  const char *endTag = nullptr;
  unsigned long lastDataAt = millis();
  String scanBuffer = "";
  scanBuffer.reserve(512);

  const size_t CHUNK_SIZE = 256;
  char chunk[CHUNK_SIZE + 1];

  while (stream->connected() || stream->available()) {
    if (!stream->available()) {
      // Don't block forever on stalled connections.
      if (millis() - lastDataAt > 5000) break;
      delay(1);
      continue;
    }

    size_t toRead = stream->available();
    if (toRead > CHUNK_SIZE) toRead = CHUNK_SIZE;
    int got = stream->readBytes(chunk, toRead);
    if (got <= 0) continue;
    chunk[got] = '\0';
    lastDataAt = millis();

    String part = String(chunk);

    if (!inItem) {
      scanBuffer += part;
      if (scanBuffer.length() > 1024) {
        scanBuffer.remove(0, scanBuffer.length() - 1024);
      }

      int itemPos = scanBuffer.indexOf("<item");
      int entryPos = scanBuffer.indexOf("<entry");
      if (itemPos >= 0 || entryPos >= 0) {
        int startPos = -1;
        inItem = true;
        if (entryPos >= 0 && (itemPos < 0 || entryPos < itemPos)) {
          endTag = "</entry>";
          startPos = entryPos;
        } else {
          endTag = "</item>";
          startPos = itemPos;
        }
        part = scanBuffer.substring(startPos);
        scanBuffer = "";
      } else {
        continue;
      }
    }

    if (firstItem.length() + (size_t)got > MAX_FIRST_ITEM_BUFFER) {
      Serial.println("[News] First item exceeded buffer cap, aborting parse.");
      http.end();
      return false;
    }

    firstItem += part;

    // Early extraction makes parsing robust even when feed markup is large.
    if (title.length() == 0) title = getTagContent(firstItem, "title");
    if (desc.length() == 0) desc = getTagContent(firstItem, "description");
    if (desc.length() == 0) desc = getTagContent(firstItem, "summary");
    if (desc.length() == 0) desc = getTagContent(firstItem, "content:encoded");
    if (desc.length() == 0) desc = getTagContent(firstItem, "content");
    if (link.length() == 0) link = getTagContent(firstItem, "link");

    // Atom feeds often use <link href="..."/> instead of <link>...</link>
    if (link.length() == 0) {
      int linkTagStart = firstItem.indexOf("<link");
      if (linkTagStart >= 0) {
        int linkTagEnd = firstItem.indexOf(">", linkTagStart);
        if (linkTagEnd > linkTagStart) {
          String linkTag = firstItem.substring(linkTagStart, linkTagEnd + 1);
          int hrefStart = linkTag.indexOf("href=\"");
          char quote = '"';
          if (hrefStart < 0) {
            hrefStart = linkTag.indexOf("href='");
            quote = '\'';
          }
          if (hrefStart >= 0) {
            hrefStart += 6;
            int hrefEnd = linkTag.indexOf(quote, hrefStart);
            if (hrefEnd > hrefStart) link = linkTag.substring(hrefStart, hrefEnd);
          }
        }
      }
    }

    // Don't stop too early: some feeds provide description later in the item.
    if (link.length() > 0 && title.length() > 0 && desc.length() > 0) break;
    if (endTag && firstItem.indexOf(endTag) >= 0) break;
  }

  http.end();

  if (!inItem || firstItem.length() == 0) return false;

  title = getTagContent(firstItem, "title");
  if (link.length() == 0) link = getTagContent(firstItem, "link");
  if (desc.length() == 0) desc = getTagContent(firstItem, "description");

  // Atom feeds often use <link href="..."/> instead of <link>...</link>
  if (link.length() == 0) {
    int linkTagStart = firstItem.indexOf("<link");
    if (linkTagStart >= 0) {
      int linkTagEnd = firstItem.indexOf(">", linkTagStart);
      if (linkTagEnd > linkTagStart) {
        String linkTag = firstItem.substring(linkTagStart, linkTagEnd + 1);
          int hrefStart = linkTag.indexOf("href=\"");
          char quote = '"';
          if (hrefStart < 0) {
            hrefStart = linkTag.indexOf("href='");
            quote = '\'';
          }
          if (hrefStart >= 0) {
            hrefStart += 6;
            int hrefEnd = linkTag.indexOf(quote, hrefStart);
          if (hrefEnd > hrefStart) link = linkTag.substring(hrefStart, hrefEnd);
        }
      }
    }
  }

  // RSS fallback: some feeds provide the canonical article URL in <guid>.
  if (link.length() == 0) link = getTagContent(firstItem, "guid");

  // Some feeds use alternative description tags.
  if (desc.length() == 0) desc = getTagContent(firstItem, "summary");
  if (desc.length() == 0) desc = getTagContent(firstItem, "content:encoded");
  if (desc.length() == 0) desc = getTagContent(firstItem, "content");

  title = normalizeNewsText(title);
  desc  = normalizeNewsText(desc);

  if (desc.length() > 300) desc = desc.substring(0, 300) + "...";

  if (link == "" && (title != "" || desc != "")) {
    // Fallback: keep the news card usable even when a feed item has no
    // per-article link that we can parse reliably.
    link = newsFeedUrls[feedIndex];
  }
  if (link == "") return false;
  if (title == "" && desc == "") return false;

  Serial.println("[News] Feed: " + String(newsFeedNames[feedIndex]));
  Serial.println("[News] Title: " + title);
  Serial.println("[News] Link: " + link);
  Serial.println("[News] Desc: " + desc);

  return true;
}

// Tries for 30 seconds to fetch the latest news (sometimes it takes more tries to get a reliable connection)
// @TODO -- find a better way, maybe with a timer instead of while loop because it is the only blocking code we are doing
bool getLatestNews(String &title, String &link, String &desc) {
  unsigned long long startTimestamp = millis();
  const unsigned long maxFetchWindowMs = fastNewsFetchMode ? 6000UL : 30000UL;
  const unsigned long retryDelayMs = fastNewsFetchMode ? 250UL : 1000UL;
  Serial.println("[News] Fetching News");

  while (!fetchLatestNews(title, link, desc)) {
    Serial.println("[News] Another cycle of news fetching");
    delay(retryDelayMs);
    if (startTimestamp + maxFetchWindowMs < millis()) return false;
  }

  return true;
}

// Fetch latest session result, returns false if failed or not yet available (session ongoing or not enough time passed since the end)
bool getLastSessionResults(SessionResults results[DRIVERS_NUMBER]) {
  got_new_results = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Session Results] WiFi not connected!");
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
    Serial.printf("[Session Results] HTTP request failed, code: %d\n", httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  if (payload.substring(2,8) == "detail") return false;

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("[Session Results] JSON parsing failed for Last Session Results: ");
    Serial.println(error.c_str());
    return false;
  }

  Serial.print("[Session Results] Payload: ");
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
  if (error) { Serial.printf("[Driver Standings] JSON error: %s\n", error.c_str()); return false; }

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
    if (error) { Serial.printf("[Driver Standings] JSON error: %s\n", error.c_str()); return false; }

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
    if (error) { Serial.printf("[Driver Standings] JSON error: %s\n", error.c_str()); return false; }

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
    if (error) { Serial.printf("[Constructor Standings] JSON error: %s\n", error.c_str()); return false; }

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
      if (error) { Serial.printf("[Constructor Standings] JSON error: %s\n", error.c_str()); return false; }
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
        Serial.println("[Next Race Info] JSON parse failed");
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
      Serial.println("[Next Race Info] Race: " + next_race.raceName);
      Serial.println("[Next Race Info] Circuit: " + next_race.circuitName);
      Serial.println("[Next Race Info] Country: " + next_race.country);
      String next_race_type = next_race.isSprintWeekend ? "Sprint Weekend" : "Normal Weekend";
      Serial.println("[Next Race Info] " + next_race_type);

      for (int i = 0; i < next_race.sessionCount; i++) {
        String has_started = "No";
        if (hasSessionStarted(next_race.sessions[i].date, next_race.sessions[i].time)) has_started = "Yes";
          Serial.printf("[Next Race Info] %s - %s %s - Has already started: %s\n",
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
    Serial.printf("[Statistics Data] HTTP error: %d\n", httpCode);
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
    Serial.printf("[Statistics Data] Synced: %d notifications available.\n", notificationQueue.size());
  }

  http.end();

  //Serial.printf("[Statistics Data] Stats response: %s\n", payload.c_str()); // debug
  return;
}

//flag for saving data, needed for WiFiManager
bool shouldSaveConfig = false;

// WiFiManager callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("[WiFiManager] Should save config");
  shouldSaveConfig = true;
}

// Called when WiFi Access Point is activated for connection setup (first setup or connection failed)
void configModeCallback (WiFiManager *myWiFiManager) {
  lv_screen_load(screen.wifi);

  // Show WiFi setup instructions
  lv_obj_t * label3 = lv_label_create(screen.wifi);
  lv_label_set_text_fmt(label3, localized_text->wifi_connection_failed, WiFi.softAPIP().toString().c_str());
  lv_obj_align(label3, LV_ALIGN_BOTTOM_MID, 0, -20);
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
      Serial.println("[WiFiManager] failed to connect and hit timeout");
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
      Serial.println("[WiFiManager] failed to connect and hit timeout");
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