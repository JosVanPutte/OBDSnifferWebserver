const char *testHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
    <meta charset="UTF-8">
    <title>URL Parameter Stream</title>
    <style>
        body { font-family: sans-serif; background: #121212; color: white; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
        .card { background: #1e1e1e; padding: 40px; border-radius: 12px; text-align: center; border: 1px solid #00d1b2; }
        .label { font-size: 1.2rem; color: #aaa; margin-bottom: 10px; }
        .value { font-size: 4rem; font-family: monospace; color: #00d1b2; }
    </style>
</head>
<body>

<div class="card">
    <div id="display-label" class="label">Wachten op parameters...</div>
    <div id="display-value" class="value">0.00</div>
</div>

<script>
    // 1. Haal de parameters uit de URL (bijv. ?label=Jopie&min=10&max=20)
    const urlParams = new URLSearchParams(window.location.search);
    
    // Gebruik de waarden uit de URL, of een standaardwaarde (fallback) als ze ontbreken
    const config = {
        label: urlParams.get('label') || "Waarde",
        min: parseFloat(urlParams.get('min')) || 0,
        max: parseFloat(urlParams.get('max')) || 100
    };

    // Toon direct het label
    document.getElementById('display-label').innerText = config.label;

    let currentValue = config.min;
    let direction = 1;

    // 2. De simulatie loop
    function updateValue() {
        const step = (config.max - config.min) / 100;
        currentValue += (step * direction);

        if (currentValue >= config.max || currentValue <= config.min) {
            direction *= -1;
        }

        document.getElementById('display-value').innerText = currentValue.toFixed(2);
    }

    // Start de loop elke 100ms
    setInterval(updateValue, 100);
</script>
</body>
</html>
)rawliteral";