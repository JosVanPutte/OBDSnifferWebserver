const char *configHtml = R"rawliteral(
<!DOCTYPE html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Stream Configurator</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #f0f2f5; padding: 40px; color: #333; }
        .container { max-width: 650px; background: white; padding: 30px; border-radius: 12px; box-shadow: 0 4px 20px rgba(0,0,0,0.08); margin: auto; }
        .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid #f0f0f0; margin-bottom: 25px; padding-bottom: 15px; }
        .form-group { margin-bottom: 18px; }
        label { display: block; margin-bottom: 6px; font-weight: 600; font-size: 0.9em; color: #555; }
        input, select { width: 100%; padding: 12px; border: 1px solid #ccd0d5; border-radius: 6px; box-sizing: border-box; }
        .row { display: flex; gap: 15px; }
        .btn-group { display: flex; gap: 10px; margin-top: 20px; }
        .btn { border: none; padding: 14px; width: 100%; border-radius: 6px; cursor: pointer; font-weight: bold; }
        .btn-save { background: #007bff; color: white; }
        .btn-export { background: #28a745; color: white; }
        .lang-switch { cursor: pointer; color: #007bff; font-weight: bold; border: 1px solid #007bff; padding: 5px 12px; border-radius: 20px; font-size: 0.8em; }
    </style>
</head>
<body>

<div class="container">
    <div class="header">
        <h2 id="title">Configuratie</h2>
    </div>

    <form id="configForm">
        <div class="row">
            <div class="form-group" style="flex: 1;">
                <label id="lbl-code">Code (Hex)</label>
                <input type="text" id="code" placeholder="0x1A2B" pattern="^0x[0-9a-fA-F]+$" required>
            </div>
            <div class="form-group" style="flex: 2;">
                <label id="lbl-name">Naam</label>
                <input type="text" id="name" required>
            </div>
        </div>

        <div class="row">
            <div class="form-group" style="flex: 1;">
                <label id="lbl-datatype">Datatype</label>
                <select id="datatype">
                    <option value="byte">Byte</option>
                    <option value="word">Word</option>
                </select>
            </div>
            <div class="form-group" style="flex: 1;">
                <label id="lbl-offset">Offset</label>
                <input type="number" id="offset">
            </div>
        </div>

        <div class="row">
            <div class="form-group" style="flex: 1;">
                <label id="lbl-bits">Bits</label>
                <input type="number" id="bits" min="1" max="16">
            </div>
            <div class="form-group" style="flex: 1;">
                <label id="lbl-endian">Endianness</label>
                <select id="endian">
                    <option value="little">Little Endian</option>
                    <option value="big">Big Endian</option>
                </select>
            </div>
        </div>

        <div class="row">
            <div class="form-group" style="flex: 1;">
                <label id="lbl-max">Max. Waarde</label>
                <input type="number" id="max_value" step="any">
            </div>
            <div class="form-group" style="flex: 1;">
                <label id="lbl-min">Min. Waarde</label>
                <input type="number" id="min" step="any">
            </div>
            <div class="form-group" style="flex: 1;">
                <label id="lbl-factor">Vermenigvuldigings factor</label>
                <input type="number" id="factor" step="0.0001">
            </div>

            <div class="form-group" style="flex: 1;">
                <label id="lbl-type">Visualisatie Type</label>
                <select id="viz_type">
                    <option value="graph" data-nl="Grafiek" data-en="Graph">Grafiek</option>
                    <option value="bar" data-nl="Balk" data-en="Bar">Balk</option>
                    <option value="number" data-nl="Getal" data-en="Number">Getal</option>
                    <option value="pie" data-nl="Meter" data-en="Gauge">Meter</option>
                </select>
            </div>
        </div>
        <div class="btn-group">
            <button type="button" class="btn btn-save" id="btn-save" onclick="postToServer()">Send</button>
            <button type="button" class="btn btn-export" id="btn-export" onclick="exportJSON()">Export JSON</button>
        </div>
    </form>
</div>

<script>
    let currentLang = 'nl';
    const translations = {
        nl: {
            title: "Configuratie", langBtn: "English", name: "Naam", code: "Code (Hex)", 
            offset: "Offset", datatype: "Datatype",
            bits: "Aantal Bits", endian: "Endianness", min: "Minimale Waarde",
            max: "Maximale Waarde", factor: "Factor", type: "Visualisatie", 
            save: "Zenden", export: "Exporteer JSON"
        },
        en: {
            title: "Configuration", langBtn: "Nederlands", name: "Name", code: "Code (Hex)", 
            offset: "Offset", datatype: "Data Type",
            bits: "Bit Count", endian: "Endianness", min: "Min Value",
            max: "Max Value", factor: "Multiplier", type: "Visualization", 
            save: "Send", export: "Export JSON"
        }
    };

    window.onload = function() {
        const urlParams = new URLSearchParams(window.location.search);
        
        // Vul velden op basis van URL (bijv: ?name=Motor&offset=10&datatype=float)
        const fields = ['name', 'code', 'offset', 'datatype', 'bits', 'endian', 'max_value', 'min', 'factor', 'viz_type'];
        fields.forEach(field => {
            const val = urlParams.get(field);
            if (val !== null) {
                const el = document.getElementById(field);
                if (el) el.value = val;
            }
        });

        currentLang = urlParams.get('lang') || 'nl';
        setLanguage();
    }

    function setLanguage() {
        const t = translations[currentLang];
        Object.keys(t).forEach(key => {
            const el = document.getElementById('lbl-' + key) || document.getElementById(key) || document.getElementById('btn-' + key);
            if (el && key !== 'title' && key !== 'langBtn') el.innerText = t[key];
        });
        document.getElementById('title').innerText = t.title;
        document.getElementById('langBtn').innerText = t.langBtn;
        
        // Update de dropdown opties (data-nl / data-en)
        const typeSelect = document.getElementById('viz_type');
        for (let opt of typeSelect.options) {
            opt.text = opt.getAttribute(`data-${currentLang}`);
        }
    }

    function toggleLanguage() {
        currentLang = currentLang === 'nl' ? 'en' : 'nl';
        setLanguage();
    }

    async function postToServer() {
        // Verzamel data voor de POST request
        const config = {};
        ['name', 'code', 'offset', 'datatype', 'bits', 'endian', 'max_value', 'min', 'factor', 'viz_type']
        .forEach(id => config[id] = document.getElementById(id).value);

        console.log("Verzenden:", config);
        const response = await fetch('/config'+
            '?name='+ document.getElementById('name').value +
            '&code='+ document.getElementById('code').value +
            '&offset='+ document.getElementById('offset').value +
            '&datatype='+ document.getElementById('datatype').value +
            '&bits='+ parseInt(document.getElementById('bits').value) +
            '&endian='+ document.getElementById('endian').value +
            '&max_value=' + parseFloat(document.getElementById('max_value').value) +
            '&min=' + parseFloat(document.getElementById('min').value) +
            '&factor=' + parseFloat(document.getElementById('factor').value) +
            '&viz_type=' +  document.getElementById('viz_type').value
            ,{
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(config)
            });

        alert("Data verzonden! Zie console.");
        // Hier kun je jouw fetch-logica plaatsen
    }

    function exportJSON() {
        const config = {};
        ['name', 'code', 'offset', 'datatype', 'bits', 'endian', 'max_value', 'min', 'factor', 'viz_type']
        .forEach(id => config[id] = document.getElementById(id).value);

        const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(config, null, 4));
        const downloadAnchorNode = document.createElement('a');
        downloadAnchorNode.setAttribute("href", dataStr);
        downloadAnchorNode.setAttribute("download", "config.json");
        document.body.appendChild(downloadAnchorNode);
        downloadAnchorNode.click();
        downloadAnchorNode.remove();
    }
</script>

</body>
</html>
)rawliteral";