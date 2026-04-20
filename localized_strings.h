typedef struct {
  const char* months[12];
  const char* short_months[12];
  const char* days[7];
  const char* short_days[7];
  const char* wifi_connection_needed;
  const char* wifi_connection_success;
  const char* wifi_connection_failed;
  const char* ok;
  const char* yes;
  const char* no;
  const char* close;
  const char* cancel;
  const char* success;
  const char* fail;
  const char* reload_clock;
  const char* clock_updated;
  const char* FP1;
  const char* FP2;
  const char* FP3;
  const char* qualifying;
  const char* Q1;
  const char* Q2;
  const char* Q3;
  const char* race;
  const char* sprint_race;
  const char* sprint_q;
  const char* free_practice;
  const char* free_practice_1;
  const char* free_practice_2;
  const char* free_practice_3;
  const char* next_race;
  const char* next_session;
  const char* sprint_weekend;
  const char* driver_standings_title;
  const char* team_standings_title;
  const char* scan_to_read;
  const char* scan_to_open;
  const char* language;
  const char* language_name;
  const char* language_name_eng;
  const char* brightness;
  const char* night_mode;
  const char* night_brightness;
  const char* update_available_title;
  const char* no_spoiler_mode;
  const char* new_results_available;
  const char* show_results;
  const char* display;
  const char* time_settings;
  const char* enable_timezone_override;
  const char* need_help_button_text;
  const char* help_dialog_title;
  const char* help_dialog_message;
  const char* wifi_help_qr_caption;
} LanguageStrings;

const LanguageStrings language_strings_en PROGMEM = {
  {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"},
  {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"},
  {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"},
  {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
  "WiFi access loading...",
  "Successfully connected to WiFi!",
  "Failed to connect to WiFi, connect to \"Halo-F1\" Network from your phone to set the credentials.\n\nIf the captive portal doesn't open automatically, open your browser and navigate to %s",
  "Ok",
  "Yes",
  "No",
  "Close",
  "Cancel",
  "Success",
  "Fail",
  "Reload clock",
  "Clock has been updated!",
  "FP1",
  "FP2",
  "FP3",
  "Qualifying",
  "Q1",
  "Q2",
  "Q3",
  "Race",
  "Race", // was "Sprint Race"
  "Qualifying", // was "Sprint Qualy"
  "Free Practice",
  "Free Practice 1",
  "Free Practice 2",
  "Free Practice 3",
  "Upcoming Grand Prix",
  "Upcoming Session",
  "Sprint Weekend",
  "DRIVER STANDINGS",
  "TEAM STANDINGS",
  "Scan to Read",
  "Scan to Open",
  "Language",
  "English",
  "English",
  "Brightness",
  "Night Mode",
  "Night Mode Brightness",
  "Update Available!",
  "No Spoiler Mode",
  "New Results Available",
  "Show Results",
  "Display",
  "Time",
  "Enable Timezone Override",
  LV_SYMBOL_CHARGE " Need Help? Click here!",
  LV_SYMBOL_CHARGE " We got you!",
  "Scan to join our Discord server and get support!",
  LV_SYMBOL_CHARGE " Need Help? We got you! Scan to join our Discord server and get support!",
};

const LanguageStrings language_strings_it PROGMEM = {
  {"Gennaio", "Febbraio", "Marzo", "Aprile", "Maggio", "Giugno", "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "Dicembre"},
  {"Gen", "Feb", "Mar", "Apr", "Giu", "Lug", "Ago", "Set", "Ott", "Nov", "Dic"},
  {"Domenica", "Lunedì", "Martedì", "Mercoledì", "Giovedì", "Venerdì", "Sabato"},
  {"Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"},
  "Accesso WiFi in corso...",
  "Collegamento WiFi riuscito!",
  "Collegamento WiFi fallito, connettiti alla rete \"Halo-F1\" dal tuo cellulare per impostare i dati.\n\nSe il captive portal non si apre automaticamente, apri il browser e naviga a %s",
  "Ok",
  "Sì",
  "No",
  "Chiudi",
  "Annulla",
  "Successo",
  "Problemi problemi",
  "Aggiorna orologio",
  "L'orologio e' stato aggiornato!",
  "PL1",
  "PL2",
  "PL3",
  "Qualifiche",
  "Q1",
  "Q2",
  "Q3",
  "Gara",
  "Gara", // sprint race
  "Qualifiche", // sprint qualifying
  "Prove Libere",
  "Prove Libere 1",
  "Prove Libere 2",
  "Prove Libere 3",
  "Prossimo Gran Premio",
  "Prossima Sessione",
  "Sprint Weekend",
  "CLASSIFICA PILOTI",
  "CLASSIFICA TEAM",
  "Scansiona per leggere",
  "Scansiona per aprire",
  "Lingua",
  "Italiano",
  "Italian",
  "Luminosità",
  "Modalità Notturna",
  "Luminosità Notturna",
  "Aggiornamento Disponibile!",
  "Modalità No Spoiler",
  "Nuovi Risultati Disponibili",
  "Mostra Risultati",
  "Schermo",
  "Orario",
  "Abilita Override Fuso Orario",
  LV_SYMBOL_CHARGE " Serve aiuto? Clicca qui!",
  LV_SYMBOL_CHARGE " Ci pensiamo noi!",
  "Scansiona per unirti al nostro server Discord e ricevere supporto!",
  LV_SYMBOL_CHARGE " Serve aiuto? Ci pensiamo noi! Scansiona per unirti al nostro server Discord e ricevere supporto!",
};

const LanguageStrings language_strings_es PROGMEM = {
  {"Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"},
  {"Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"},
  {"Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sabado"},
  {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"},
  "Cargando acceso WiFi...",
  "¡Conectado a WiFi correctamente!",
  "Error al conectar a WiFi, conéctese a la red \"Halo-F1\" desde su teléfono para establecer las credenciales.\n\nSi el captive portal no se abre automáticamente, abra su navegador y vaya a %s",
  "Ok",
  "Si",
  "No",
  "Cerrar",
  "Cancelar",
  "Exito",
  "Error",
  "Recargar reloj",
  "¡El reloj ha sido actualizado!",
  "FP1",
  "FP2",
  "FP3",
  "Clasificacion",
  "Q1",
  "Q2",
  "Q3",
  "Carrera",
  "Carrera", // sprint race
  "Clasificacion", // sprint qualifying
  "Entrenamiento libre",
  "Entrenamiento libre 1",
  "Entrenamiento libre 2",
  "Entrenamiento libre 3",
  "Proximo Gran Premio",
  "Proxima sesion",
  "Fin de semana Sprint",
  "PILOTOS",
  "EQUIPOS",
  "Escanear para leer",
  "Escanear para abrir",
  "Idioma",
  "Español",
  "Spanish",
  "Brillo",
  "Modo Nocturno",
  "Brillo de Modo Nocturno",
  "¡Actualización Disponible!",
  "Modo Sin Spoiler",
  "Nuevos Resultados Disponibles",
  "Mostrar Resultados",
  "Pantalla",
  "Hora",
  "Habilitar anulación de zona horaria",
  LV_SYMBOL_CHARGE " ¿Necesitas ayuda?",
  LV_SYMBOL_CHARGE " Te tenemos cubierto!",
  "Escanea para unirte a nuestro servidor de Discord y obtener soporte!",
  LV_SYMBOL_CHARGE " ¿Necesitas ayuda? Te tenemos cubierto! Escanea para unirte a nuestro servidor de Discord y obtener soporte!",
};

const LanguageStrings language_strings_fr PROGMEM = {
  {"Janvier", "Fevrier", "Mars", "Avril", "Mai", "Juin", "Juillet", "Aout", "Septembre", "Octobre", "Novembre", "Decembre"},
  {"Jan", "Fev", "Mar", "Avr", "Mai", "Juin", "Juil", "Aou", "Sep", "Oct", "Nov", "Dec"},
  {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"},
  {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"},
  "Chargement de l'acces WiFi...",
  "Connexion WiFi reussie !",
  "Echec de connexion au WiFi, connectez-vous au reseau \"Halo-F1\" depuis votre telephone pour definir les identifiants.\n\nSi le captive portal ne s'ouvre pas automatiquement, ouvrez votre navigateur et rendez-vous sur %s",
  "Ok",
  "Oui",
  "Non",
  "Fermer",
  "Annuler",
  "Succes",
  "Echec",
  "Recharger l'horloge",
  "L'horloge a ete mise a jour!",
  "EL1",
  "EL2",
  "EL3",
  "Qualifications",
  "Q1",
  "Q2",
  "Q3",
  "Course",
  "Course", // sprint race
  "Qualifications", // sprint qualifying
  "Essais libres",
  "Essais libres 1",
  "Essais libres 2",
  "Essais libres 3",
  "Prochain Grand Prix",
  "Prochaine session",
  "Week-end Sprint",
  "PILOTES",
  "EQUIPES",
  "Scanner pour lire",
  "Scanner pour ouvrir",
  "Language",
  "Français",
  "French",
  "Luminosite",
  "Mode Nuit",
  "Luminosite Mode Nuit",
  "Mise a jour disponible!",
  "Mode Sans Spoiler",
  "Nouveaux Resultats Disponibles",
  "Afficher les Resultats",
  "Affichage",
  "Heure",
  "Activer le remplacement du fuseau horaire",
  LV_SYMBOL_CHARGE " Besoin d'aide? Cliquez ici!",
  LV_SYMBOL_CHARGE " On s'occupe de vous!",
  "Scannez pour rejoindre notre serveur Discord et obtenir de l'aide!",
  LV_SYMBOL_CHARGE " Besoin d'aide? On s'occupe de vous! Scannez pour rejoindre notre serveur Discord et obtenir de l'aide!",
};

const LanguageStrings language_strings_nl PROGMEM = {
  {"Januari", "Februari", "Maart", "April", "Mei", "Juni", "Juli", "Augustus", "September", "Oktober", "November", "December"},
  {"Jan", "Feb", "Mrt", "Apr", "Mei", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dec"},
  {"Zondag", "Maandag", "Dinsdag", "Woensdag", "Donderdag", "Vrijdag", "Zaterdag"},
  {"Zo", "Ma", "Di", "Wo", "Do", "Vr", "Za"},
  "WiFi-toegang laden...",
  "Succesvol verbonden met WiFi!",
  "Verbinding met WiFi mislukt, verbind met het netwerk \"Halo-F1\" via je telefoon om de inloggegevens in te stellen.\n\nAls de captive portal niet automatisch opent, open dan je browser en ga naar %s",
  "Ok",
  "Ja",
  "Nee",
  "Sluiten",
  "Annuleren",
  "Succes",
  "Fout",
  "Klok herladen",
  "Klok is bijgewerkt!",
  "VT1",
  "VT2",
  "VT3",
  "Kwalificatie",
  "Q1",
  "Q2",
  "Q3",
  "Race",
  "Race", // sprint race
  "Kwalificatie", // sprint qualifying
  "Vrije training",
  "Vrije training 1",
  "Vrije training 2",
  "Vrije training 3",
  "Komende Grand Prix",
  "Komende sessie",
  "Sprintweekend",
  "COUREURS",
  "TEAMS",
  "Scan om te lezen",
  "Scan om te openen",
  "Language",
  "Nederlands",
  "Dutch",
  "Helderheid",
  "Nachtmodus",
  "Helderheid Nachtmodus",
  "Update Beschikbaar!",
  "Geen Spoilermodus",
  "Nieuwe Resultaten Beschikbaar",
  "Toon Resultaten",
  "Scherm",
  "Tijd",
  "Tijdzone Override Inschakelen",
  LV_SYMBOL_CHARGE " Heb je hulp nodig? Klik hier!",
  LV_SYMBOL_CHARGE " We hebben je gedekt!",
  "Scan om lid te worden van onze Discord-server en ondersteuning te krijgen!",
  LV_SYMBOL_CHARGE " Heb je hulp nodig? We hebben je gedekt! Scan om lid te worden van onze Discord-server en ondersteuning te krijgen!",
};

const LanguageStrings language_strings_de PROGMEM = {
  {"Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"},
  {"Jan", "Feb", "Mär", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"},
  {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"},
  {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"},
  "WLAN-Zugang wird geladen...",
  "Erfolgreich mit WLAN verbunden!",
  "Fehler bei der WLAN-Verbindung, verbinde dich mit dem Netzwerk \"Halo-F1\" von deinem Handy aus, um die Zugangsdaten einzugeben.\n\nWenn der captive portal nicht automatisch öffnet, öffne deinen Browser und navigiere zu %s",
  "Ok",
  "Ja",
  "Nein",
  "Schließen",
  "Abbrechen",
  "Erfolg",
  "Fehler",
  "Uhr neu laden",
  "Uhr wurde aktualisiert!",
  "FT1",
  "FT2",
  "FT3",
  "Qualifying",
  "Q1",
  "Q2",
  "Q3",
  "Rennen",
  "Rennen", // sprint race
  "Qualifying", // sprint qualifying
  "Freies Training",
  "Freies Training 1",
  "Freies Training 2",
  "Freies Training 3",
  "Nächster Grand Prix",
  "Nächste Sitzung",
  "Sprint-Wochenende",
  "FAHRER-WERTUNG",
  "TEAM-WERTUNG",
  "Scannen zum Lesen",
  "Scannen zum Öffnen",
  "Language",
  "Deutsch",
  "German",
  "Helligkeit",
  "Nachtmodus",
  "Helligkeit Nachtmodus",
  "Update Verfügbar!",
  "Kein Spoilermodus",
  "Neue Ergebnisse Verfügbar",
  "Ergebnisse Anzeigen",
  "Anzeige",
  "Zeit",
  "Zeitzonen-Override Aktivieren",
  LV_SYMBOL_CHARGE "Brauchst du Hilfe? Klicke hier!",
  LV_SYMBOL_CHARGE "Wir sind für dich da!",
  "Scanne den Code, um unserem Discord-Server beizutreten und Unterstützung zu erhalten.",
  LV_SYMBOL_CHARGE "Brauchst du Hilfe? Klicke hier! Wir sind für dich da! Scanne den Code, um unserem Discord-Server beizutreten und Unterstützung zu erhalten.",
};

const LanguageStrings language_strings_pt PROGMEM = {
  {"Janeiro", "Fevereiro", "Março", "Abril", "Maio", "Junho", "Julho", "Agosto", "Setembro", "Outubro", "Novembro", "Dezembro"},
  {"Jan", "Fev", "Mar", "Abr", "Mai", "Jun", "Jul", "Ago", "Set", "Out", "Nov", "Dez"},
  {"Domingo", "Segunda-feira", "Terça-feira", "Quarta-feira", "Quinta-feira", "Sexta-feira", "Sábado"},
  {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sáb"},
  "Carregando acesso WiFi...",
  "Conectado ao WiFi com sucesso!",
  "Falha ao conectar ao WiFi, conecte-se à rede \"Halo-F1\" pelo seu telefone para definir as credenciais.\n\nSe o captive portal não abrir automaticamente, abra seu navegador e vá para %s",
  "Ok",
  "Sim",
  "Não",
  "Fechar",
  "Cancelar",
  "Sucesso",
  "Falha",
  "Recarregar relógio",
  "O relógio foi atualizado!",
  "TL1",
  "TL2",
  "TL3",
  "Classificação",
  "Q1",
  "Q2",
  "Q3",
  "Corrida",
  "Corrida", // sprint race
  "Classificação", // sprint qualifying
  "Treino Livre",
  "Treino Livre 1",
  "Treino Livre 2",
  "Treino Livre 3",
  "Próximo Grande Prêmio",
  "Próxima Sessão",
  "Fim de Semana Sprint",
  "PILOTOS",
  "EQUIPES",
  "Escaneie para Ler",
  "Escaneie para Abrir",
  "Idioma",
  "Português",
  "Português",
  "Brilho",
  "Modo Noturno",
  "Brilho do Modo Noturno",
  "Atualização Disponível!",
  "Modo Sem Spoiler",
  "Novos Resultados Disponíveis",
  "Mostrar Resultados",
  "Tela",
  "Hora",
  "Ativar Override de Fuso Horário",
  LV_SYMBOL_CHARGE " Precisa de ajuda?",
  LV_SYMBOL_CHARGE " Nós te cobrimos!",
  "Escaneie para entrar no nosso servidor Discord e obter suporte!",
  LV_SYMBOL_CHARGE " Precisa de ajuda? Nós te cobrimos! Escaneie para entrar no nosso servidor Discord e obter suporte!",
};

const LanguageStrings language_strings_no PROGMEM = {
  {"Januar", "Februar", "Mars", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Desember"},
  {"Jan", "Feb", "Mar", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Des"},
  {"Søndag", "Mandag", "Tirsdag", "Onsdag", "Torsdag", "Fredag", "Lørdag"},
  {"Søn", "Man", "Tir", "Ons", "Tor", "Fre", "Lør"},
  "Laster inn WiFi-tilgang...",
  "Tilkoblet WiFi!",
  "Kunne ikke koble til WiFi, koble til nettverket \"Halo-F1\" fra din telefon for å skrive inn påloggingsinformasjonen.\n\nHvis den captive portal ikke åpnes automatisk, åpne din nettleser og gå til %s",
  "Ok",
  "Ja",
  "Nei",
  "Lukk",
  "Avbryt",
  "Vellykket",
  "Feil",
  "Last inn klokken på nytt",
  "Klokken har blitt oppdatert!",
  "FP1",
  "FP2",
  "FP3",
  "Kvalifisering",
  "Q1",
  "Q2",
  "Q3",
  "Løp",
  "Løp", // sprint race
  "Kvalifisering", // sprint qualifying
  "Trening",
  "Trening 1",
  "Trening 2",
  "Trening 3",
  "Kommende Grand Prix",
  "Kommende økt",
  "Sprinthelg",
  "FØRERPOENG",
  "LAGPOENG",
  "Skann for å lese",
  "Skann for å åpne",
  "Språk",
  "Engelsk",
  "Engelsk",
  "Lysstyrke",
  "Kveldsmodus",
  "Lysstyrke For Kveldsmodus",
  "Oppdatering Tilgjengelig!",
  "Spoilerfri Modus",
  "Nye Resultater Tilgjengelig",
  "Vis Resultater",
  "Skjerm",
  "Tid",
  "Aktiver Tidszone Override",
  LV_SYMBOL_CHARGE " Trenger du hjelp? Klikk her!",
  LV_SYMBOL_CHARGE " Vi har deg dekket!",
  "Skann for å bli med i vår Discord-server og få støtte!",
  LV_SYMBOL_CHARGE " Trenger du hjelp? Vi har deg dekket! Skann for å bli med i vår Discord-server og få støtte!",
};

const LanguageStrings language_strings_pl PROGMEM = {
  {"Stycznia", "Lutego", "Marca", "Kwietnia", "Maja", "Czerwca", "Lipca", "Sierpnia", "Wrzesnia", "Pazdziernika", "Listopada", "Grudnia"},
  {"Sty", "Lut", "Mar", "Kwi", "Maj", "Cze", "Sie", "Wrz", "Paź", "Lis", "Gru"},
  {"Niedziela", "Poniedzialek", "Wtorek", "Sroda", "Czwartek", "Piatek", "Sobota"},
  {"Niedz.", "Pon.", "Wt.", "Sr.", "Czw.", "Pt.", "Sb."},
  "Laczenie z WiFi...",
  "Polaczono z WiFi!",
  "Nie udalo sie polaczyc z WiFi, podlacz sie do sieci \"Halo-F1\" ze swojego telefonu i wprowadz nazwe sieci oraz haslo.\n\nJesli captive portal nie otworzy sie automatycznie, otwórz przegladarke i przejdz do %s",
  "OK",
  "Tak",
  "Nie",
  "Zamknij",
  "Anuluj",
  "Powodzenie",
  "Niepowodzenie",
  "Zaktualizuj zegar",
  "Zegar zostal zaktualizowany!",
  "FP1",
  "FP2",
  "FP3",
  "Kwalifikacje",
  "Q1",
  "Q2",
  "Q3",
  "Wyscig",
  "Sprint", // sprint race
  "Kwalifikacje", // sprint qualifying
  "Sesja treningowa",
  "Sesja treningowa 1",
  "Sesja treningowa 2",
  "Sesja treningowa 3",
  "Nadchodzace Grand Prix",
  "Nadchodzaca Sesja",
  "Weekend Sprinterski",
  "KIEROWCOW",
  "ZESPOLOW",
  "Skanuj Aby Przeczytac",
  "Skanuj Aby Otworzyc",
  "Jezyk",
  "Polski",
  "Polish",
  "Jasnosc Ekranu",
  "Tryb Nocny",
  "Jasnosc Trybu Nocnego",
  "Dostepna Aktualizacja!",
  "Tryb No Spoiler",
  "Dostepne Nowe Wyniki",
  "Pokaz",
  "Wyswietlacz",
  "Czas",
  "Aktywuj Nadpisanie Strefy Czasowej",
  LV_SYMBOL_CHARGE " Potrzebujesz pomocy?",
  LV_SYMBOL_CHARGE " Mamy Cie!",
  "Skanuj, aby dolaczyc do naszego serwera Discord i uzyskac wsparcie!",
  LV_SYMBOL_CHARGE " Potrzebujesz pomocy? Mamy Cie! Skanuj, aby dolaczyc do naszego serwera Discord i uzyskac wsparcie!",
};

uint32_t get_team_color(String team) {
  // maybe use a switch case
  if (team == "mercedes") return 0x00D7B6;
  if (team == "red_bull") return 0x0000FF;
  if (team == "ferrari") return 0xFF1100;
  if (team == "mclaren") return 0xF47600;
  if (team == "alpine") return 0x00A1E8;
  if (team == "rb") return 0x6C98FF;
  if (team == "aston_martin") return 0x229971;
  if (team == "williams") return 0x1868DB;
  if (team == "sauber") return 0x01C00E;
  if (team == "haas") return 0xFFFFFF; //0x9C9FA2;
  if (team == "audi") return 0xFF3300;

  return 0x333333;
}

struct LanguageEntry {
    const char* displayName; // Human-readable name for dropdown
    const LanguageStrings* strings; // Pointer to PROGMEM struct
};

// Language list
const LanguageEntry languages[] = {
    {"English", &language_strings_en},
    {"Italiano", &language_strings_it},
    {"Español",   &language_strings_es},
    {"Français",  &language_strings_fr},
    {"Nederlands",&language_strings_nl},
    {"Deutsch",   &language_strings_de},
    {"Português", &language_strings_pt},
    {"Norsk", &language_strings_no},
    {"Polski", &language_strings_pl},
};

const size_t languageCount = sizeof(languages) / sizeof(languages[0]);
const LanguageStrings *localized_text = &language_strings_en;
