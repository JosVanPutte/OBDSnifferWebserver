const char *testPieHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
    <meta charset="UTF-8">
    <title>Gauge Monitor</title>
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
            stroke-dasharray: 565; /* Omtrek van de cirkel (2 * PI * r) */
            stroke-dashoffset: 565; 
            transition: stroke-dashoffset 0.1s linear;
        }

        .info { position: absolute; top: 55%; left: 50%; transform: translate(-50%, -50%); }
        .value { font-size: 3rem; font-weight: bold; display: block; }
        .label { font-size: 1.2rem; color: #888; text-transform: uppercase; }
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
    </div>
</div>

<script>
    // 1. Parameters uit URL halen
    const urlParams = new URLSearchParams(window.location.search);
    const label = urlParams.get('label') || "Data";
    const max = parseFloat(urlParams.get('max')) || 100;
    const min = parseFloat(urlParams.get('min')) || 0; // Gebruik 'min' uit URL, anders 0

    document.getElementById('lbl-display').innerText = label;

    // 2. Simulatie variabelen
    let currentValue = min;
    let direction = 1;
    const step = (max - min) / 50; // Snelheid van de variatie
    
    const progressPath = document.getElementById('progress-path');
    const valDisplay = document.getElementById('val-display');
    const pathLength = progressPath.getTotalLength();

    // 3. De Update Loop (elke 100ms)
    function updateGauge() {
        // Waarde aanpassen
        currentValue += (step * direction);
        if (currentValue >= max || currentValue <= min) direction *= -1;

        // Tekst updaten
        valDisplay.innerText = Math.round(currentValue);

        // Meter visueel updaten
        // De dashoffset bepaalt hoeveel van de lijn 'leeg' blijft
        const percentage = (currentValue - min) / (max - min);
        const offset = pathLength - (percentage * pathLength);
        progressPath.style.strokeDashoffset = offset;
    }

    // Start de simulatie
    progressPath.style.strokeDasharray = pathLength;
    setInterval(updateGauge, 100);
</script>

</body>
</html>
)rawliteral";