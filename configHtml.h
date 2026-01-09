const char *configHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
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
        input, select { width: 100%; padding: 12px; border: 1px solid #ccd0d5; border-radius: 6px; box-sizing: border-box; transition: border 0.2s; }
        input:focus { border-color: #007bff; outline: none; }
        .row { display: flex; gap: 15px; }
        .btn-group { display: flex; gap: 10px; margin-top: 20px; }
        .btn { border: none; padding: 14px; width: 100%; border-radius: 6px; cursor: pointer; font-size: 1em; font-weight: bold; transition: opacity 0.2s; }
        .btn-save { background: #007bff; color: white; }
        .btn-export { background: #28a745; color: white; }
        .btn:hover { opacity: 0.9; }
        .lang-switch { cursor: pointer; color: #007bff; font-weight: bold; border: 1px solid #007bff; padding: 5px 12px; border-radius: 20px; font-size: 0.8em; }
    </style>
</head>
<body>

<div class="container">
    <div class="header">
        <h2 id="title">Configuratie</h2>
        <span class="lang-switch" onclick="toggleLanguage()" id="langBtn">English</span>
    </div>

    <form id="configForm" action="config" method="post">
        <div class="row">
            <div class="form-group" style="flex: 2;">
                <label id="lbl-code">Code (Hexadecimaal)</label>
                <input type="text" id="code" placeholder="0x1A2B" pattern="^0x[0-9a-fA-F]+$" required>
            </div>
            <div class="form-group" style="flex: 1;">
                <label id="lbl-bytes">Bytes</label>
                <input type="number" id="bytes" min="1" max="8" value="1">
            </div>
        </div>

        <div class="form-group">
            <label id="lbl-endian">Endianness</label>
            <select id="endian">
                <option value="little">Little Endian</option>
                <option value="big">Big Endian</option>
            </select>
        </div>

        <div class="row">
            <div class="form-group" style="flex: 1;">
                <label id="lbl-range">Bereik (Range)</label>
                <input type="text" id="range" placeholder="0-100">
            </div>
            <div class="form-group" style="flex: 1;">
                <label id="lbl-max">Max. Waarde</label>
                <input type="number" id="max_value" step="any">
            </div>
        </div>

        <div class="form-group">
            <label id="lbl-factor">Vermenigvuldigings factor</label>
            <input type="number" id="factor" step="0.0001" value="1.0">
        </div>

        <div class="form-group">
            <label id="lbl-type">Visualisatie Type</label>
            <select id="viz_type">
                <option value="graph" data-nl="Grafiek" data-en="Graph">Grafiek</option>
                <option value="bar" data-nl="Balk" data-en="Bar">Balk</option>
                <option value="pie" data-nl="Cirkel" data-en="Pie">Cirkel</option>
                <option value="number" data-nl="Getal" data-en="Number">Getal</option>
            </select>
        </div>

        <div class="btn-group">
            <button type="submit" class="btn btn-save" id="btn-save" onClick="postToServer()">Send</button>
            <button type="button" class="btn btn-export" id="btn-export" onclick="exportJSON()">Export JSON</button>
        </div>
    </form>
</div>

<script>
    let currentLang = 'nl';
    const translations = {
        nl: {
            title: "Configuratie", langBtn: "English", code: "Code (Hex)", 
            bytes: "Aantal Bytes", endian: "Endianness", range: "Bereik", 
            max: "Maximale Waarde", factor: "Factor", type: "Visualisatie", 
            save: "Zenden", export: "Exporteer JSON"
        },
        en: {
            title: "Configuration", langBtn: "Nederlands", code: "Code (Hex)", 
            bytes: "Byte Count", endian: "Endianness", range: "Range", 
            max: "Max Value", factor: "Multiplier", type: "Visualization", 
            save: "Send", export: "Export JSON"
        }
    };

    function toggleLanguage() {
        currentLang = currentLang === 'nl' ? 'en' : 'nl';
        const t = translations[currentLang];

        document.getElementById('title').innerText = t.title;
        document.getElementById('langBtn').innerText = t.langBtn;
        document.getElementById('lbl-code').innerText = t.code;
        document.getElementById('lbl-bytes').innerText = t.bytes;
        document.getElementById('lbl-endian').innerText = t.endian;
        document.getElementById('lbl-range').innerText = t.range;
        document.getElementById('lbl-max').innerText = t.max;
        document.getElementById('lbl-factor').innerText = t.factor;
        document.getElementById('lbl-type').innerText = t.type;
        document.getElementById('btn-save').innerText = t.save;
        document.getElementById('btn-export').innerText = t.export;

        const typeSelect = document.getElementById('viz_type');
        for (let option of typeSelect.options) {
            option.text = option.getAttribute(`data-${currentLang}`);
        }
	}
   	async function postToServer() {
        try {
            const config = {
                hexCode: document.getElementById('code').value,
                byteCount: parseInt(document.getElementById('bytes').value),
                endianness: document.getElementById('endian').value,
                range: document.getElementById('range').value,
                maxValue: parseFloat(document.getElementById('max_value').value),
                multiplier: parseFloat(document.getElementById('factor').value),
                visualization: document.getElementById('viz_type').value
            };
            const response = await fetch('/config'+
            '?code='+ document.getElementById('code').value +
            '&bytes='+ parseInt(document.getElementById('bytes').value) +
            '&endian='+ document.getElementById('endian').value +
            '&range='+ document.getElementById('range').value +
            '&max_value=' + parseFloat(document.getElementById('max_value').value) +
            '&factor=' + parseFloat(document.getElementById('factor').value) +
            '&viz_type=' +  document.getElementById('viz_type').value
            ,{
				method: 'POST',
				headers: {
					'Content-Type': 'application/json'
				},
				body: JSON.stringify(config)
			});
        } catch (error) {
            console.error('Fout bij verzenden:', error);
            alert(currentLang === 'nl' ? 'Fout bij het verbinden met de server.' : 'Error connecting to server.');
        }
    }

    function exportJSON() {
        const config = {
            hexCode: document.getElementById('code').value,
            byteCount: parseInt(document.getElementById('bytes').value),
            endianness: document.getElementById('endian').value,
            range: document.getElementById('range').value,
            maxValue: parseFloat(document.getElementById('max_value').value),
            multiplier: parseFloat(document.getElementById('factor').value),
            visualization: document.getElementById('viz_type').value,
            timestamp: new Date().toISOString()
        };

        const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(config, null, 4));
        const downloadAnchorNode = document.createElement('a');
        downloadAnchorNode.setAttribute("href", dataStr);
        downloadAnchorNode.setAttribute("download", "stream_config.json");
        document.body.appendChild(downloadAnchorNode);
        downloadAnchorNode.click();
        downloadAnchorNode.remove();
    }
</script>

</body>
</html>
)rawliteral";