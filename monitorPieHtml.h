const char *monitorPieHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
    <meta charset="UTF-8">
    <title>Gauge Monitor - Live</title>
    <style>
        body { font-family: 'Segoe UI', sans-serif; background: #1a1a1a; color: white; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
        .gauge-container { position: relative; width: 300px; text-align: center; }
        
        /* De SVG Meter */
        .gauge-svg { transform: rotate(0deg); }
        .gauge-bg { fill: none; stroke: #333; stroke-width: 20; }
        .gauge-progress { 
            fill: none; 
            stroke: #00d1b2; 
            stroke-width: 20; 
            stroke-linecap: round;
            /* Transition zorgt voor een vloeiende beweging tussen server-updates */
            transition: stroke-dashoffset 0.5s ease-in-out; 
        }

        .info { position: absolute; top: 55%; left: 50%; transform: translate(-50%, -50%); }
        .value { font-size: 3rem; font-weight: bold; display: block; }
        .label { font-size: 1.2rem; color: #888; text-transform: uppercase; }
        .status-dot { font-size: 0.6rem; color: #444; margin-top: 5px; }
    </style>
</head>
<body>

<div class="gauge-container">
    <svg class="gauge-svg" width="250" height="250" viewBox="0 0 200 200">
        <path class="gauge-bg" d="M 40 160 A 85 85 0 1 1 160 160" />
        <path id="progress-path" class="gauge-progress" d="M 40 160 A 85 85 0 1 1 160 160" />
    </svg>

    <div class="info">
        <span id="val-display" class="value">0</span>
        <span id="lbl-display" class="label">Laden...</span>
        <div id="status" class="status-dot">Verbinden...</div>
    </div>
</div>

<script>
    // 1. Instellingen uit URL
    const urlParams = new URLSearchParams(window.location.search);
    const label = urlParams.get('label') || "Data";
    const max = parseFloat(urlParams.get('max')) || 100;
    const intervalTime = parseInt(urlParams.get('ms')) || 1000; // Standaard elke seconde
    const min = parseFloat(urlParams.get('min')) || 0; // Gebruik 'min' uit URL, anders 0

    document.getElementById('lbl-display').innerText = label;
    
    const progressPath = document.getElementById('progress-path');
    const valDisplay = document.getElementById('val-display');
    const statusDisplay = document.getElementById('status');
    const pathLength = progressPath.getTotalLength();

    // Initialiseer de meter op 0
    progressPath.style.strokeDasharray = pathLength;
    progressPath.style.strokeDashoffset = pathLength;

    // 2. De functie die de echte waarde ophaalt
    async function updateGaugeFromServer() {
        try {
            // Vervang '/get-value' door de URL van jouw server data
            const response = await fetch('/get-value');
            if (!response.ok) throw new Error();

            const data = await response.json();
            // Werkt als de server {"waarde": 50} stuurt of gewoon het getal 50
            const currentValue = (typeof data === 'object') ? data.waarde : parseFloat(data);

            // Tekst updaten
            valDisplay.innerText = Math.round(currentValue);

            // Meter visueel updaten
            const percentage = Math.min(Math.max((currentValue - min) / (max - min), 0), 1); // Clamp tussen 0 en 1
            const offset = pathLength - (percentage * pathLength);
            progressPath.style.strokeDashoffset = offset;

            // Status indicator (klein groen stipje effect)
            statusDisplay.innerText = "● LIVE";
            statusDisplay.style.color = "#00d1b2";

        } catch (error) {
            console.error("Server fout:", error);
            statusDisplay.innerText = "● OFFLINE";
            statusDisplay.style.color = "#ff4444";
        }
    }

    // 3. Start de loop
    setInterval(updateGaugeFromServer, intervalTime);
    updateGaugeFromServer(); // Direct de eerste keer uitvoeren
</script>

</body>
</html>
)rawliteral";