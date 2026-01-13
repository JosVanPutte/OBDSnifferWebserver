const char *monitorBarHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Live Bar Graph - Server</title>
    <style>
        body { font-family: 'Segoe UI', sans-serif; background: #121212; color: white; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
        .container { text-align: center; background: #1e1e1e; padding: 40px; border-radius: 15px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); width: 250px; }
        
        .bar-container { width: 80px; height: 300px; background: #333; margin: 20px auto; border-radius: 8px; position: relative; overflow: hidden; display: flex; align-items: flex-end; }
        
        /* De feitelijke staaf met vloeiende overgang */
        .bar-fill { 
            width: 100%; 
            height: 0%; 
            background: linear-gradient(to top, #00d1b2, #00ffcc); 
            transition: height 0.6s cubic-bezier(0.4, 0, 0.2, 1), background 0.3s ease; 
            border-radius: 4px; 
        }
        
        .label { font-size: 1.2rem; color: #888; text-transform: uppercase; letter-spacing: 2px; margin-bottom: 5px; }
        .value { font-size: 2.5rem; font-weight: bold; font-family: monospace; color: #00d1b2; }
        .status-text { font-size: 0.7rem; color: #444; margin-top: 10px; display: block; }
    </style>
</head>
<body>

<div class="container">
    <div id="lbl-display" class="label">Laden...</div>
    <div id="val-display" class="value">0</div>
    
    <div class="bar-container">
        <div id="bar-fill" class="bar-fill"></div>
    </div>
    
    <span id="status-display" class="status-text">Zoeken naar server...</span>
</div>

<script>
    // 1. Instellingen uit de URL
    const urlParams = new URLSearchParams(window.location.search);
    const label = urlParams.get('label') || "Sensor";
    const maxVal = parseFloat(urlParams.get('max')) || 100;
    const min = parseFloat(urlParams.get('min')) || 0; // Gebruik 'min' uit URL, anders 0
    const intervalTime = parseInt(urlParams.get('ms')) || 1000;

    document.getElementById('lbl-display').innerText = label;

    const barFill = document.getElementById('bar-fill');
    const valDisplay = document.getElementById('val-display');
    const statusDisplay = document.getElementById('status-display');

    // 2. Functie om data op te halen
    async function fetchBarData() {
        try {
            // Pas de URL '/get-value' aan naar de endpoint van je server
            const response = await fetch('/get-value');
            if (!response.ok) throw new Error();

            const data = await response.json();
            // Ondersteunt zowel getal als JSON {"waarde": X}
            const currentValue = (typeof data === 'object') ? data.waarde : parseFloat(data);

            // Tekst updaten
            valDisplay.innerText = Math.round(currentValue);

            // Percentage berekenen voor de hoogte
            const percentage = (currentValue / (maxVal - min)) * 100;
            barFill.style.height = percentage + "%";

            // Kleur-logica (rood bij > 80%)
            if (percentage > 80) {
                barFill.style.background = "linear-gradient(to top, #ff3860, #ff5252)";
                valDisplay.style.color = "#ff3860";
            } else {
                barFill.style.background = "linear-gradient(to top, #00d1b2, #00ffcc)";
                valDisplay.style.color = "#00d1b2";
            }

            statusDisplay.innerText = "Verbonden";
            statusDisplay.style.color = "#444";

        } catch (error) {
            console.error("Data error:", error);
            statusDisplay.innerText = "Server offline";
            statusDisplay.style.color = "#ff3860";
        }
    }

    // 3. Start de loop
    setInterval(fetchBarData, intervalTime);
    fetchBarData(); // Direct de eerste keer laden
</script>

</body>
</html>
)rawliteral";