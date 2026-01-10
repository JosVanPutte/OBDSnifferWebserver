const char *monitorNumberHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
    <meta charset="UTF-8">
    <title>Live Server Data Stream</title>
    <style>
        body { font-family: sans-serif; background: #121212; color: white; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
        .card { background: #1e1e1e; padding: 40px; border-radius: 12px; text-align: center; border: 1px solid #00d1b2; min-width: 300px; }
        .label { font-size: 1.2rem; color: #aaa; margin-bottom: 10px; }
        .value { font-size: 4rem; font-family: monospace; color: #00d1b2; }
        .status { font-size: 0.7rem; margin-top: 15px; color: #555; }
    </style>
</head>
<body>

<div class="card">
    <div id="display-label" class="label">Laden...</div>
    <div id="display-value" class="value">0.00</div>
    <div id="connection-status" class="status">Verbinden met server...</div>
</div>

<script>
    const urlParams = new URLSearchParams(window.location.search);
    
    const config = {
        label: urlParams.get('label') || "Live Waarde",
        interval: parseInt(urlParams.get('ms')) || 1000 // Hoe vaak verversen (standaard 1 sec)
    };

    document.getElementById('display-label').innerText = config.label;

    // --- DE NIEUWE FUNCTIE: Gegevens ophalen van server ---
    async function fetchValueFromServer() {
        try {
            // Pas '/get-value' aan naar het juiste pad op je webserver
            const response = await fetch('/get-value');
            
            if (!response.ok) throw new Error('Netwerk response was niet ok');

            // We gaan er vanuit dat de server een getal of JSON stuurt: {"waarde": 12.34}
            const data = await response.json();
            const newValue = (typeof data === 'object') ? data.waarde : data;

            document.getElementById('display-value').innerText = parseFloat(newValue).toFixed(2);
            document.getElementById('connection-status').innerText = "Laatste update: " + new Date().toLocaleTimeString();
            document.getElementById('connection-status').style.color = "#00d1b2";

        } catch (error) {
            console.error("Fout bij ophalen:", error);
            document.getElementById('connection-status').innerText = "Server offline - bezig met opnieuw proberen...";
            document.getElementById('connection-status').style.color = "red";
        }
    }

    // Start de herhaling op basis van de interval instelling
    setInterval(fetchValueFromServer, config.interval);

    // Voer direct de eerste keer uit bij laden
    fetchValueFromServer();
</script>
</body>
</html>
)rawliteral";