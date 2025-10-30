/*
 * Web Pages - Dashboard
 * Main landing page with status overview and navigation cards
 */

#ifndef WEB_PAGES_DASHBOARD_H
#define WEB_PAGES_DASHBOARD_H

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>VAIL SUMMIT - Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #fff;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        header {
            text-align: center;
            margin-bottom: 40px;
            padding: 30px 20px;
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            backdrop-filter: blur(10px);
        }
        h1 { font-size: 2.5rem; margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }
        .subtitle { font-size: 1.1rem; opacity: 0.9; }
        .status-bar {
            display: flex;
            gap: 20px;
            justify-content: center;
            margin-top: 20px;
            flex-wrap: wrap;
        }
        .status-item {
            background: rgba(255,255,255,0.15);
            padding: 10px 20px;
            border-radius: 8px;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        .status-icon { font-size: 1.5rem; }
        .dashboard-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 40px;
        }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 25px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
            transition: transform 0.2s, box-shadow 0.2s;
        }
        .card:hover {
            transform: translateY(-5px);
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
        }
        .card h2 {
            font-size: 1.5rem;
            margin-bottom: 15px;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        .card-icon { font-size: 2rem; }
        .card p { opacity: 0.9; margin-bottom: 15px; line-height: 1.6; }
        .btn {
            display: inline-block;
            padding: 12px 24px;
            background: rgba(255,255,255,0.2);
            color: #fff;
            text-decoration: none;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            transition: all 0.2s;
            cursor: pointer;
            font-size: 1rem;
        }
        .btn:hover {
            background: rgba(255,255,255,0.3);
            border-color: rgba(255,255,255,0.5);
        }
        .btn-primary {
            background: #00d4ff;
            color: #1e3c72;
            border: none;
            font-weight: 600;
        }
        .btn-primary:hover { background: #00b8e6; }
        footer {
            text-align: center;
            padding: 20px;
            opacity: 0.7;
            font-size: 0.9rem;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>📡 VAIL SUMMIT</h1>
            <p class="subtitle">Portable Morse Code Training Device</p>
            <div class="status-bar" id="statusBar">
                <div class="status-item">
                    <span class="status-icon">🔋</span>
                    <span id="battery">Loading...</span>
                </div>
                <div class="status-item">
                    <span class="status-icon">📶</span>
                    <span id="wifi">Connected</span>
                </div>
                <div class="status-item">
                    <span class="status-icon">📊</span>
                    <span id="qsoCount">0 QSOs</span>
                </div>
            </div>
        </header>

        <div class="dashboard-grid">
            <div class="card">
                <h2><span class="card-icon">📝</span> QSO Logger</h2>
                <p>View, manage, and export your contact logs. Download ADIF files for upload to QRZ, LoTW, and other services.</p>
                <a href="/logger" class="btn btn-primary">Open Logger</a>
            </div>

            <div class="card">
                <h2><span class="card-icon">⚙️</span> Device Settings</h2>
                <p>Configure CW speed, tone frequency, volume, key type, and other device preferences.</p>
                <a href="/settings" class="btn btn-primary">Manage Settings</a>
            </div>

            <div class="card">
                <h2><span class="card-icon">📡</span> WiFi Setup</h2>
                <p>Scan for networks and configure WiFi credentials for internet connectivity.</p>
                <a href="/wifi" class="btn btn-primary">WiFi Config</a>
            </div>

            <div class="card">
                <h2><span class="card-icon">ℹ️</span> System Info</h2>
                <p>View firmware version, memory usage, storage stats, and device diagnostics.</p>
                <a href="/system" class="btn btn-primary">View Info</a>
            </div>

            <div class="card">
                <h2><span class="card-icon">📻</span> Radio Control</h2>
                <p>Send morse code messages to your connected ham radio via 3.5mm output jack.</p>
                <a href="/radio" class="btn btn-primary">Radio Mode</a>
            </div>
        </div>

        <footer>
            <p>VAIL SUMMIT Web Interface | Firmware v__FIRMWARE_VERSION__ (Build: __FIRMWARE_DATE__)</p>
            <p>Access this page at: <strong>http://__MDNS_HOSTNAME__.local</strong></p>
        </footer>
    </div>

    <script>
        // Load device status
        async function loadStatus() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();

                document.getElementById('battery').textContent = data.battery;
                document.getElementById('wifi').textContent = data.wifi;
                document.getElementById('qsoCount').textContent = data.qsoCount + ' QSOs';
            } catch (error) {
                console.error('Failed to load status:', error);
            }
        }

        // Load status on page load and refresh every 10 seconds
        loadStatus();
        setInterval(loadStatus, 10000);
    </script>
</body>
</html>
)rawliteral";

// Function to render dashboard with dynamic values
void serveDashboard(AsyncWebServerRequest *request, const String& firmwareVersion, const String& firmwareDate, const String& mdnsHostname) {
  String html = String(DASHBOARD_HTML);
  html.replace("__FIRMWARE_VERSION__", firmwareVersion);
  html.replace("__FIRMWARE_DATE__", firmwareDate);
  html.replace("__MDNS_HOSTNAME__", mdnsHostname);
  request->send(200, "text/html", html);
}

#endif // WEB_PAGES_DASHBOARD_H
