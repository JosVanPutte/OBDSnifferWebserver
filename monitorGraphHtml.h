const char *monitorGraphHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
    <meta charset="UTF-8">
    <title>Live Server Grafiek</title>
    <style>
        body { font-family: sans-serif; background: #121212; color: white; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0; }
        .container { background: #1e1e1e; padding: 20px; border-radius: 10px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); width: 80%; max-width: 800px; }
        .header { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 10px; }
        .label { font-size: 1.5rem; color: #00d1b2; font-weight: bold; }
        .current-value { font-family: monospace; font-size: 2rem; color: #00d1b2; }
        
        svg { background: #000; border: 1px solid #333; width: 100%; height: 300px; display: block; border-radius: 4px; }
        polyline { fill: none; stroke: #00d1b2; stroke-width: 2; vector-effect: non-scaling-stroke; transition: all 0.1s linear; }
        
        .grid-line { stroke: #222; stroke-width: 1; }
        .status { font-size: 0.7rem; color: #444; margin-top: 10px; text-align: right; width: 100%; }
    </style>
</head>
<body>

<div class="container">
    <div class="header">
        <div id="lbl-title" class="label">Laden...</div>
        <div id="val-display" class="current-value">--.-</div>
    </div>

    <svg id="graph-svg" viewBox="0 0 500 100" preserveAspectRatio="none">
        <line class="grid-line" x1="0" y1="25" x2="500" y2="25" />
        <line class="grid-line" x1="0" y1="50" x2="500" y2="50" />
        <line class="grid-line" x1="0" y1="75" x2="500" y2="75" />
        
        <polyline id="line" points=""></polyline>
    </svg>
    <div id="status" class="status">Verbinden met server...</div>
</div>

<script>
    // 1. URL Parameters & Configuratie
    const urlParams = new URLSearchParams(window.location.search);
    const label = urlParams.get('label') || "Live Signaal";
    const minVal = parseFloat(urlParams.get('min')) || 0;
    const maxVal = parseFloat(urlParams.get('max')) || 100;
    const intervalTime = parseInt(urlParams.get('ms')) || 200; // Hoe vaak verversen

    document.getElementById('lbl-title').innerText = label;

    const maxPoints = 100; // Breedte van de geschiedenis
    let points = Array(maxPoints).fill(minVal); // Start met een platte lijn

    const polyline = document.getElementById('line');
    const valDisplay = document.getElementById('val-display');
    const statusDisplay = document.getElementById('status');

    // 2. Data ophalen en Grafiek tekenen
    async function fetchData() {
        try {
            const response = await fetch('/get-value');
            if (!response.ok) throw new Error();

            const data = await response.json();
            const val = (typeof data === 'object') ? data.waarde : parseFloat(data);

            // Update tekst
            valDisplay.innerText = val.toFixed(1);

            // Voeg nieuwe waarde toe en schuif de rest op
            points.push(val);
            if (points.length > maxPoints) {
                points.shift();
            }

            // Teken de polyline
            const mappedPoints = points.map((v, i) => {
                const x = (i / (maxPoints - 1)) * 500;
                // Clamp waarde tussen min en max en bereken Y (0-100)
                const clampedV = Math.min(Math.max(v, minVal), maxVal);
                const y = 100 - ((clampedV - minVal) / (maxVal - minVal) * 100);
                return `${x},${y}`;
            }).join(" ");

            polyline.setAttribute("points", mappedPoints);
            statusDisplay.innerText = "Verbonden ●";
            statusDisplay.style.color = "#00d1b2";

        } catch (error) {
            statusDisplay.innerText = "Server Offline ✖";
            statusDisplay.style.color = "#ff3860";
        }
    }

    // Start loop
    setInterval(fetchData, intervalTime);
    fetchData();
</script>

</body>
</html>
)rawliteral";