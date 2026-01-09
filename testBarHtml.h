const char *testBarHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Live Bar Graph</title>
    <style>
        body { font-family: 'Segoe UI', sans-serif; background: #121212; color: white; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
        .container { text-align: center; background: #1e1e1e; padding: 40px; border-radius: 15px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); width: 250px; }
        
        .bar-container { width: 80px; height: 300px; background: #333; margin: 20px auto; border-radius: 8px; position: relative; overflow: hidden; display: flex; align-items: flex-end; }
        
        /* De feitelijke staaf */
        .bar-fill { width: 100%; height: 0%; background: linear-gradient(to top, #00d1b2, #00ffcc); transition: height 0.1s linear; border-radius: 4px; }
        
        .label { font-size: 1.2rem; color: #888; text-transform: uppercase; letter-spacing: 2px; margin-bottom: 5px; }
        .value { font-size: 2.5rem; font-weight: bold; font-family: monospace; color: #00d1b2; }
        .max-label { font-size: 0.8rem; color: #555; margin-top: 10px; }
    </style>
</head>
<body>

<div class="container">
    <div id="lbl-display" class="label">Laden...</div>
    <div id="val-display" class="value">0</div>
    
    <div class="bar-container">
        <div id="bar-fill" class="bar-fill"></div>
    </div>
    
    <div id="max-display" class="max-label">Max: 0</div>
</div>

<script>
    // 1. Parameters uit de URL halen: ?label=Temperatuur&max=100
    const urlParams = new URLSearchParams(window.location.search);
    const label = urlParams.get('label') || "Waarde";
    const maxVal = parseFloat(urlParams.get('max')) || 100;

    document.getElementById('lbl-display').innerText = label;
    document.getElementById('max-display').innerText = `Max: ${maxVal}`;

    let currentValue = 0;
    let direction = 1;
    const step = maxVal / 50; // In 50 stappen van 0 naar max

    const barFill = document.getElementById('bar-fill');
    const valDisplay = document.getElementById('val-display');

    // 2. De Update Loop (elke 100ms)
    function updateGraph() {
        // Waarde aanpassen
        currentValue += (step * direction);
        
        // Grenzen controleren
        if (currentValue >= maxVal) {
            currentValue = maxVal;
            direction = -1;
        } else if (currentValue <= 0) {
            currentValue = 0;
            direction = 1;
        }

        // Tekst updaten
        valDisplay.innerText = Math.round(currentValue);

        // Staaf hoogte updaten (in procenten)
        const percentage = (currentValue / maxVal) * 100;
        barFill.style.height = percentage + "%";

        // Optioneel: kleur veranderen als de waarde hoog is
        if (percentage > 80) {
            barFill.style.background = "linear-gradient(to top, #ff3860, #ff5252)";
        } else {
            barFill.style.background = "linear-gradient(to top, #00d1b2, #00ffcc)";
        }
    }

    // Start de timer
    setInterval(updateGraph, 100);
</script>

</body>
</html>
)rawliteral";