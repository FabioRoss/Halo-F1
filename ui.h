// -- FORWARD DECLARATIONS -- //
void create_or_reload_settings_ui();
void update_driver_standings_ui();
bool getLastSessionResults(SessionResults results[DRIVERS_NUMBER]);
bool fetch_f1_driver_standings();
static void populate_standings(lv_obj_t * container, int offset);
static void populate_results(lv_obj_t * container, int offset);
bool getLatestNews(String &title, String &link, String &desc);
void create_or_reload_news_ui(lv_timer_t *timer);
void create_or_reload_standings_tab_ui();

// -- STYLES -- //

// STANDINGS CONTAINER
static lv_style_t style_fade;
static bool style_fade_inited = false;
// STANDINGS
static lv_style_t style_standings_row;
static lv_style_t style_standings_num_container;
static lv_style_t style_standings_team_color;
static lv_style_t style_standings_lbl_name;
static lv_style_t style_standings_lbl_points;
static bool styles_standings_initialized = false;
// RACE SESSIONS
static lv_style_t style_session_row;    // NEW: per-session flex-row wrapper
static lv_style_t style_session_label;
static lv_style_t style_stripe;
static bool race_styles_initialized = false;
// NEWS
static lv_style_t style_news_container;
static lv_style_t style_news_title;
static lv_style_t style_news_desc;
static lv_style_t style_qr_caption;
static bool news_styles_initialized = false;
// STANDINGS TAB
static lv_style_t style_standings_tab_btn_active;
static lv_style_t style_standings_tab_btn_inactive;
static lv_style_t style_standings_tab_list;
static bool standings_tab_styles_initialized = false;

static constexpr uint8_t NEWS_TAB_INDEX = 2;

static void news_tab_pulse_anim_cb(void *var, int32_t v) {
    lv_obj_t *btn = (lv_obj_t *)var;
    if (!btn) return;
    lv_obj_set_style_opa(btn, (lv_opa_t)v, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void stop_news_tab_pulse() {
    if (!news_tab_button) return;
    // Hard-stop all animations bound to this tab button to avoid any lingering pulse.
    lv_anim_del(news_tab_button, NULL);
    lv_obj_set_style_opa(news_tab_button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void start_news_tab_pulse() {
    if (!news_tab_button || !newsPulseEnabled) return;

    lv_anim_del(news_tab_button, news_tab_pulse_anim_cb);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, news_tab_button);
    lv_anim_set_exec_cb(&a, news_tab_pulse_anim_cb);
    // Stronger pulse: deeper dip + a bit faster.
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_30);
    lv_anim_set_time(&a, 650);
    lv_anim_set_playback_time(&a, 650);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

static void sync_news_tab_pulse_state() {
    if (!news_tab_button) return;
    if (hasUnreadNews && newsPulseEnabled && home_tabs && lv_tabview_get_tab_active(home_tabs) != NEWS_TAB_INDEX) {
        start_news_tab_pulse();
    } else {
        stop_news_tab_pulse();
    }
}

static void news_pulse_switch_handler(lv_event_t * e) {
    lv_obj_t * sw = (lv_obj_t *) lv_event_get_target(e);
    newsPulseEnabled = lv_obj_has_state(sw, LV_STATE_CHECKED);

    sync_news_tab_pulse_state();

    saveSettings();
}

static void news_tab_button_clicked_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED &&
        code != LV_EVENT_RELEASED &&
        code != LV_EVENT_VALUE_CHANGED) return;
    hasUnreadNews = false;
    sync_news_tab_pulse_state();
}


void adjustBrightness(uint8_t new_brightness) {
  lv_bb_spi_lcd_t * dsc = (lv_bb_spi_lcd_t *)lv_display_get_driver_data(disp);
  dsc->lcd->setBrightness(new_brightness);
  Serial.printf("[UI] Brightness Adjusted to: %d\n", new_brightness);
}

static void language_selection_event_handler(lv_event_t * e) {
  lv_obj_t* obj = (lv_obj_t *)lv_event_get_target(e);
  uint16_t sel = lv_dropdown_get_selected(obj);

  if (sel < languageCount) {
      localized_text = languages[sel].strings;
      Serial.printf("[UI] Language changed to: %s\n", languages[sel].displayName);
      create_or_reload_settings_ui();
      create_or_reload_standings_tab_ui();
      force_update_ui();
      //create_or_reload_news_ui(nullptr); //takes too long
  }

  saveSettings();
}

static void news_feed_selection_event_handler(lv_event_t * e) {
  lv_obj_t* obj = (lv_obj_t *)lv_event_get_target(e);
  uint16_t sel = lv_dropdown_get_selected(obj);

  if (sel < NEWS_FEED_COUNT) {
      selectedNewsFeed = (uint8_t)sel;
      Serial.printf("[UI] News feed changed to: %s\n", newsFeedNames[selectedNewsFeed]);
      fastNewsFetchMode = true;
      create_or_reload_news_ui(nullptr);
      fastNewsFetchMode = false;
  }

  saveSettings();
}

static void msgbox_close_event_handler(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * btn = lv_event_get_target_obj(e);

  lv_msgbox_close(lv_obj_get_parent(lv_obj_get_parent(btn)));
}

static void brightness_slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = (lv_obj_t *) lv_event_get_target(e);
    brightness = (uint8_t) lv_slider_get_value(slider);
    
    if (!isNightTime() || !nightModeActive) adjustBrightness(brightness);

    if (lv_event_get_code(e) == LV_EVENT_RELEASED) saveSettings();
}

static void night_brightness_slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = (lv_obj_t *) lv_event_get_target(e);
    night_brightness = (uint8_t) lv_slider_get_value(slider);
 
    if (isNightTime() && nightModeActive) adjustBrightness(night_brightness);

    if (lv_event_get_code(e) == LV_EVENT_RELEASED) saveSettings();
}

static void no_spoiler_switch_handler(lv_event_t * e) {
    lv_obj_t * sw = (lv_obj_t *) lv_event_get_target(e);

    // Discard any active lift
    noSpoilerLifted           = false;
    noSpoilerLiftedForSession = "";

    if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        noSpoilerModeActive = true;
    } else {
        noSpoilerModeActive = false;
    }

    force_update_ui();
    //update_ui(nullptr); //maybe add if condition to only update if we're potentially exposed to spoilers?

    saveSettings();
}


static void timezone_roller_event_handler(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) saveSettings();
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    if (!timezoneOverrideActive) return;
    if (!timezoneRoller.hours) return;

    // Get current UTC from NTP
    struct tm utcTime;
    if (!getLocalTime(&utcTime)) return;

    int utc_total_minutes = utcTime.tm_hour * 60 + utcTime.tm_min;

    // Read roller (local time the user set)
    int32_t local_h = (int32_t)lv_roller_get_selected(timezoneRoller.hours);
    int local_total_minutes = local_h * 60 + utcTime.tm_min;

    // Compute offset with wraparound handling
    int offset_minutes = local_total_minutes - utc_total_minutes;
    if (offset_minutes >  720) offset_minutes -= 1440; // > +12h → wrap negative
    if (offset_minutes < -720) offset_minutes += 1440; // < -12h → wrap positive

    UTCoffsetHours   = offset_minutes / 60;
    //UTCoffsetMinutes = offset_minutes % 60;
    UTCoffset        = (long)offset_minutes * 60;
}

static void timezone_override_switch_handler(lv_event_t * e) {
    lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
    timezoneOverrideActive = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (!timezoneOverrideActive) {
        // Re-fetch from ipapi so we snap back to real offset
        UTCoffset = getUtcOffsetInSeconds();
    }
    // If activating: rollers already reflect current UTCoffset (set during creation),
    // so we just stop future ipapi calls from overwriting.
    saveSettings();
}

static void night_mode_switch_handler(lv_event_t * e) {
    lv_obj_t * sw = (lv_obj_t *) lv_event_get_target(e);

    if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        nightModeActive = true;

        if (isNightTime()) {
            adjustBrightness(night_brightness);
            //esp_light_sleep_start();
        } else {
            adjustBrightness(brightness);
        }
    } else {
        nightModeActive = false;
        adjustBrightness(brightness);
    }

    saveSettings();
}


static void night_mode_roller_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    //lv_obj_t * obj = lv_event_get_target_obj(e);
    
    if(code == LV_EVENT_VALUE_CHANGED) {
      char buf[32];
      lv_roller_get_selected_str(nightModeStartRoller.hours, buf, sizeof(buf));
      LV_LOG_USER("Selected hours: %s\n", buf);

      nightModeTimes.start_hours = (uint8_t) lv_roller_get_selected(nightModeStartRoller.hours);
      nightModeTimes.start_minutes = (uint8_t) lv_roller_get_selected(nightModeStartRoller.minutes) * 5;

      nightModeTimes.stop_hours = (uint8_t) lv_roller_get_selected(nightModeStopRoller.hours);
      nightModeTimes.stop_minutes = (uint8_t) lv_roller_get_selected(nightModeStopRoller.minutes) * 5;

      Serial.printf("[UI] Night Mode Timings: %02d:%02d -> %02d:%02d", nightModeTimes.start_hours, nightModeTimes.start_minutes, nightModeTimes.stop_hours, nightModeTimes.stop_minutes);
    
      if (nightModeActive && isNightTime()) {
        adjustBrightness(night_brightness);
      } else {
        adjustBrightness(brightness);
      }

    } else if (code == LV_EVENT_RELEASED) {
      saveSettings();
    }
}

// Currently unused, but ready for implementation
static void reload_clock_event_handler(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  update_internal_clock();
  force_update_ui();
  //update_ui(nullptr);

  lv_obj_t *msgbox = lv_msgbox_create(NULL);
  lv_msgbox_add_text(msgbox, localized_text->clock_updated);
  char buf[50];
  snprintf(buf, 50, "%s %s", LV_SYMBOL_OK, localized_text->success);
  lv_msgbox_add_title(msgbox, buf);
  lv_obj_t *btn = lv_msgbox_add_footer_button(msgbox, localized_text->ok);
  lv_obj_add_event_cb(btn, msgbox_close_event_handler, LV_EVENT_CLICKED, NULL);
}

lv_obj_t* create_standings_row(lv_obj_t *parent,
                            const String &number,
                            const String &name,
                            const String &surname,
                            const String &points,
                            const String &team) {
    //Serial.println("Creating Standings Row");

    // Initialize styles only once
    if(!styles_standings_initialized) {
        styles_standings_initialized = true;

        // Row container
        lv_style_init(&style_standings_row);
        lv_style_set_bg_opa(&style_standings_row, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_standings_row, 0);
        lv_style_set_pad_all(&style_standings_row, 0);

        // Number container
        lv_style_init(&style_standings_num_container);
        lv_style_set_bg_opa(&style_standings_num_container, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_standings_num_container, 0);
        lv_style_set_pad_gap(&style_standings_num_container, 4);

        // Team color block
        lv_style_init(&style_standings_team_color);
        lv_style_set_border_width(&style_standings_team_color, 0);
        lv_style_set_radius(&style_standings_team_color, 2);

        // Driver name label
        lv_style_init(&style_standings_lbl_name);
        lv_style_set_text_align(&style_standings_lbl_name, LV_TEXT_ALIGN_LEFT);

        // Points label
        lv_style_init(&style_standings_lbl_points);
        lv_style_set_text_align(&style_standings_lbl_points, LV_TEXT_ALIGN_RIGHT);
        
    }

    // --- Row container ---
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), 22);
    lv_obj_add_style(row, &style_standings_row, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // --- Driver number container ---
    lv_obj_t *num_container = lv_obj_create(row);
    lv_obj_set_width(num_container, 48); // fixed width for alignment
    lv_obj_add_style(num_container, &style_standings_num_container, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_layout(num_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(num_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(num_container, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Driver number label
    lv_obj_t *lbl_number = lv_label_create(num_container);
    lv_label_set_text(lbl_number, number.c_str());

    // Team color rectangle
    lv_obj_t *team_color = lv_obj_create(num_container);
    lv_obj_set_size(team_color, 8, 18);
    lv_obj_add_style(team_color, &style_standings_team_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(team_color, lv_color_hex(get_team_color(team)),
                              LV_PART_MAIN | LV_STATE_DEFAULT); // dynamic per row
    lv_obj_remove_flag(team_color, LV_OBJ_FLAG_SCROLLABLE);

    // --- Name + surname label ---
    String fullname = name + " " + surname;
    lv_obj_t *lbl_name = lv_label_create(row);
    lv_label_set_text(lbl_name, fullname.c_str());
    lv_obj_set_width(lbl_name, LV_PCT(60));
    lv_obj_add_style(lbl_name, &style_standings_lbl_name, LV_PART_MAIN | LV_STATE_DEFAULT);

    // --- Points label ---
    lv_obj_t *lbl_points = lv_label_create(row);
    lv_label_set_text_fmt(lbl_points, "%s", points.c_str());
    lv_obj_set_width(lbl_points, 60);
    lv_label_set_long_mode(lbl_points, LV_LABEL_LONG_SCROLL);
    lv_obj_add_style(lbl_points, &style_standings_lbl_points, LV_PART_MAIN | LV_STATE_DEFAULT);

    return row;
}

void animate_standings(lv_obj_t * container) {
    if (!style_fade_inited) {
        lv_style_init(&style_fade);
        lv_style_set_opa(&style_fade, LV_OPA_COVER);  // start fully visible
        style_fade_inited = true;
    }

    // Ensure container has our style attached
    lv_obj_add_style(container, &style_fade, LV_PART_MAIN | LV_STATE_DEFAULT);

    //Serial.printf("Heap free: %d\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
    //Serial.println("Animating Standings...fading out the container");

    // Step 1: Fade OUT
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &style_fade);   // animate the style, not the object
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 300);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v) {
        lv_style_set_opa((lv_style_t *)var, v);
        lv_obj_report_style_change((lv_style_t *)var);
    });
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);

    //lv_anim_set_deleted_cb(&a, [](lv_anim_t * a) {
    lv_anim_set_completed_cb(&a, [](lv_anim_t * a) {
        //Serial.println("Container faded out, cleaning container");

        lv_obj_t * cont = (lv_obj_t *)a->user_data;

        // Clear old rows
        lv_obj_clean(cont);

        //Serial.println("Container cleaned, populating");

        // Advance offset
        standings_offset += STANDINGS_PAGE_SIZE;
        if (standings_offset >= TOTAL_DRIVERS) standings_offset = 0;

        // Repopulate
        populate_standings(cont, standings_offset);

        //Serial.println("Populated, fading container back in");

        // Fade IN
        lv_anim_t a_in;
        lv_anim_init(&a_in);
        lv_anim_set_var(&a_in, &style_fade);
        lv_anim_set_values(&a_in, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_anim_set_time(&a_in, 400);
        lv_anim_set_exec_cb(&a_in, [](void * var, int32_t v) {
            lv_style_set_opa((lv_style_t *)var, v);
            lv_obj_report_style_change((lv_style_t *)var);
        });
        lv_anim_set_path_cb(&a_in, lv_anim_path_ease_out);
        lv_anim_start(&a_in);

        //Serial.println("Fading standings container back in -- animation started");
    });

    a.user_data = container;
    lv_anim_start(&a);

    //Serial.println("Animating standings -- function has run -- (fade out animation should have started now)");
}

static void populate_standings(lv_obj_t * container, int offset) {
    //Serial.println("Populating standings");

    for (int i = 0; i < STANDINGS_PAGE_SIZE; i++) {
        int idx = offset + i;
        if (idx >= TOTAL_DRIVERS) break;

        String points = String(current_season.driver_standings[idx].points) + " pt.";
        lv_obj_t * row = create_standings_row(
            container,
            current_season.driver_standings[idx].position.c_str(),
            current_season.driver_standings[idx].name.c_str(),
            current_season.driver_standings[idx].surname.c_str(),
            points.c_str(),
            current_season.driver_standings[idx].constructorId
        );
    }

    //Serial.println("Populating standings DONE");
}

void animate_results(lv_obj_t * container) {
    if (!style_fade_inited) {
        lv_style_init(&style_fade);
        lv_style_set_opa(&style_fade, LV_OPA_COVER);  // start fully visible
        style_fade_inited = true;
    }

    // Ensure container has our style attached
    lv_obj_add_style(container, &style_fade, LV_PART_MAIN | LV_STATE_DEFAULT);

    //Serial.printf("Heap free: %d\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
    //Serial.println("Animating Results...fading out the container");

    // Step 1: Fade OUT
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &style_fade);   // animate the style, not the object
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 300);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v) {
        lv_style_set_opa((lv_style_t *)var, v);
        lv_obj_report_style_change((lv_style_t *)var);
    });
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);

    //lv_anim_set_deleted_cb(&a, [](lv_anim_t * a) {
    lv_anim_set_completed_cb(&a, [](lv_anim_t * a) {
        //Serial.println("Container faded out, cleaning container");

        lv_obj_t * cont = (lv_obj_t *)a->user_data;

        // Clear old rows
        lv_obj_clean(cont);

        //Serial.println("Standings container cleaned, populating");

        // Advance offset
        standings_offset += STANDINGS_PAGE_SIZE;
        if (standings_offset >= TOTAL_DRIVERS) standings_offset = 0;

        // Repopulate
        populate_results(cont, standings_offset);

       //Serial.println("Standings Populated, fading container back in");

        // Fade IN
        lv_anim_t a_in;
        lv_anim_init(&a_in);
        lv_anim_set_var(&a_in, &style_fade);
        lv_anim_set_values(&a_in, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_anim_set_time(&a_in, 400);
        lv_anim_set_exec_cb(&a_in, [](void * var, int32_t v) {
            lv_style_set_opa((lv_style_t *)var, v);
            lv_obj_report_style_change((lv_style_t *)var);
        });
        lv_anim_set_path_cb(&a_in, lv_anim_path_ease_out);
        lv_anim_start(&a_in);

        //Serial.println("Fading results container back in -- animation started");
    });

    a.user_data = container;
    lv_anim_start(&a);

    //Serial.println("Animating results -- function has run -- (fade out animation should have started now)");
}

static void populate_results(lv_obj_t * container, int offset) {
    //Serial.println("[UI] Populating Results");
    uint32_t delay = 400;

    if (current_results != "Qualifying" && current_results != "Sprint Qualifying") {
      for (int i = 0; i < STANDINGS_PAGE_SIZE; i++) {
        int idx = offset + i;
        if (idx >= TOTAL_DRIVERS) break;

        DriverStanding * driver = getDriverInfoByNumber(results[idx].driver_number);

        if (driver == nullptr) {
          Serial.printf("[UI] Driver not found for number %d, id: %d\n", results[idx].driver_number, idx);
          continue; // skip this entry
        }

        char driverInitial;
        if (driver->name.length() == 0) {
          Serial.printf("[UI] Driver %d has empty name\n", results[idx].driver_number);
        } else {
          driverInitial = driver->name.charAt(0);
        }

        String name = String(driverInitial) + ". " + driver->surname + " #" + results[idx].driver_number;      
        String gap;

        char gappo[10];
        snprintf(gappo, 10, "+ %.3f",  results[idx].gap_to_leader);

        gap = (String) gappo;
        if (idx==0) gap = (String) formatLapTime(results[idx].duration);

        lv_obj_t * row = create_standings_row(container,
                            results[idx].position.c_str(),
                            name.c_str(),
                            "",
                            gap,
                            driver->constructorId);

        Serial.println("[UI] Standings row created and populated");
      }
    } else {
        for (int i = 0; i < STANDINGS_PAGE_SIZE; i++) {
        int idx = offset + i;
        if (idx >= TOTAL_DRIVERS) break;

        DriverStanding * driver = getDriverInfoByNumber(results[idx].driver_number);

        if (driver == nullptr) {
          Serial.printf("[UI] Driver not found for number %d\n", results[idx].driver_number);
          continue; // skip this entry
        }

        char driverInitial;
        if (driver->name.length() == 0) {
          Serial.printf("[UI] Driver %d has empty name\n", results[idx].driver_number);
        } else {
          driverInitial = driver->name.charAt(0);
        }

        String name = String(driverInitial) + ". " + driver->surname + " #" + results[idx].driver_number;      
        String gap;


        char gappo[10];
        if (idx < 10) snprintf(gappo, 10, "+ %.3f",  results[idx].gap_to_leader_quali[2]);

        gap = (String) gappo;
        if (idx==0) gap = (String) formatLapTime(results[idx].quali[2]);

        lv_obj_t * row = create_standings_row(container,
                            results[idx].position.c_str(),
                            name.c_str(),
                            "",
                            gap,
                            driver->constructorId);
      }
    }
}

void set_custom_theme(void) {
  static lv_theme_t *my_theme;

  my_theme = lv_theme_default_init(
    NULL,
    lv_color_hex(0xFF1511), //0xFFCC00 0x0088FF 0x008C3F
    lv_color_hex(0x000000),
    LV_THEME_DEFAULT_DARK,
    &montserrat_14
  );

  lv_disp_set_theme(NULL, my_theme);
}

lv_obj_t* create_chequered_stripe(lv_obj_t* parent) {
    lv_coord_t screen_w = lv_disp_get_hor_res(NULL);
    lv_coord_t stripe_h = 8;
    lv_coord_t square_height = stripe_h / 2;  // two rows of squares
    lv_coord_t square_width = 17;  // two rows of squares

    // Container for the stripe (black background)
    lv_obj_t* stripe = lv_obj_create(parent);
    lv_obj_set_size(stripe, screen_w, stripe_h);
    lv_obj_clear_flag(stripe, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(stripe, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(stripe, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(stripe, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(stripe, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Number of columns needed
    //int cols = (screen_w + square_width - 1) / (square_size * width_factor);
    int cols = 16;

    // Create only the white squares
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < cols; col++) {
            if ((row + col) % 2 == 1) {  // only draw the white ones
                lv_obj_t* square = lv_obj_create(stripe);
                lv_obj_set_size(square, square_width, square_height);
                lv_obj_set_pos(square, col * square_width, row * square_height);
                lv_obj_set_style_border_width(square, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(square, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_radius(square, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(square, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
    }

    return stripe;
}

lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt, int builder_variant) {
    lv_obj_t * obj = lv_menu_cont_create(parent);

    lv_obj_t * img = NULL;
    lv_obj_t * label = NULL;

    if(icon) {
        img = lv_img_create(obj);
        lv_img_set_src(img, icon);
    }

    if(txt) {
        label = lv_label_create(obj);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);
    }

    if(builder_variant == 0 && icon && txt) {
        lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_swap(img, label);
    }

    
    lv_obj_set_style_text_color(obj, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_pad_ver(obj, 20, 0);

    return obj;
}

lv_obj_t * create_language_selector(lv_obj_t * parent) {
  lv_obj_t *obj = create_text(parent, LV_SYMBOL_KEYBOARD, localized_text->language, 1);

  lv_obj_t *selector = lv_dropdown_create(obj);

  char language_options[256];
  size_t used = 0;
  language_options[0] = '\0';
  for (size_t i = 0; i < languageCount; i++) {
      int written = snprintf(
          language_options + used,
          sizeof(language_options) - used,
          (i < languageCount - 1) ? "%s\n" : "%s",
          languages[i].displayName
      );
      if (written < 0) break;
      if ((size_t)written >= sizeof(language_options) - used) {
          used = sizeof(language_options) - 1;
          language_options[used] = '\0';
          break;
      }
      used += (size_t)written;
  }
  lv_dropdown_set_options(selector, language_options);

  size_t currentIndex = 0;
  for (size_t i = 0; i < languageCount; i++) {
      if (languages[i].strings == localized_text) {
          currentIndex = i;
          break;
      }
  }
  lv_dropdown_set_selected(selector, currentIndex);

  return selector;
}

lv_obj_t * create_news_feed_selector(lv_obj_t * parent) {
  lv_obj_t *obj = create_text(parent, LV_SYMBOL_LIST, localized_text->news_feed_label, 1);
  lv_obj_t *selector = lv_dropdown_create(obj);

  char news_feed_options[192];
  size_t used = 0;
  news_feed_options[0] = '\0';
  for (uint8_t i = 0; i < NEWS_FEED_COUNT; i++) {
      int written = snprintf(
          news_feed_options + used,
          sizeof(news_feed_options) - used,
          (i < NEWS_FEED_COUNT - 1) ? "%s\n" : "%s",
          newsFeedNames[i]
      );
      if (written < 0) break;
      if ((size_t)written >= sizeof(news_feed_options) - used) {
          used = sizeof(news_feed_options) - 1;
          news_feed_options[used] = '\0';
          break;
      }
      used += (size_t)written;
  }
  lv_dropdown_set_options(selector, news_feed_options);

  if (selectedNewsFeed >= NEWS_FEED_COUNT) selectedNewsFeed = 0;
  lv_dropdown_set_selected(selector, selectedNewsFeed);
  lv_obj_add_flag(selector, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  lv_obj_set_width(selector, LV_PCT(100));

  return selector;
}

lv_obj_t * create_slider(lv_obj_t * parent, const char * icon, const char * txt, int32_t min, int32_t max, int32_t val) {
    lv_obj_t * obj = create_text(parent, icon, txt, 1);
    lv_obj_t * slider = lv_slider_create(obj);

    lv_obj_set_style_pad_right(obj, 18, LV_PART_MAIN);

    lv_obj_set_flex_grow(slider, 1);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);

    lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

    return slider;
}

lv_obj_t * create_switch(lv_obj_t * parent, const char * icon, const char * txt, bool chk) {
    lv_obj_t * obj = create_text(parent, icon, txt, 1);
    lv_obj_t * sw = lv_switch_create(obj);

    halo_set_switch_state(sw, chk);

    return sw;
}

lv_obj_t * create_time_roller(lv_obj_t *parent, const char *icon, const char *text) {
    lv_obj_t * obj = create_text(parent, icon, text, 1);
    lv_obj_t * sw = lv_switch_create(obj);

    halo_set_switch_state(sw, nightModeActive);

    lv_obj_add_event_cb(sw, night_mode_switch_handler, LV_EVENT_VALUE_CHANGED, NULL);

    nightModeStartRoller.hours = lv_roller_create(obj);
    nightModeStartRoller.minutes = lv_roller_create(obj);

    lv_obj_add_flag(nightModeStartRoller.hours, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

    lv_roller_set_options(nightModeStartRoller.hours,
                          "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23",
                          LV_ROLLER_MODE_INFINITE);
    lv_roller_set_options(nightModeStartRoller.minutes,
                          "00\n05\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55",
                          LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(nightModeStartRoller.hours, 2);
    lv_roller_set_visible_row_count(nightModeStartRoller.minutes, 2);

    lv_roller_set_selected(nightModeStartRoller.hours, nightModeTimes.start_hours, LV_ANIM_OFF);
    lv_roller_set_selected(nightModeStartRoller.minutes, nightModeTimes.start_minutes / 5, LV_ANIM_OFF);

    lv_obj_t * arrowLabel = lv_label_create(obj);
    lv_label_set_text(arrowLabel, LV_SYMBOL_RIGHT);

    nightModeStopRoller.hours = lv_roller_create(obj);
    nightModeStopRoller.minutes = lv_roller_create(obj);

    lv_roller_set_options(nightModeStopRoller.hours,
                          "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23",
                          LV_ROLLER_MODE_INFINITE);
    lv_roller_set_options(nightModeStopRoller.minutes,
                          "00\n05\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55",
                          LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(nightModeStopRoller.hours, 2);
    lv_roller_set_visible_row_count(nightModeStopRoller.minutes, 2);

    lv_roller_set_selected(nightModeStopRoller.hours, nightModeTimes.stop_hours, LV_ANIM_OFF);
    lv_roller_set_selected(nightModeStopRoller.minutes, nightModeTimes.stop_minutes / 5, LV_ANIM_OFF);

    lv_obj_add_event_cb(nightModeStartRoller.hours, night_mode_roller_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(nightModeStartRoller.minutes, night_mode_roller_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(nightModeStopRoller.hours, night_mode_roller_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(nightModeStopRoller.minutes, night_mode_roller_event_handler, LV_EVENT_ALL, NULL);
    return obj;
}

// Creates a horizontal rule in the settings container.
// Pass nullptr for title to get a plain line with no label.
lv_obj_t * create_settings_divider(lv_obj_t *parent, const char *title = nullptr) {
    // Wrapper row: full width, holds optional label + the line
    lv_obj_t *wrapper = lv_obj_create(parent);
    lv_obj_remove_style_all(wrapper);
    lv_obj_set_size(wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_ver(wrapper, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(wrapper, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wrapper,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(wrapper, LV_OBJ_FLAG_SCROLLABLE);

    if (title != nullptr && strlen(title) > 0) {
        lv_obj_t *lbl = lv_label_create(wrapper);
        lv_label_set_text(lbl, title);
        lv_obj_set_style_text_font(lbl, &montserrat_12, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
        lv_obj_set_style_pad_right(lbl, 8, 0);
        lv_obj_set_height(lbl, LV_SIZE_CONTENT);
    }

    // The horizontal line: a thin object that grows to fill remaining width
    lv_obj_t *line = lv_obj_create(wrapper);
    lv_obj_remove_style_all(line);
    lv_obj_set_flex_grow(line, 1);
    lv_obj_set_height(line, 1);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x333333), 0);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);

    return wrapper;
}

lv_obj_t * create_timezone_roller(lv_obj_t *parent, const char *icon, const char *text) {
    lv_obj_t * obj = create_text(parent, icon, text, 1);

    timezone_override_switch = lv_switch_create(obj);
    halo_set_switch_state(timezone_override_switch, timezoneOverrideActive);

    lv_obj_add_event_cb(timezone_override_switch, timezone_override_switch_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // Compute current local time to pre-populate rollers
    struct tm utcTime;
    time_t epoch;
    if (getLocalTime(&utcTime)) {
        epoch = timegm(&utcTime);
    } else {
        // Time not yet synced; fall back to Unix epoch (00:00 UTC, 1 Jan 1970)
        epoch = 0;
    }
    epoch += UTCoffset;
    struct tm localTime;
    gmtime_r(&epoch, &localTime);

    // Hours roller
    timezoneRoller.hours = lv_roller_create(obj);
    lv_obj_set_width(timezoneRoller.hours, LV_PCT(100));
    lv_obj_add_flag(timezoneRoller.hours, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_roller_set_options(timezoneRoller.hours,
        "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11"
        "\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23",
        LV_ROLLER_MODE_INFINITE);
    lv_roller_set_visible_row_count(timezoneRoller.hours, 2);
    lv_roller_set_selected(timezoneRoller.hours, localTime.tm_hour, LV_ANIM_OFF);

    lv_obj_add_event_cb(timezoneRoller.hours, timezone_roller_event_handler, LV_EVENT_ALL, NULL);

    return obj;
}

lv_obj_t * create_button(lv_obj_t *parent, const char *icon, const char *text, lv_event_cb_t event_handler) {
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_style_bg_color(btn, lv_color_hex(HALO_COLOR_RED), 0);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    if (icon) {
        lv_obj_t * img = lv_img_create(btn);
        lv_img_set_src(img, icon);
    }

    if (text) {
        lv_obj_t * lbl = lv_label_create(btn);
        lv_label_set_text(lbl, text);
        lv_obj_set_style_text_font(lbl, &montserrat_14, 0);
    }

    if (event_handler) {
        lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);
    }

    return btn;
}

// Replaces standings_container content with a spoiler-proof button.
// wasStandings: true = hiding championship standings, false = hiding session results.
void show_spoiler_button(lv_obj_t *container, bool wasStandings) {
    // Cancel any running fade animation and standing timer before touching the container
    lv_anim_del(&style_fade, NULL);
    if (standings_ui_timer) { lv_timer_del(standings_ui_timer); standings_ui_timer = NULL; }

    // Record what we are hiding so the button callback knows what to restore
    noSpoilerWasStandings      = wasStandings;
    noSpoilerLastKnownSession  = wasStandings ? "" : current_results; // standings have no session name

    lv_obj_clean(container);

    // Full-width centring wrapper
    lv_obj_t *wrapper = lv_obj_create(container);
    lv_obj_remove_style_all(wrapper);
    lv_obj_set_size(wrapper, LV_PCT(100), 110);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wrapper,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(wrapper, 8, LV_PART_MAIN);
    lv_obj_remove_flag(wrapper, LV_OBJ_FLAG_SCROLLABLE);

    // Warning icon
    lv_obj_t *icon = lv_label_create(wrapper);
    lv_label_set_text(icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(icon, lv_color_hex(HALO_COLOR_RED), 0);

    lv_obj_t *label = lv_label_create(wrapper);
    lv_label_set_text(label, localized_text->new_results_available);

    // "Show Results" button
    lv_obj_t *btn = lv_btn_create(wrapper);
    lv_obj_set_style_bg_color(btn, lv_color_hex(HALO_COLOR_RED), 0);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, localized_text->show_results);
    lv_obj_set_style_text_font(lbl, &montserrat_14, 0);

    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
        // Set state flags immediately while btn is still alive
        noSpoilerLifted           = true;
        noSpoilerLiftedForSession = noSpoilerLastKnownSession;

        // Defer all UI manipulation — calling lv_obj_clean on an ancestor from
        // within a descendant's event callback causes a double-free in LVGL 9.
        // lv_async_call runs after the current event dispatch fully completes.
        lv_async_call([](void *) {
            if (!noSpoilerLifted) return;  // settings were changed before this ran — abort
            lv_anim_del(&style_fade, NULL);
            if (standings_ui_timer) { lv_timer_del(standings_ui_timer); standings_ui_timer = NULL; }
            lv_obj_clean(standings_container);

            if (noSpoilerWasStandings) {
                populate_standings(standings_container, 0);
                standings_ui_timer = lv_timer_create([](lv_timer_t *t) {
                    animate_standings((lv_obj_t *)lv_timer_get_user_data(t));
                }, 15000, standings_container);
            } else {
                populate_results(standings_container, 0);
                standings_ui_timer = lv_timer_create([](lv_timer_t *t) {
                    animate_results((lv_obj_t *)lv_timer_get_user_data(t));
                }, 15000, standings_container);
            }
        }, nullptr);
    }, LV_EVENT_CLICKED, nullptr);
}

// THE BIG ONE
// @TODO - make nice graphics for Qualy and race results
// @TODO - make results/standings not scrollable
void create_or_reload_race_sessions(bool force_reload) {

  if (!race_styles_initialized) {
      race_styles_initialized = true;

      // ── Row wrapper: the red highlight lives here now ──────────────────────
      lv_style_init(&style_session_row);
      lv_style_set_bg_color(&style_session_row, lv_color_hex(HALO_COLOR_RED));
      // bg_opa is set per-object below (COVER = active, TRANSP = future)
      lv_style_set_border_width(&style_session_row, 0);
      lv_style_set_radius(&style_session_row, 0);
      lv_style_set_pad_all(&style_session_row, 0);
      lv_style_set_pad_gap(&style_session_row, 0);

      // ── Session text label: transparent bg, inherits row's background ──────
      lv_style_init(&style_session_label);
      lv_style_set_text_align(&style_session_label, LV_TEXT_ALIGN_LEFT);
      lv_style_set_text_font(&style_session_label, &montserrat_12);
      lv_style_set_pad_top(&style_session_label, 3);
      lv_style_set_pad_bottom(&style_session_label, 3);
      lv_style_set_pad_left(&style_session_label, 8);
      lv_style_set_pad_right(&style_session_label, 0);
      lv_style_set_bg_opa(&style_session_label, LV_OPA_TRANSP);

      // ── Stripe (chequered flag separator) ─────────────────────────────────
      lv_style_init(&style_stripe);
      lv_style_set_margin_top(&style_stripe, 20);
      lv_style_set_margin_bottom(&style_stripe, 20);
  }

  // Clean container
  lv_anim_del(sessions_container, NULL);
  lv_obj_clean(sessions_container);

  lv_obj_t *session_label;
  RaceSession session;
  RaceSession last_session;

  if (!next_race.sessionCount || next_race.sessionCount == NULL) return;

  // ── Row width: same 90 % of screen that the old single labels used ─────────
  lv_coord_t row_w = (lv_coord_t)(SCREEN_WIDTH - 4);

  // Weather-badge column is only shown when data is available.
  const lv_coord_t WEATHER_W = 48;

  for (int i = 0; i < next_race.sessionCount; i++) {
    session = next_race.sessions[i];
    int j = i + 1 < next_race.sessionCount ? i + 1 : next_race.sessionCount - 1;

    bool started      = hasSessionStarted(session.date, session.time);
    bool started_next = hasSessionStarted(next_race.sessions[j].date,
                                          next_race.sessions[j].time);
    bool is_active    = (started && !started_next) ||
                        (started && i == next_race.sessionCount - 1);

    // ── 1. Flex-row wrapper ─────────────────────────────────────────────────
    lv_obj_t* session_row = lv_obj_create(sessions_container);
    lv_obj_remove_style_all(session_row);
    lv_obj_add_style(session_row, &style_session_row, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_width(session_row, row_w);
    lv_obj_set_height(session_row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(session_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(session_row,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(session_row, LV_OBJ_FLAG_SCROLLABLE);

    // Red-highlight or transparent background
    lv_obj_set_style_bg_opa(session_row,
                            is_active ? LV_OPA_COVER : LV_OPA_TRANSP,
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    // Conditional top/bottom margins (same logic as before, now on the row)
    if (session.name == "Sprint Qualifying" || session.name == "Qualifying") {
        lv_obj_set_style_margin_top(session_row, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (session.name == "Qualifying" && !next_race.isSprintWeekend) {
        lv_obj_set_style_margin_bottom(session_row, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (is_active) last_session = session;

    // ── 2. Session info label (fills available width) ───────────────────────
    session_label = lv_label_create(session_row);
    lv_obj_add_style(session_label, &style_session_label, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_grow(session_label, 1);  // expands to fill space left of badge
    lv_label_set_long_mode(session_label, LV_LABEL_LONG_MODE_SCROLL);

    if (next_race.isSprintWeekend &&
        (session.name == "Sprint Qualifying" || session.name == "Sprint Race")) {
        lv_label_set_text_fmt(session_label,
                              LV_SYMBOL_CHARGE "   %s   " LV_SYMBOL_RIGHT "   %s",
                              getLocalizedSessionName(session),
                              getSessionDateTimeFormatted(session.date, session.time));
    } else {
        lv_label_set_text_fmt(session_label,
                              "%s  " LV_SYMBOL_RIGHT "  %s",
                              getLocalizedSessionName(session),
                              getSessionDateTimeFormatted(session.date, session.time));
    }

    // ── 3. Weather badge (right-side, fixed width) ───────────────────────────
    // Only rendered when Open-Meteo data has been fetched for this slot.
    if (weather_fetched && i < 10 && session_weather[i].valid) {
        lv_obj_t* w_lbl = lv_label_create(session_row);

        // Icon glyph + temperature: e.g. "☀ 22°"
        // The icon is rendered in weather_icons_16; the degree+number are
        // rendered as plain text with the same font (digits are in every font).
        char w_buf[16];
        snprintf(w_buf, sizeof(w_buf), "%s %d\xC2\xB0",
                 getWeatherIcon(session_weather[i].wmo_code),
                 (int)session_weather[i].temp_c);
        lv_label_set_text(w_lbl, w_buf);

        lv_obj_set_width(w_lbl, WEATHER_W);
        lv_obj_set_height(w_lbl, LV_SIZE_CONTENT);
        lv_label_set_long_mode(w_lbl, LV_LABEL_LONG_MODE_CLIP);

        lv_obj_set_style_text_font(w_lbl,  &weather_icons_12, LV_PART_MAIN);
        lv_obj_set_style_text_align(w_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_obj_set_style_pad_right(w_lbl,  4, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(w_lbl,     LV_OPA_TRANSP, LV_PART_MAIN);

        lv_color_t badge_color = is_active
            ? lv_color_white()
            : getWeatherColor(session_weather[i].wmo_code);
        lv_obj_set_style_text_color(w_lbl, badge_color, LV_PART_MAIN);
    }
  }

  // ── No Spoiler: reset lift when the active session has changed ──────────
  noSpoilerLastKnownSession = last_session.name; // always keep this current
  if (noSpoilerLifted && last_session.name != noSpoilerLiftedForSession) {
      noSpoilerLifted           = false;
      noSpoilerLiftedForSession = "";
  }

  // Start Finish Separator
  lv_obj_t *stripe = create_chequered_stripe(sessions_container);
  lv_obj_add_style(stripe, &style_stripe, LV_PART_MAIN | LV_STATE_DEFAULT);

  // The rest of your logic (fetch standings, timers, etc.) stays unchanged
  if (!standings_loaded_once) {
      fetch_f1_driver_standings();
      if (!standings_loaded_once) return;
  }

  if (!hasRaceWeekendStarted() && (force_reload || millis() > last_checked_session_results + check_delay)) {
      last_checked_session_results = millis();

      if (standings_ui_timer) { lv_timer_del(standings_ui_timer); standings_ui_timer = NULL; }
      lv_anim_del(&style_fade, NULL); //was standings_container
      lv_obj_clean(standings_container);

      if (noSpoilerModeActive && !noSpoilerLifted) {
          show_spoiler_button(standings_container, true);  // true = hiding standings
      } else {
          populate_standings(standings_container, 0);

          standings_ui_timer = lv_timer_create([](lv_timer_t *t) {
              //Serial.println("Inside Standings Animation Timer");
              animate_standings((lv_obj_t *)lv_timer_get_user_data(t));
          }, 15000, standings_container);
      }

      check_delay = 30 * 60000;
      return;
  }

  if (! hasRaceWeekendStarted()) return;

  // race weekend has started
  Serial.println("[UI] Race Weekend Has Started, following with results logic");

  current_results = last_session.name;
  //Serial.println("[UI] Current results set");

  if (last_session.name == "FP1" || last_session.name == "FP2" || last_session.name == "FP3") {
    Serial.println("[UI] Inside FP123");
    
    if (! hasFreePracticeFinished(last_session.date, last_session.time)) {
      Serial.println("[UI] Inside FP not finished");
      check_delay = 5 * 60000; //every 5 minutes
      if (standings_ui_timer) lv_timer_del(standings_ui_timer);
      standings_ui_timer = NULL;
      lv_anim_del(&style_fade, NULL); //was standings_container
      lv_obj_clean(standings_container);
      return;
    } 

    // only run this part once every 30 minutes
    if (force_reload || millis() > last_checked_session_results + check_delay) {
      last_checked_session_results = millis();

      // clean container
      if (standings_ui_timer) lv_timer_del(standings_ui_timer);
      standings_ui_timer = NULL;
      lv_anim_del(&style_fade, NULL); //was standings_container
      lv_obj_clean(standings_container);

      Serial.println("[UI] Running Results API Check");
      bool got_results = getLastSessionResults(results);

      if (!got_results || !results_loaded_once) {
        Serial.println("[UI] API Check Run but encountered an error");
        if (last_results != current_results) return;
      } else {
        last_results = current_results;
      }

      check_delay = 1800000;
    
      if (!results_loaded_once) return;

      if (noSpoilerModeActive && !noSpoilerLifted) {
          show_spoiler_button(standings_container, false);  // false = hiding results
      } else {
          populate_results(standings_container, 0);

          standings_ui_timer = lv_timer_create([](lv_timer_t * t){
              animate_results((lv_obj_t *)lv_timer_get_user_data(t));
          }, 15000, standings_container);
      }

      Serial.println("[UI] Results for Free Practice Rendered");
    } else {
        Serial.println("[UI] Not yet time to check for session's results");
    }

    return;
  }

  // only run this part once every 30 minutes
  if (!force_reload && millis() < last_checked_session_results + 1800000ULL && results_checked_once) return; // if time now is less than time of next check then stop
  results_checked_once = true;
  last_checked_session_results = millis();

  if (standings_ui_timer) lv_timer_del(standings_ui_timer);
  standings_ui_timer = NULL;
  lv_anim_del(&style_fade, NULL); //was standings_container
  lv_obj_clean(standings_container);

  Serial.println("[UI] Running Results API Check");

  bool got_results = getLastSessionResults(results);

  if (!got_results) {
    Serial.println("[UI] API Check Run but encountered an error");
    if (last_results != current_results) return;
  } else {
    last_results = current_results;
  }

  if (last_session.name == "Sprint Qualifying" || last_session.name == "Sprint Race" || last_session.name == "Qualifying" || last_session.name == "Race") {
    // clean container
    if (standings_ui_timer) lv_timer_del(standings_ui_timer);
    standings_ui_timer = NULL;
    lv_anim_del(&style_fade, NULL); //was standings_container
    lv_obj_clean(standings_container);

    // show Race Grid when available
    if (!results_loaded_once) return;

    if (noSpoilerModeActive && !noSpoilerLifted) {
        show_spoiler_button(standings_container, false);  // false = hiding results
    } else {
        populate_results(standings_container, 0);

        standings_ui_timer = lv_timer_create([](lv_timer_t * t){
            animate_results((lv_obj_t *)lv_timer_get_user_data(t));
        }, 15000, standings_container);
    }
    return;
  }
}

// Runs once or when language is changed
void create_or_reload_race_ui() {
  Serial.println("[UI] Creating or Reloading Race UI...");
  lv_anim_del(tabs.race, NULL);
  lv_anim_del(sessions_container, NULL);
  lv_anim_del(&style_fade, NULL); //was standings_container

  lv_obj_clean(tabs.race);

  //----------//
  //   DATE   //
  //----------//

  racetab_labels.date = lv_label_create(tabs.race);
  lv_label_set_text(racetab_labels.date, "");

  lv_obj_align(racetab_labels.date, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_text_align(racetab_labels.date, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(racetab_labels.date, LV_LABEL_LONG_MODE_DOTS); 
  lv_obj_set_width(racetab_labels.date, 0.9 * SCREEN_WIDTH);
  lv_obj_set_style_bg_opa(racetab_labels.date, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(racetab_labels.date, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(racetab_labels.date, lv_color_hex(HALO_COLOR_RED), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(racetab_labels.date, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(racetab_labels.date, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(racetab_labels.date, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(racetab_labels.date, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_left(racetab_labels.date, 4, LV_PART_MAIN | LV_STATE_DEFAULT);


  lv_obj_set_style_text_font(racetab_labels.date, &montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);


  //---------//
  //  CLOCK  //
  //---------//

  racetab_labels.clock = lv_label_create(tabs.race);

  lv_obj_align(racetab_labels.clock, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_text_align(racetab_labels.clock, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(racetab_labels.clock, LV_LABEL_LONG_MODE_CLIP); 
  lv_obj_set_style_bg_opa(racetab_labels.clock, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(racetab_labels.clock, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_left(racetab_labels.clock, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_set_style_text_font(racetab_labels.clock, &montserrat_38, LV_PART_MAIN | LV_STATE_DEFAULT);


  //------------//
  //  RACENAME  //
  //------------//

  racetab_labels.race_name = lv_label_create(tabs.race);
  lv_label_set_text(racetab_labels.race_name, "");

  lv_obj_align(racetab_labels.race_name, LV_ALIGN_TOP_MID, 0, 60);
  lv_obj_set_style_width(racetab_labels.race_name, LV_SIZE_CONTENT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(racetab_labels.race_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_long_mode(racetab_labels.race_name, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR); 

  lv_obj_set_style_text_font(racetab_labels.race_name, &montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);


  sessions_container = lv_obj_create(tabs.race);
  lv_obj_remove_style_all(sessions_container);
  lv_obj_set_style_width(sessions_container, SCREEN_WIDTH, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_height(sessions_container, LV_SIZE_CONTENT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(sessions_container, LV_ALIGN_TOP_MID, 0, 110);
  lv_obj_set_flex_flow(sessions_container, LV_FLEX_FLOW_COLUMN);
  //create_or_reload_race_sessions();

  standings_container = lv_obj_create(tabs.race);
  lv_obj_remove_style_all(standings_container);
  lv_obj_set_layout(standings_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(standings_container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(standings_container, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_width(standings_container, SCREEN_WIDTH, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_scroll_snap_y(standings_container, LV_SCROLL_SNAP_START);
  lv_obj_align(standings_container, LV_ALIGN_TOP_MID, - SCREEN_WIDTH * 0.025, 295);

  Serial.println("[UI] Creating or Reloading Race UI -- DONE!");
}

// Runs once every 5 minutes to fetch latest article and update the news tab -- @TODO -> maybe add some flavour to the graphics?
void create_or_reload_news_ui(lv_timer_t *timer) {
    String title = "", link = "", desc = "";
    static String last_link = "";
    bool notifyNewArticle = false;

    // If user is on News tab, ensure pulse is always fully stopped.
    if (home_tabs && lv_tabview_get_tab_active(home_tabs) == NEWS_TAB_INDEX) {
        hasUnreadNews = false;
        sync_news_tab_pulse_state();
    }

    // Always clear the tab first so feed switches never leave stale content visible.
    lv_obj_clean(tabs.news);

    if (!getLatestNews(title, link, desc) || (title == "" && desc == "") || link == "") {
        lv_obj_t *cont = lv_obj_create(tabs.news);
        lv_obj_remove_style_all(cont);
        lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
        lv_obj_center(cont);
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *label = lv_label_create(cont);
        lv_label_set_text(label, localized_text->news_unavailable_for_selected_feed);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_width(label, LV_PCT(90));
        lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);

        return;
    }

    if (last_link != link) {
        notifyNewArticle = true;
        last_link = link;
    }

    Serial.println("[UI] It appears we have a winner (news fetched)");

    if (!news_styles_initialized) {
        news_styles_initialized = true;

        // Container style
        lv_style_init(&style_news_container);
        lv_style_set_pad_row(&style_news_container, 20);

        // Title label style
        lv_style_init(&style_news_title);
        lv_style_set_text_font(&style_news_title, &montserrat_20);
        lv_style_set_text_align(&style_news_title, LV_TEXT_ALIGN_LEFT);

        // Description label style
        lv_style_init(&style_news_desc);
        lv_style_set_text_font(&style_news_desc, &montserrat_12);
        lv_style_set_text_align(&style_news_desc, LV_TEXT_ALIGN_LEFT);
        lv_style_set_border_width(&style_news_desc, 3);
        lv_style_set_border_side(&style_news_desc, LV_BORDER_SIDE_LEFT);
        lv_style_set_border_color(&style_news_desc, lv_color_hex(HALO_COLOR_RED));
        lv_style_set_pad_left(&style_news_desc, 10);

        // QR caption style
        lv_style_init(&style_qr_caption);
        lv_style_set_text_font(&style_qr_caption, &montserrat_12);
        lv_style_set_text_align(&style_qr_caption, LV_TEXT_ALIGN_CENTER);
        lv_style_set_bg_color(&style_qr_caption, lv_color_hex(HALO_COLOR_RED));
        lv_style_set_bg_opa(&style_qr_caption, LV_OPA_COVER);
        lv_style_set_width(&style_qr_caption, 100);
    }

    lv_obj_t *cont = lv_obj_create(tabs.news);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont,
                          LV_FLEX_ALIGN_START,   /* main axis (vertical in column) */
                          LV_FLEX_ALIGN_CENTER,  /* cross axis (horizontal) */
                          LV_FLEX_ALIGN_CENTER); /* track cross axis */
    lv_obj_add_style(cont, &style_news_container, LV_PART_MAIN);

    Serial.println("[UI] News Container Created");

    // Title label
    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, title.c_str());
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_height(label, LV_SIZE_CONTENT);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_add_style(label, &style_news_title, LV_PART_MAIN);

    // Description label
    label = lv_label_create(cont);
    lv_label_set_text(label, desc.c_str());
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_height(label, LV_SIZE_CONTENT);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_add_style(label, &style_news_desc, LV_PART_MAIN);

    //Serial.println("News Desc Label Created");

    // QR code container
    lv_obj_t *qr_cont = lv_obj_create(cont);
    lv_obj_remove_style_all(qr_cont);
    lv_obj_set_width(qr_cont, LV_PCT(100));
    lv_obj_set_flex_grow(qr_cont, 1);

    // QR code itself
    lv_obj_t *qr = lv_qrcode_create(qr_cont);
    lv_qrcode_set_size(qr, 100);
    lv_qrcode_update(qr, link.c_str(), strlen(link.c_str()));
    lv_obj_center(qr);

    // Caption under QR
    lv_obj_t *caption = lv_label_create(qr_cont);
    lv_label_set_text(caption, localized_text->scan_to_read);
    lv_obj_add_style(caption, &style_qr_caption, LV_PART_MAIN);
    lv_obj_align_to(caption, qr, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
    lv_label_set_long_mode(caption, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);

    //Serial.println("News QR Code Created");

    if (notifyNewArticle) {
        Serial.println("[UI] Article Link is new, running notification routine for new article fetched.");
        playNotificationSound();
        if (home_tabs && lv_tabview_get_tab_active(home_tabs) != NEWS_TAB_INDEX) {
            hasUnreadNews = true;
            sync_news_tab_pulse_state();
        } else {
            hasUnreadNews = false;
            sync_news_tab_pulse_state();
        }
        //lv_tabview_set_active(home_tabs, 1, LV_ANIM_ON); // switch to article tab -- place under a bool switch in settings
    }
}

// Runs once or when language is changed
void create_or_reload_settings_ui() {
  lv_obj_clean(tabs.settings);

  timezoneRoller = {nullptr, nullptr};
  timezone_override_switch = nullptr;

  lv_obj_t *cont = lv_obj_create(tabs.settings);
  lv_obj_remove_style_all(cont);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100)); //LV_SIZE_CONTENT
  lv_obj_center(cont);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont,
                        LV_FLEX_ALIGN_START,   /* main axis (vertical in column) */
                        LV_FLEX_ALIGN_CENTER,  /* cross axis (horizontal) */
                        LV_FLEX_ALIGN_CENTER); /* track cross axis */

  lv_obj_set_style_pad_row(cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Scrollbar styles
  lv_obj_set_style_bg_color(tabs.settings, lv_color_black(), LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_color(tabs.settings, lv_color_hex(HALO_COLOR_RED), LV_PART_SCROLLBAR); // | LV_STATE_SCROLLED
  lv_obj_set_style_width(tabs.settings, 3, LV_PART_SCROLLBAR);

  // Language
  language_selector = create_language_selector(cont);
  lv_obj_add_event_cb(language_selector, language_selection_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

  // -- Race Settings --
  create_settings_divider(cont, localized_text->race);
  // News Feed Source
  news_feed_selector = create_news_feed_selector(cont);
  lv_obj_add_event_cb(news_feed_selector, news_feed_selection_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

  // No Spoiler Mode
  no_spoiler_switch = create_switch(cont, LV_SYMBOL_WARNING, localized_text->no_spoiler_mode, noSpoilerModeActive);
  lv_obj_add_event_cb(no_spoiler_switch, no_spoiler_switch_handler, LV_EVENT_VALUE_CHANGED, NULL);
  // News pulse effect
  news_pulse_switch = create_switch(cont, LV_SYMBOL_BELL, localized_text->news_pulse_effect_label, newsPulseEnabled);
  lv_obj_add_event_cb(news_pulse_switch, news_pulse_switch_handler, LV_EVENT_VALUE_CHANGED, NULL);

  // -- Timezone Settings --
  create_settings_divider(cont, localized_text->time_settings);
  create_timezone_roller(cont, LV_SYMBOL_LOOP, localized_text->enable_timezone_override);

  // -- Display Settings --
  create_settings_divider(cont, localized_text->display);
  // Brightness
  brightness_slider = create_slider(cont, LV_SYMBOL_IMAGE, localized_text->brightness, 5, 255, brightness);
  lv_obj_add_event_cb(brightness_slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(brightness_slider, brightness_slider_event_cb, LV_EVENT_RELEASED, NULL);

  // Night Mode
  create_time_roller(cont, LV_SYMBOL_EYE_CLOSE, localized_text->night_mode);

  // Night Mode Brightness
  night_brightness_slider = create_slider(cont, LV_SYMBOL_IMAGE, localized_text->night_brightness, 5, 255, night_brightness);
  lv_obj_add_event_cb(night_brightness_slider, night_brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(night_brightness_slider, night_brightness_slider_event_cb, LV_EVENT_RELEASED, NULL);

  // Restart WiFi Manager Button
  // Doesn't work currently, maybe can be done by setting a global flag, save it to flash, then restart the device
  //create_button(cont, false, LV_SYMBOL_WIFI "Restart WiFi Manager", restart_wifimanager_event_handler);

  create_settings_divider(cont, nullptr);

  // Show App Version
    lv_obj_t *version_label = lv_label_create(cont);
    lv_label_set_text_fmt(version_label, "Halo F1 @ FW Version %s", fw_version);
    lv_obj_set_style_text_align(version_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_width(version_label, LV_PCT(100));
    lv_obj_set_style_text_font(version_label, &montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Show "Made by" with heart icon
    lv_obj_t *made_by_label = lv_label_create(cont);
    lv_label_set_text_fmt(made_by_label, "Made with *heart* by Fabio Rossato");
    lv_obj_set_style_text_align(made_by_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_width(made_by_label, LV_PCT(100));
    lv_obj_set_style_text_font(made_by_label, &montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(made_by_label, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(made_by_label, 20, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Button "Need Help?" that opens a modal
    lv_obj_t *help_button = create_button(cont, nullptr, localized_text->need_help_button_text, [](lv_event_t *e) {
        show_notification_popup(localized_text->help_dialog_title, localized_text->help_dialog_message, "https://discord.gg/qAKaPa5n5m");
    });

    lv_obj_set_style_margin_bottom(help_button, 20, LV_PART_MAIN | LV_STATE_DEFAULT);

 // Show QR Code with device UUID so the user can scan with the phone to copy
 // Not needed for now
    /*lv_obj_t *qr = lv_qrcode_create(cont);
    lv_qrcode_set_size(qr, 100);
    String uuid = getDeviceUUID();
    lv_qrcode_update(qr, uuid.c_str(), uuid.length());
    lv_obj_center(qr);
    lv_obj_set_style_border_color(qr, lv_color_white(), 0);
    lv_obj_set_style_border_width(qr, 5, 0);

    // Caption under QR
    lv_obj_t *qr_caption = lv_label_create(cont);
    lv_label_set_text(qr_caption, "Scan QR Code to Copy Device UUID");
    lv_obj_set_style_text_align(qr_caption, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_width(qr_caption, LV_PCT(100));
    lv_obj_set_style_text_font(qr_caption, &montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(qr_caption, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(qr_caption, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    */

}

// ── STANDINGS TAB ─────────────────────────────────────────────────────────────

static void populate_full_driver_standings(lv_obj_t * container) {
    if (current_season.driver_count == 0) {
        lv_obj_t * lbl = lv_label_create(container);
        lv_label_set_text(lbl, "No data");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
        return;
    }
    for (int i = 0; i < current_season.driver_count && i < 30; i++) {
        String points = String(current_season.driver_standings[i].points) + " pt.";
        create_standings_row(
            container,
            current_season.driver_standings[i].position,
            current_season.driver_standings[i].name,
            current_season.driver_standings[i].surname,
            points,
            current_season.driver_standings[i].constructorId
        );
    }
}

static void populate_full_constructor_standings(lv_obj_t * container) {
    if (current_season.team_count == 0) {
        lv_obj_t * lbl = lv_label_create(container);
        lv_label_set_text(lbl, "No data");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
        return;
    }
    for (int i = 0; i < current_season.team_count && i < 12; i++) {
        String points = String(current_season.team_standings[i].points) + " pt.";
        create_standings_row(
            container,
            current_season.team_standings[i].position,
            current_season.team_standings[i].name,
            "",
            points,
            current_season.team_standings[i].id
        );
    }
}

static void standings_tab_update_buttons() {
    if (standings_tab_showing_drivers) {
        lv_obj_add_style(standings_tab_btn_drivers, &style_standings_tab_btn_active, 0);
        lv_obj_remove_style(standings_tab_btn_drivers, &style_standings_tab_btn_inactive, 0);
        lv_obj_add_style(standings_tab_btn_constructors, &style_standings_tab_btn_inactive, 0);
        lv_obj_remove_style(standings_tab_btn_constructors, &style_standings_tab_btn_active, 0);
    } else {
        lv_obj_add_style(standings_tab_btn_constructors, &style_standings_tab_btn_active, 0);
        lv_obj_remove_style(standings_tab_btn_constructors, &style_standings_tab_btn_inactive, 0);
        lv_obj_add_style(standings_tab_btn_drivers, &style_standings_tab_btn_inactive, 0);
        lv_obj_remove_style(standings_tab_btn_drivers, &style_standings_tab_btn_active, 0);
    }
}

static void standings_tab_repopulate() {
    lv_obj_clean(standings_tab_list);
    if (standings_tab_showing_drivers) {
        populate_full_driver_standings(standings_tab_list);
    } else {
        populate_full_constructor_standings(standings_tab_list);
    }
}

static void standings_tab_drivers_clicked(lv_event_t * e) {
    if (standings_tab_showing_drivers) return;
    standings_tab_showing_drivers = true;
    standings_tab_update_buttons();
    standings_tab_repopulate();
}

static void standings_tab_constructors_clicked(lv_event_t * e) {
    if (!standings_tab_showing_drivers) return;
    standings_tab_showing_drivers = false;
    standings_tab_update_buttons();
    standings_tab_repopulate();
}

void create_or_reload_standings_tab_ui() {
    Serial.println("[UI] Creating or Reloading Standings Tab UI...");
    lv_obj_clean(tabs.standings);

    // ── Lazy style init ──
    if (!standings_tab_styles_initialized) {
        standings_tab_styles_initialized = true;

        lv_style_init(&style_standings_tab_btn_active);
        lv_style_set_bg_opa(&style_standings_tab_btn_active, LV_OPA_COVER);
        lv_style_set_bg_color(&style_standings_tab_btn_active, lv_color_black());
        lv_style_set_text_color(&style_standings_tab_btn_active, lv_color_white());
        lv_style_set_border_side(&style_standings_tab_btn_active, LV_BORDER_SIDE_BOTTOM);
        lv_style_set_border_width(&style_standings_tab_btn_active, 3);
        lv_style_set_border_color(&style_standings_tab_btn_active, lv_color_hex(HALO_COLOR_RED));
        lv_style_set_radius(&style_standings_tab_btn_active, 0);

        lv_style_init(&style_standings_tab_btn_inactive);
        lv_style_set_bg_opa(&style_standings_tab_btn_inactive, LV_OPA_COVER);
        lv_style_set_bg_color(&style_standings_tab_btn_inactive, lv_color_hex(0x000000));
        lv_style_set_text_color(&style_standings_tab_btn_inactive, lv_color_hex(0x666666));
        lv_style_set_border_width(&style_standings_tab_btn_inactive, 3);
        lv_style_set_border_side(&style_standings_tab_btn_inactive, LV_BORDER_SIDE_BOTTOM);
        lv_style_set_border_color(&style_standings_tab_btn_inactive, lv_color_hex(0x000000));
        lv_style_set_radius(&style_standings_tab_btn_inactive, 0);


        lv_style_init(&style_standings_tab_list);
        lv_style_set_bg_opa(&style_standings_tab_list, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_standings_tab_list, 0);
        lv_style_set_pad_all(&style_standings_tab_list, 0);
        lv_style_set_pad_gap(&style_standings_tab_list, 2);
    }

    // ── Toggle button row ──
    lv_obj_t * btn_row = lv_obj_create(tabs.standings);
    lv_obj_remove_style_all(btn_row);
    lv_obj_set_size(btn_row, LV_PCT(100), 32);
    lv_obj_set_layout(btn_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(btn_row, LV_ALIGN_TOP_MID, 0, 0);

    // Drivers button
    standings_tab_btn_drivers = lv_button_create(btn_row);
    lv_obj_set_size(standings_tab_btn_drivers, LV_PCT(50), 30);
    lv_obj_t * lbl_drv = lv_label_create(standings_tab_btn_drivers);
    lv_label_set_text(lbl_drv, localized_text->driver_standings_title);
    lv_obj_set_style_text_font(lbl_drv, &montserrat_12, 0);
    lv_obj_center(lbl_drv);
    lv_obj_add_event_cb(standings_tab_btn_drivers, standings_tab_drivers_clicked, LV_EVENT_CLICKED, NULL);

    // Constructors button
    standings_tab_btn_constructors = lv_button_create(btn_row);
    lv_obj_set_size(standings_tab_btn_constructors, LV_PCT(50), 30);
    lv_obj_t * lbl_cst = lv_label_create(standings_tab_btn_constructors);
    lv_label_set_text(lbl_cst, localized_text->team_standings_title);
    lv_obj_set_style_text_font(lbl_cst, &montserrat_12, 0);
    lv_obj_center(lbl_cst);
    lv_obj_add_event_cb(standings_tab_btn_constructors, standings_tab_constructors_clicked, LV_EVENT_CLICKED, NULL);

    // Apply initial button styles
    standings_tab_showing_drivers = true;
    standings_tab_update_buttons();

    // ── Scrollable list container ──
    standings_tab_list = lv_obj_create(tabs.standings);
    lv_obj_remove_style_all(standings_tab_list);
    lv_obj_add_style(standings_tab_list, &style_standings_tab_list, 0);
    lv_obj_set_size(standings_tab_list, SCREEN_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_style_max_height(standings_tab_list, 390, 0);
    lv_obj_set_layout(standings_tab_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(standings_tab_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_align(standings_tab_list, LV_ALIGN_TOP_MID, 0, 34);
    lv_obj_add_flag(standings_tab_list, LV_OBJ_FLAG_SCROLLABLE);

    // Populate with driver standings by default
    populate_full_driver_standings(standings_tab_list);

    Serial.println("[UI] Creating or Reloading Standings Tab UI -- DONE!");
}

// Runs once
lv_obj_t * create_main_tabview(lv_obj_t * screen) {
    lv_obj_t * tabview;
    tabview = lv_tabview_create(screen);
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_BOTTOM);
    lv_tabview_set_tab_bar_size(tabview, 48);


    // Get the tab bar (button container)
    lv_obj_t * tab_bar = lv_tabview_get_tab_bar(tabview);

    // Set the font for all tab buttons
    lv_obj_set_style_text_font(tab_bar, &f1_symbols_28, LV_PART_MAIN); //LV_PART_ITEMS
    lv_obj_set_style_bg_color(tab_bar, lv_color_black(), 0);

    lv_obj_t * content = lv_tabview_get_content(tabview);
    lv_obj_set_style_bg_color(tabview, lv_color_black(), 0);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    tabs.race = lv_tabview_add_tab(tabview, F1_SYMBOL_CHEQUERED_FLAG);
    tabs.standings = lv_tabview_add_tab(tabview, F1_SYMBOL_RANKING);
    tabs.news = lv_tabview_add_tab(tabview, F1_SYMBOL_BARS);
    tabs.settings = lv_tabview_add_tab(tabview, F1_SYMBOL_WRENCH);

    lv_obj_set_style_pad_all(tabs.race, 0, 0);
    lv_obj_set_style_pad_all(tabs.standings, 0, 0);

    int tab_count = lv_tabview_get_tab_count(tabview);
    for(int i = 0; i < tab_count; i++) {
        lv_obj_t * button = lv_obj_get_child(tab_bar, i);
        lv_obj_set_style_bg_color(button, lv_color_black(), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_side(button, LV_BORDER_SIDE_TOP, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_width(button, 3, LV_PART_MAIN | LV_STATE_CHECKED);
        if (i == NEWS_TAB_INDEX) {
            news_tab_button = button;
            lv_obj_add_event_cb(news_tab_button, news_tab_button_clicked_handler, LV_EVENT_ALL, NULL);
            sync_news_tab_pulse_state();
        }
    }

    return tabview;
}

// Runs once and terminates by loading WiFi Manager info screen
void create_ui_skeleton() {
  screen.wifi = lv_obj_create(NULL);
  screen.home = lv_obj_create(NULL);
  //screen.settings = lv_obj_create(NULL);

  set_custom_theme();

  lv_obj_set_style_bg_color(screen.wifi, lv_color_hex(0x000000), 0); // Set to a dark background
  lv_obj_set_style_bg_opa(screen.wifi, LV_OPA_COVER, 0);

  lv_obj_set_style_bg_color(screen.home, lv_color_hex(0x000000), 0); // Set to a dark background
  lv_obj_set_style_bg_opa(screen.home, LV_OPA_COVER, 0);

  //lv_obj_set_style_bg_color(screen.settings, lv_color_hex(0x000000), 0); // Set to a dark background
  //lv_obj_set_style_bg_opa(screen.settings, LV_OPA_COVER, 0);

  // this only needs to be created once
  lv_obj_t * label = lv_label_create(screen.wifi);
  lv_label_set_text(label, localized_text->wifi_connection_needed);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_height(label, LV_SIZE_CONTENT);
  lv_obj_set_width(label, SCREEN_WIDTH);
  lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP); 
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t *qr = lv_qrcode_create(screen.wifi);
  lv_qrcode_set_size(qr, 100);
  String discord_invite = "https://discord.gg/qAKaPa5n5m";
  lv_qrcode_update(qr, discord_invite.c_str(), discord_invite.length());
  lv_obj_set_style_border_color(qr, lv_color_white(), 0);
  lv_obj_set_style_border_width(qr, 5, 0);
  lv_obj_align(qr, LV_ALIGN_TOP_MID, 0, 20);

  // Caption under QR
  lv_obj_t *qr_caption = lv_label_create(screen.wifi);
  lv_label_set_text(qr_caption, localized_text->wifi_help_qr_caption);
  lv_obj_set_style_text_align(qr_caption, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_width(qr_caption, LV_PCT(100));
  lv_obj_set_style_text_font(qr_caption, &montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(qr_caption, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(qr_caption, LV_ALIGN_TOP_MID, 0, 150);
    

  home_tabs = create_main_tabview(screen.home);

  create_or_reload_race_ui();
  create_or_reload_standings_tab_ui();
  create_or_reload_settings_ui();

  lv_screen_load(screen.wifi);
  lv_timer_periodic_handler();
}

// Runs once after WiFi connection is established
void post_wifi_ui_creation() {
  create_or_reload_race_sessions();
  create_or_reload_standings_tab_ui();
}