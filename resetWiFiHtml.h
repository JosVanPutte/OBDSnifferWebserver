const char *resetWiFiHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi Configuratiescherm</title>
    <style>
        body { font-family: sans-serif; background-color: #f4f4f9; display: flex; justify-content: center; padding: 20px; }
        .card { background: white; padding: 2rem; border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); width: 100%; max-width: 400px; }
        h2 { color: #333; margin-top: 0; }
        .field { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input[type="text"], input[type="password"], select { width: 100%; padding: 10px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
        button { width: 100%; padding: 12px; background-color: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; margin-top: 10px; }
        button:hover { background-color: #0056b3; }
        .lang-switch { font-size: 0.8rem; text-align: right; margin-bottom: 10px; }
        .lang-switch a { color: #007bff; text-decoration: none; }
    </style>
</head>
<body>

<div class="card">
    <h2 id="title">WiFi Instellingen</h2>
    
    <form id="wifiForm" onsubmit="return false;">
        <div class="field">
            <label id="lbl-ssid" for="ssid">Netwerknaam (SSID)</label>
            <input type="text" id="ssid" name="ssid" placeholder="Vul SSID in">
        </div>
        
        <div class="field">
            <label id="lbl-pw" for="password">Wachtwoord</label>
            <input type="password" id="password" name="password">
        </div>

        <div class="field">
            <label id="lbl-ap" for="ap_mode">Gebruik als Access Point (AP)</label>
            <select id="ap_mode" name="ap_mode">
                <option value="0" id="opt-no">Nee</option>
                <option value="1" id="opt-yes">Ja</option>
            </select>
            <label id="lbl-swap" for="swap_mode">Swap CAN H en L</label>
            <select id="swap_mode" name="swap_mode">
                <option value="0" id="opt-no1">Nee</option>
                <option value="1" id="opt-yes1">Ja</option>
            </select>
        </div>

        <button type="button" id="btn-save" onClick="postToServer()">Instellingen Opslaan</button>
    </form>
</div>

<script>
    const translations = {
        nl: {
            title: "WiFi Instellingen",
            ssid: "Netwerknaam (SSID)",
            pw: "Wachtwoord",
            ap: "Gebruik als Access Point (AP)",
            save: "Instellingen Opslaan",
            placeholder: "Vul SSID in",
            yes: "Ja",
            no: "Nee"
        },
        en: {
            title: "WiFi Settings",
            ssid: "Network Name (SSID)",
            pw: "Password",
            ap: "Use as Access Point (AP)",
            save: "Save Settings",
            placeholder: "Enter SSID",
            yes: "Yes",
            no: "No"
        }
    };

    function updateLanguage() {
        const urlParams = new URLSearchParams(window.location.search);
        const ssid = urlParams.get('ssid') || "";
        const password = urlParams.get('password') || "";
        const ap_mode = urlParams.get('ap_mode') || 0;
        const swap_mode = urlParams.get('swap_mode') | 0;
        const lang = urlParams.get('lang') === 'en' ? 'en' : 'nl';
        const t = translations[lang];

        document.getElementById('ssid').value = ssid;
        document.getElementById('password').value = password;
        document.getElementById('ap_mode').value = ap_mode;
        document.getElementById('swap_mode').value = swap_mode;
        document.getElementById('title').innerText = t.title;
        document.getElementById('lbl-ssid').innerText = t.ssid;
        document.getElementById('lbl-pw').innerText = t.pw;
        document.getElementById('lbl-ap').innerText = t.ap;
        document.getElementById('btn-save').innerText = t.save;
        document.getElementById('ssid').placeholder = t.placeholder;
        
        // Vertaal opties in de selectbox
        document.getElementById('opt-no').innerText = t.no;
        document.getElementById('opt-yes').innerText = t.yes;
        document.getElementById('opt-no1').innerText = t.no;
        document.getElementById('opt-yes1').innerText = t.yes;
        
        document.documentElement.lang = lang;
    }

    async function postToServer() {
        const ssid = document.getElementById('ssid').value;
        const password = document.getElementById('password').value;
        const apMode = document.getElementById('ap_mode').value;
        const swapMode = document.getElementById('swap_mode').value;
        const lang = document.documentElement.lang;

        try {
            // Let op: Ik gebruik hier URLSearchParams voor een schonere URL opbouw
            const params = new URLSearchParams({
                ssid: ssid,
                password: password,
                ap_mode: apMode,
                swap_mode : swapMode
            });

            const response = await fetch('/wifi?' + params.toString(), {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid, password, ap_mode: apMode, swap_mode : swapMode }) 
            });

            if(response.ok) {
                alert(lang === 'nl' ? 'Instellingen succesvol verzonden!' : 'Settings sent successfully!');
            }
        } catch (error) {
            console.error('Fout bij verzenden:', error);
            alert(lang === 'nl' ? 'Fout bij het verbinden met de server.' : 'Error connecting to server.');
        }
    }

    window.onload = updateLanguage;
</script>

</body>
</html>
)rawliteral";