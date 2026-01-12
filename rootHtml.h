const char *rootHtml = R"rawliteral(
<!DOCTYPE html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Stream Control Hub</title>
    <style>
        body { font-family: 'Segoe UI', sans-serif; background: #121212; color: white; display: flex; flex-direction: column; justify-content: center; align-items: center; height: 100vh; margin: 0; }
        .menu-container { display: grid; grid-template-columns: repeat(2, 1fr); gap: 20px; max-width: 500px; width: 100%; padding: 20px; }
        
        .menu-item { background: #1e1e1e; border: 2px solid #333; padding: 30px; border-radius: 12px; text-align: center; cursor: pointer; text-decoration: none; color: white; transition: all 0.3s ease; display: flex; flex-direction: column; align-items: center; gap: 10px; }
        .menu-item:hover { border-color: #00d1b2; background: #252525; transform: translateY(-5px); box-shadow: 0 5px 15px rgba(0,0,0,0.3); }
        
        .icon { font-size: 2.5rem; }
        .title { font-weight: bold; text-transform: uppercase; letter-spacing: 1px; font-size: 0.9rem; }
        .lang-status { color: #00d1b2; font-size: 0.8rem; font-weight: bold; margin-top: 5px; }
        h1 { margin-bottom: 30px; font-weight: 300; color: #888; letter-spacing: 2px; }
    </style>
</head>
<body>

    <h1 id="main-title">STREAM CONTROL PANEL</h1>

    <div class="menu-container">
        <a href="configuratie.html" id="link-config" class="menu-item">
            <span class="icon">⚙️</span>
            <span id="txt-config" class="title">Configuratie</span>
        </a>

        <a href="test.html" id="link-test" class="menu-item">
            <span class="icon">🧪</span>
            <span id="txt-test" class="title">Test</span>
        </a>

        <a href="monitor.html" id="link-monitor" class="menu-item">
            <span class="icon">📊</span>
            <span id="txt-monitor" class="title">Monitor</span>
        </a>

        <a href="reset-wifi.html" id="link-wifi" class="menu-item">
            <span class="icon">📶</span>
            <span id="txt-wifi" class="title">Reset WiFi</span>
        </a>

        <div class="menu-item" onclick="toggleLanguage()">
            <span class="icon">🌍</span>
            <span id="txt-lang" class="title">Taal</span>
            <span id="active-lang" class="lang-status">NEDERLANDS</span>
        </div>
    </div>

    <script>
     const urlParams = new URLSearchParams(window.location.search);
    
    // Gebruik de waarden uit de URL, of een standaardwaarde (fallback) als ze ontbreken
      const ssid = urlParams.get('ssid') || "OBDSniffer"
      const password = urlParams.get('password')
    
        const translations = {
            nl: { 
                title: "CONTROLE PANEEL", 
                config: "Configuratie", 
                test: "Test Pagina", 
                monitor: "Monitor", 
                wifi: "Reset WiFi", // NL vertaling
                lang: "Taal", 
                current: "NEDERLANDS" 
            },
            en: { 
                title: "CONTROL PANEL", 
                config: "Configuration", 
                test: "Test Page", 
                monitor: "Monitor", 
                wifi: "Reset WiFi", // EN vertaling
                lang: "Language", 
                current: "ENGLISH" 
            }
        };

        let currentLang = localStorage.getItem('preferredLang') || 'nl';

        function updateLinksAndText() {
            const t = translations[currentLang];
            
            document.getElementById('main-title').innerText = t.title;
            document.getElementById('txt-config').innerText = t.config;
            document.getElementById('txt-test').innerText = t.test;
            document.getElementById('txt-monitor').innerText = t.monitor;
            document.getElementById('txt-wifi').innerText = t.wifi; // Tekst bijwerken
            document.getElementById('txt-lang').innerText = t.lang;
            document.getElementById('active-lang').innerText = t.current;

            // Links bijwerken met taalparameter
            document.getElementById('link-config').href = `configuratie.html?lang=${currentLang}`;
            document.getElementById('link-test').href = `test.html?lang=${currentLang}`;
            document.getElementById('link-monitor').href = `monitor.html?lang=${currentLang}&label=${currentLang === 'nl' ? 'Druk' : 'Pressure'}&min=0&max=100`;
            document.getElementById('link-wifi').href = `reset-wifi.html?lang=${currentLang}&ssid=${ssid}&password=${password}`;
        }

        function toggleLanguage() {
            currentLang = (currentLang === 'nl') ? 'en' : 'nl';
            localStorage.setItem('preferredLang', currentLang);
            updateLinksAndText();
        }

        window.onload = updateLinksAndText;
    </script>
</body>
</html>
)rawliteral";