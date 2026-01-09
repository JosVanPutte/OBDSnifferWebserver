const char *testGraphHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
    <meta charset="UTF-8">
    <title>SVG Zaagtand Grafiek</title>
    <style>
        body { font-family: sans-serif; background: #121212; color: white; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0; }
        .container { background: #1e1e1e; padding: 20px; border-radius: 10px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); width: 80%; max-width: 800px; }
        .header { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 10px; }
        .label { font-size: 1.5rem; color: #00d1b2; font-weight: bold; }
        .current-value { font-family: monospace; font-size: 2rem; }
        
        svg { background: #000; border: 1px solid #333; width: 100%; height: 300px; display: block; }
        polyline { fill: none; stroke: #00d1b2; stroke-width: 2; vector-effect: non-scaling-stroke; }
        
        .grid-line { stroke: #222; stroke-width: 1; }
    </style>
</head>
<body>

<div class="container">
    <div class="header">
        <div id="lbl-title" class="label">Laden...</div>
        <div id="val-display" class="current-value">0.0</div>
    </div>

    <svg id="graph-svg" viewBox="0 0 500 100" preserveAspectRatio="none">
        <line class="grid-line" x1="0" y1="25" x2="500" y2="25" />
        <line class="grid-line" x1="0" y1="50" x2="500" y2="50" />
        <line class="grid-line" x1="0" y1="75" x2="500" y2="75" />
        
        <polyline id="line" points=""></polyline>
    </svg>
</div>

<script>
    // 1. URL Parameters uitlezen
    const urlParams = new URLSearchParams(window.location.search);
    const label = urlParams.get('label') || "Signaal";
    const minVal = parseFloat(urlParams.get('min')) || 0;
    const maxVal = parseFloat(urlParams.get('max')) || 100;

    document.getElementById('lbl-title').innerText = label;

    // 2. Data variabelen
    const maxPoints = 100; // Hoeveelheid punten in beeld
    let points = [];
    let currentValue = minVal;
    const step = (maxVal - minVal) / 20; // Snelheid van de stijging

    const polyline = document.getElementById('line');
    const valDisplay = document.getElementById('val-display');

    function updateGraph() {
        // --- DE ZAAGTANK GENERATOR ---
        currentValue += step;
        
        // Zodra we de max bereiken, springen we direct terug naar min (Zaagtand)
        if (currentValue > maxVal) {
            currentValue = minVal;
        }

        // Toevoegen aan de data array
        points.push(currentValue);
        if (points.length > maxPoints) {
            points.shift(); // Verwijder oudste punt (schuifeffect)
        }

        // --- SVG TEKENEN ---
        // We mappen de waarde naar de 100 units hoogte van de SVG
        // Let op: in SVG is y=0 de bovenkant, dus we trekken het af van 100
        const mappedPoints = points.map((v, i) => {
            const x = (i / (maxPoints - 1)) * 500;
            const y = 100 - ((v - minVal) / (maxVal - minVal) * 100);
            return `${x},${y}`;
        }).join(" ");

        polyline.setAttribute("points", mappedPoints);
        valDisplay.innerText = currentValue.toFixed(1);
    }

    // Elke 100ms een update (10 Hz)
    setInterval(updateGraph, 100);
</script>

</body>
</html>
)rawliteral";