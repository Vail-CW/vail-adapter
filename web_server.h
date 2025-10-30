/*
 * Web Server Module
 * Provides a comprehensive web interface for device management
 * Features: QSO logging, settings management, device status
 *
 * Access via: http://vail-summit.local or device IP address
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

// Note: qso_logger_storage.h is already included by the main .ino file before this header
#include "web_logger_enhanced.h"

// Global web server instance
AsyncWebServer webServer(80);

// mDNS hostname
String mdnsHostname = "vail-summit";

// Server state
bool webServerRunning = false;

// Forward declarations
void setupWebServer();
void stopWebServer();
String getDeviceStatusJSON();
String getQSOLogsJSON();
String generateADIF();
String generateCSV();

/*
 * Initialize and start the web server
 * Called automatically when WiFi connects
 */
void setupWebServer() {
  if (webServerRunning) {
    Serial.println("Web server already running");
    return;
  }

  Serial.println("Starting web server...");

  // Set up mDNS responder
  if (MDNS.begin(mdnsHostname.c_str())) {
    Serial.print("mDNS responder started: http://");
    Serial.print(mdnsHostname);
    Serial.println(".local");
    MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println("Error setting up mDNS responder!");
  }

  // ============================================
  // Main Dashboard Page
  // ============================================
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
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
            <p>VAIL SUMMIT Web Interface | Firmware v)rawliteral" + String(FIRMWARE_VERSION) + R"rawliteral( (Build: )rawliteral" + String(FIRMWARE_DATE) + R"rawliteral()</p>
            <p>Access this page at: <strong>http://)rawliteral" + mdnsHostname + R"rawliteral(.local</strong></p>
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
    request->send(200, "text/html", html);
  });

  // ============================================
  // QSO Logger Page (Enhanced)
  // ============================================
  webServer.on("/logger", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", LOGGER_HTML);
  });

  // ============================================
  // Radio Control Page
  // ============================================
  webServer.on("/radio", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Radio Control - VAIL SUMMIT</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #fff;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 800px; margin: 0 auto; }
        header {
            text-align: center;
            margin-bottom: 30px;
            padding: 20px;
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            backdrop-filter: blur(10px);
        }
        h1 { font-size: 2rem; margin-bottom: 10px; }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 25px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
            margin-bottom: 20px;
        }
        .card h2 { font-size: 1.5rem; margin-bottom: 15px; }
        .status-badge {
            display: inline-block;
            padding: 8px 16px;
            border-radius: 8px;
            font-size: 0.9rem;
            font-weight: 600;
            margin-bottom: 15px;
        }
        .status-inactive { background: rgba(255,255,255,0.2); }
        .status-active { background: #00d4ff; color: #1e3c72; }
        .form-group { margin-bottom: 20px; }
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            font-size: 0.9rem;
            text-transform: uppercase;
            letter-spacing: 1px;
            opacity: 0.9;
        }
        textarea {
            width: 100%;
            padding: 12px;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            background: rgba(255,255,255,0.1);
            color: #fff;
            font-size: 1rem;
            font-family: 'Courier New', monospace;
            resize: vertical;
            min-height: 100px;
        }
        textarea::placeholder { color: rgba(255,255,255,0.5); }
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
            font-weight: 600;
            margin-right: 10px;
        }
        .btn:hover {
            background: rgba(255,255,255,0.3);
            border-color: rgba(255,255,255,0.5);
        }
        .btn-primary {
            background: #00d4ff;
            color: #1e3c72;
            border: none;
        }
        .btn-primary:hover { background: #00b8e6; }
        .btn-danger {
            background: #ff4444;
            color: #fff;
            border: none;
        }
        .btn-danger:hover { background: #cc0000; }
        .message {
            padding: 12px;
            border-radius: 8px;
            margin-bottom: 15px;
            display: none;
        }
        .message.success { background: rgba(0,255,0,0.2); border: 1px solid rgba(0,255,0,0.5); }
        .message.error { background: rgba(255,0,0,0.2); border: 1px solid rgba(255,0,0,0.5); }
        .char-count {
            text-align: right;
            font-size: 0.85rem;
            opacity: 0.7;
            margin-top: 5px;
        }
        .info-box {
            background: rgba(0,212,255,0.1);
            border-left: 4px solid #00d4ff;
            padding: 15px;
            border-radius: 8px;
            margin-top: 20px;
        }
        .info-box p { opacity: 0.9; line-height: 1.6; margin-bottom: 10px; }
        .info-box p:last-child { margin-bottom: 0; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>📻 Radio Control</h1>
            <p>Send morse code messages to your connected radio</p>
        </header>

        <div class="card">
            <h2>Radio Mode Status</h2>
            <span class="status-badge" id="radioStatus">Checking...</span>
            <div id="messageBox" class="message"></div>

            <div style="margin-top: 20px;">
                <button class="btn btn-primary" id="enterRadioBtn" onclick="enterRadioMode()">
                    Enter Radio Mode
                </button>
                <button class="btn" onclick="window.location.href='/'">
                    Back to Dashboard
                </button>
            </div>
        </div>

        <div class="card">
            <h2>Transmission Settings</h2>
            <div class="form-group">
                <label for="wpmSlider">Speed (WPM)</label>
                <div style="display: flex; align-items: center; gap: 15px;">
                    <input type="range" id="wpmSlider" min="5" max="40" value="20"
                           style="flex: 1; height: 8px; border-radius: 4px; background: rgba(255,255,255,0.2); cursor: pointer;">
                    <span id="wpmDisplay" style="font-size: 1.5rem; font-weight: 600; min-width: 60px; text-align: right;">20 WPM</span>
                </div>
            </div>
        </div>

        <div class="card">
            <h2>Send Morse Code Message</h2>
            <div class="form-group">
                <label for="messageInput">Message to Send</label>
                <textarea id="messageInput" placeholder="Enter your message here (A-Z, 0-9, basic punctuation)"></textarea>
                <div class="char-count">
                    <span id="charCount">0</span> characters
                </div>
            </div>

            <button class="btn btn-primary" onclick="sendMessage()">
                Send Message
            </button>

            <div class="info-box">
                <p><strong>📡 Radio Output:</strong> Messages will be sent as morse code via the 3.5mm jack output.</p>
                <p><strong>⚙️ Speed & Settings:</strong> Uses your configured WPM speed and radio mode settings.</p>
                <p><strong>🔊 No Sidetone:</strong> Your radio provides the audio sidetone, not the Summit device.</p>
            </div>
        </div>
    </div>

    <script>
        // Update character count
        document.getElementById('messageInput').addEventListener('input', function(e) {
            document.getElementById('charCount').textContent = e.target.value.length;
        });

        // WPM slider handling
        document.getElementById('wpmSlider').addEventListener('input', function(e) {
            const wpm = e.target.value;
            document.getElementById('wpmDisplay').textContent = wpm + ' WPM';
        });

        document.getElementById('wpmSlider').addEventListener('change', function(e) {
            const wpm = parseInt(e.target.value);
            setWPM(wpm);
        });

        // Load current WPM speed
        async function loadWPM() {
            try {
                const response = await fetch('/api/radio/wpm');
                const data = await response.json();

                if (data.wpm) {
                    document.getElementById('wpmSlider').value = data.wpm;
                    document.getElementById('wpmDisplay').textContent = data.wpm + ' WPM';
                }
            } catch (error) {
                console.error('Failed to load WPM:', error);
            }
        }

        // Set WPM speed
        async function setWPM(wpm) {
            try {
                const response = await fetch('/api/radio/wpm', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ wpm: wpm })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('Speed updated to ' + wpm + ' WPM', 'success');
                } else {
                    showMessage('Failed to update speed: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to set WPM:', error);
                showMessage('Failed to update speed', 'error');
            }
        }

        // Check radio mode status
        async function checkRadioStatus() {
            try {
                const response = await fetch('/api/radio/status');
                const data = await response.json();

                const statusBadge = document.getElementById('radioStatus');
                const enterBtn = document.getElementById('enterRadioBtn');

                if (data.active) {
                    statusBadge.textContent = '✓ Radio Mode Active';
                    statusBadge.className = 'status-badge status-active';
                    enterBtn.textContent = 'Radio Mode Active';
                    enterBtn.disabled = true;
                    enterBtn.style.opacity = '0.6';
                    enterBtn.style.cursor = 'not-allowed';
                } else {
                    statusBadge.textContent = '○ Radio Mode Inactive';
                    statusBadge.className = 'status-badge status-inactive';
                    enterBtn.textContent = 'Enter Radio Mode';
                    enterBtn.disabled = false;
                    enterBtn.style.opacity = '1';
                    enterBtn.style.cursor = 'pointer';
                }
            } catch (error) {
                console.error('Failed to check radio status:', error);
                showMessage('Failed to check radio status', 'error');
            }
        }

        // Enter radio mode
        async function enterRadioMode() {
            try {
                const response = await fetch('/api/radio/enter', { method: 'POST' });
                const data = await response.json();

                if (data.success) {
                    showMessage('Device switched to Radio Mode', 'success');
                    checkRadioStatus();
                } else {
                    showMessage('Failed to enter radio mode: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to enter radio mode:', error);
                showMessage('Failed to enter radio mode', 'error');
            }
        }

        // Send message
        async function sendMessage() {
            const message = document.getElementById('messageInput').value.trim();

            if (message.length === 0) {
                showMessage('Please enter a message to send', 'error');
                return;
            }

            try {
                const response = await fetch('/api/radio/send', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ message: message })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('Message queued for transmission (' + message.length + ' characters)', 'success');
                    document.getElementById('messageInput').value = '';
                    document.getElementById('charCount').textContent = '0';
                } else {
                    showMessage('Failed to send message: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to send message:', error);
                showMessage('Failed to send message', 'error');
            }
        }

        // Show message
        function showMessage(text, type) {
            const messageBox = document.getElementById('messageBox');
            messageBox.textContent = text;
            messageBox.className = 'message ' + type;
            messageBox.style.display = 'block';

            setTimeout(() => {
                messageBox.style.display = 'none';
            }, 5000);
        }

        // Load initial state on page load
        loadWPM();
        checkRadioStatus();

        // Refresh status every 5 seconds
        setInterval(checkRadioStatus, 5000);
    </script>
</body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
  });

  // ============================================
  // Device Settings Page
  // ============================================
  webServer.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Device Settings - VAIL SUMMIT</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #fff;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 800px; margin: 0 auto; }
        header {
            text-align: center;
            margin-bottom: 30px;
            padding: 20px;
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            backdrop-filter: blur(10px);
        }
        h1 { font-size: 2rem; margin-bottom: 10px; }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 25px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
            margin-bottom: 20px;
        }
        .card h2 { font-size: 1.3rem; margin-bottom: 20px; }
        .form-group { margin-bottom: 25px; }
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            font-size: 0.9rem;
            text-transform: uppercase;
            letter-spacing: 1px;
            opacity: 0.9;
        }
        input[type="range"] {
            width: 100%;
            height: 8px;
            border-radius: 4px;
            background: rgba(255,255,255,0.2);
            outline: none;
            cursor: pointer;
        }
        input[type="text"] {
            width: 100%;
            padding: 12px;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            background: rgba(255,255,255,0.1);
            color: #fff;
            font-size: 1rem;
        }
        input[type="text"]::placeholder { color: rgba(255,255,255,0.5); }
        select {
            width: 100%;
            padding: 12px;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            background: rgba(255,255,255,0.15);
            color: #fff;
            font-size: 1rem;
            cursor: pointer;
        }
        select option {
            background: #1e3c72;
            color: #fff;
        }
        .slider-display {
            display: flex;
            align-items: center;
            gap: 15px;
            margin-top: 10px;
        }
        .slider-display span {
            font-size: 1.5rem;
            font-weight: 600;
            min-width: 80px;
            text-align: right;
        }
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
            font-weight: 600;
            margin-right: 10px;
        }
        .btn:hover {
            background: rgba(255,255,255,0.3);
            border-color: rgba(255,255,255,0.5);
        }
        .btn-primary {
            background: #00d4ff;
            color: #1e3c72;
            border: none;
        }
        .btn-primary:hover { background: #00b8e6; }
        .message {
            padding: 12px;
            border-radius: 8px;
            margin-bottom: 15px;
            display: none;
        }
        .message.success { background: rgba(0,255,0,0.2); border: 1px solid rgba(0,255,0,0.5); }
        .message.error { background: rgba(255,0,0,0.2); border: 1px solid rgba(255,0,0,0.5); }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>⚙️ Device Settings</h1>
            <p>Configure your VAIL SUMMIT device</p>
        </header>

        <div id="messageBox" class="message"></div>

        <div class="card">
            <h2>CW Settings</h2>

            <div class="form-group">
                <label for="wpmSlider">Speed (WPM)</label>
                <div class="slider-display">
                    <input type="range" id="wpmSlider" min="5" max="40" value="20">
                    <span id="wpmDisplay">20 WPM</span>
                </div>
            </div>

            <div class="form-group">
                <label for="toneSlider">Tone Frequency (Hz)</label>
                <div class="slider-display">
                    <input type="range" id="toneSlider" min="400" max="1200" step="50" value="700">
                    <span id="toneDisplay">700 Hz</span>
                </div>
            </div>

            <div class="form-group">
                <label for="keyTypeSelect">Key Type</label>
                <select id="keyTypeSelect">
                    <option value="0">Straight Key</option>
                    <option value="1">Iambic A</option>
                    <option value="2">Iambic B</option>
                </select>
            </div>

            <button class="btn btn-primary" onclick="saveCWSettings()">
                Save CW Settings
            </button>
        </div>

        <div class="card">
            <h2>Audio Settings</h2>

            <div class="form-group">
                <label for="volumeSlider">Volume (%)</label>
                <div class="slider-display">
                    <input type="range" id="volumeSlider" min="0" max="100" value="50">
                    <span id="volumeDisplay">50%</span>
                </div>
            </div>

            <button class="btn btn-primary" onclick="saveVolume()">
                Save Volume
            </button>
        </div>

        <div class="card">
            <h2>Station Settings</h2>

            <div class="form-group">
                <label for="callsignInput">Callsign</label>
                <input type="text" id="callsignInput" placeholder="W1ABC" maxlength="10">
            </div>

            <button class="btn btn-primary" onclick="saveCallsign()">
                Save Callsign
            </button>
        </div>

        <div style="text-align: center; margin-top: 30px;">
            <button class="btn" onclick="window.location.href='/'">
                Back to Dashboard
            </button>
        </div>
    </div>

    <script>
        // Update slider displays
        document.getElementById('wpmSlider').addEventListener('input', function(e) {
            document.getElementById('wpmDisplay').textContent = e.target.value + ' WPM';
        });

        document.getElementById('toneSlider').addEventListener('input', function(e) {
            document.getElementById('toneDisplay').textContent = e.target.value + ' Hz';
        });

        document.getElementById('volumeSlider').addEventListener('input', function(e) {
            document.getElementById('volumeDisplay').textContent = e.target.value + '%';
        });

        // Load all settings on page load
        async function loadSettings() {
            try {
                // Load CW settings
                const cwResponse = await fetch('/api/settings/cw');
                const cwData = await cwResponse.json();

                if (cwData.wpm !== undefined) {
                    document.getElementById('wpmSlider').value = cwData.wpm;
                    document.getElementById('wpmDisplay').textContent = cwData.wpm + ' WPM';
                }
                if (cwData.tone !== undefined) {
                    document.getElementById('toneSlider').value = cwData.tone;
                    document.getElementById('toneDisplay').textContent = cwData.tone + ' Hz';
                }
                if (cwData.keyType !== undefined) {
                    document.getElementById('keyTypeSelect').value = cwData.keyType;
                }

                // Load volume
                const volResponse = await fetch('/api/settings/volume');
                const volData = await volResponse.json();

                if (volData.volume !== undefined) {
                    document.getElementById('volumeSlider').value = volData.volume;
                    document.getElementById('volumeDisplay').textContent = volData.volume + '%';
                }

                // Load callsign
                const callResponse = await fetch('/api/settings/callsign');
                const callData = await callResponse.json();

                if (callData.callsign !== undefined) {
                    document.getElementById('callsignInput').value = callData.callsign;
                }
            } catch (error) {
                console.error('Failed to load settings:', error);
                showMessage('Failed to load settings', 'error');
            }
        }

        // Save CW settings
        async function saveCWSettings() {
            const wpm = parseInt(document.getElementById('wpmSlider').value);
            const tone = parseInt(document.getElementById('toneSlider').value);
            const keyType = parseInt(document.getElementById('keyTypeSelect').value);

            try {
                const response = await fetch('/api/settings/cw', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ wpm: wpm, tone: tone, keyType: keyType })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('CW settings saved successfully', 'success');
                } else {
                    showMessage('Failed to save CW settings: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to save CW settings:', error);
                showMessage('Failed to save CW settings', 'error');
            }
        }

        // Save volume
        async function saveVolume() {
            const volume = parseInt(document.getElementById('volumeSlider').value);

            try {
                const response = await fetch('/api/settings/volume', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ volume: volume })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('Volume saved successfully', 'success');
                } else {
                    showMessage('Failed to save volume: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to save volume:', error);
                showMessage('Failed to save volume', 'error');
            }
        }

        // Save callsign
        async function saveCallsign() {
            const callsign = document.getElementById('callsignInput').value.trim().toUpperCase();

            if (callsign.length === 0) {
                showMessage('Please enter a callsign', 'error');
                return;
            }

            try {
                const response = await fetch('/api/settings/callsign', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ callsign: callsign })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('Callsign saved successfully', 'success');
                    document.getElementById('callsignInput').value = callsign;
                } else {
                    showMessage('Failed to save callsign: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to save callsign:', error);
                showMessage('Failed to save callsign', 'error');
            }
        }

        // Show message
        function showMessage(text, type) {
            const messageBox = document.getElementById('messageBox');
            messageBox.textContent = text;
            messageBox.className = 'message ' + type;
            messageBox.style.display = 'block';

            setTimeout(() => {
                messageBox.style.display = 'none';
            }, 5000);
        }

        // Load settings on page load
        loadSettings();
    </script>
</body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
  });

  // ============================================
  // System Info Page
  // ============================================
  webServer.on("/system", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>System Info - VAIL SUMMIT</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #fff;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 1000px; margin: 0 auto; }
        header {
            text-align: center;
            margin-bottom: 30px;
            padding: 20px;
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            backdrop-filter: blur(10px);
        }
        h1 { font-size: 2rem; margin-bottom: 10px; }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 20px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
        }
        .card h2 { font-size: 1.2rem; margin-bottom: 15px; opacity: 0.9; }
        .info-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 10px 0;
            border-bottom: 1px solid rgba(255,255,255,0.1);
        }
        .info-row:last-child { border-bottom: none; }
        .info-label {
            font-size: 0.9rem;
            opacity: 0.7;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        .info-value {
            font-size: 1.1rem;
            font-weight: 600;
            text-align: right;
        }
        .signal-good { color: #00ff00; }
        .signal-ok { color: #ffff00; }
        .signal-poor { color: #ff4444; }
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
            font-weight: 600;
        }
        .btn:hover {
            background: rgba(255,255,255,0.3);
            border-color: rgba(255,255,255,0.5);
        }
        .last-update {
            text-align: center;
            opacity: 0.6;
            font-size: 0.85rem;
            margin-top: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>ℹ️ System Info</h1>
            <p>VAIL SUMMIT Diagnostics & Status</p>
        </header>

        <div class="grid">
            <div class="card">
                <h2>📱 Firmware</h2>
                <div class="info-row">
                    <span class="info-label">Version</span>
                    <span class="info-value" id="firmware">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Build Date</span>
                    <span class="info-value" id="firmwareDate">Loading...</span>
                </div>
            </div>

            <div class="card">
                <h2>⏱️ System</h2>
                <div class="info-row">
                    <span class="info-label">Uptime</span>
                    <span class="info-value" id="uptime">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">CPU Speed</span>
                    <span class="info-value" id="cpuSpeed">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Flash Size</span>
                    <span class="info-value" id="flashSize">Loading...</span>
                </div>
            </div>

            <div class="card">
                <h2>💾 Memory</h2>
                <div class="info-row">
                    <span class="info-label">Free RAM</span>
                    <span class="info-value" id="freeHeap">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Min Free RAM</span>
                    <span class="info-value" id="minFreeHeap">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Free PSRAM</span>
                    <span class="info-value" id="freePsram">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Min Free PSRAM</span>
                    <span class="info-value" id="minFreePsram">Loading...</span>
                </div>
            </div>

            <div class="card">
                <h2>💿 Storage</h2>
                <div class="info-row">
                    <span class="info-label">SPIFFS Used</span>
                    <span class="info-value" id="spiffsUsed">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">SPIFFS Total</span>
                    <span class="info-value" id="spiffsTotal">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">QSO Logs</span>
                    <span class="info-value" id="qsoCount">Loading...</span>
                </div>
            </div>

            <div class="card">
                <h2>📶 WiFi</h2>
                <div class="info-row">
                    <span class="info-label">Status</span>
                    <span class="info-value" id="wifiStatus">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">SSID</span>
                    <span class="info-value" id="wifiSSID">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">IP Address</span>
                    <span class="info-value" id="wifiIP">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Signal</span>
                    <span class="info-value" id="wifiSignal">Loading...</span>
                </div>
            </div>

            <div class="card">
                <h2>🔋 Battery</h2>
                <div class="info-row">
                    <span class="info-label">Voltage</span>
                    <span class="info-value" id="battVoltage">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Charge</span>
                    <span class="info-value" id="battPercent">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Monitor</span>
                    <span class="info-value" id="battMonitor">Loading...</span>
                </div>
            </div>
        </div>

        <div class="last-update" id="lastUpdate">Last updated: Never</div>

        <div style="text-align: center; margin-top: 20px;">
            <button class="btn" onclick="window.location.href='/'">
                Back to Dashboard
            </button>
        </div>
    </div>

    <script>
        // Format bytes to human readable
        function formatBytes(bytes) {
            if (bytes < 1024) return bytes + ' B';
            if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
            return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
        }

        // Format uptime
        function formatUptime(ms) {
            const seconds = Math.floor(ms / 1000);
            const minutes = Math.floor(seconds / 60);
            const hours = Math.floor(minutes / 60);
            const days = Math.floor(hours / 24);

            if (days > 0) return days + 'd ' + (hours % 24) + 'h';
            if (hours > 0) return hours + 'h ' + (minutes % 60) + 'm';
            if (minutes > 0) return minutes + 'm ' + (seconds % 60) + 's';
            return seconds + 's';
        }

        // Load system info
        async function loadSystemInfo() {
            try {
                const response = await fetch('/api/system/info');
                const data = await response.json();

                // Firmware
                document.getElementById('firmware').textContent = data.firmware || 'Unknown';
                document.getElementById('firmwareDate').textContent = data.firmwareDate || 'Unknown';

                // System
                document.getElementById('uptime').textContent = formatUptime(data.uptime || 0);
                document.getElementById('cpuSpeed').textContent = (data.cpuFreq || '?') + ' MHz';
                document.getElementById('flashSize').textContent = formatBytes(data.flashSize || 0);

                // Memory
                document.getElementById('freeHeap').textContent = formatBytes(data.freeHeap || 0);
                document.getElementById('minFreeHeap').textContent = formatBytes(data.minFreeHeap || 0);
                document.getElementById('freePsram').textContent = data.psramFound ? formatBytes(data.freePsram || 0) : 'N/A';
                document.getElementById('minFreePsram').textContent = data.psramFound ? formatBytes(data.minFreePsram || 0) : 'N/A';

                // Storage
                document.getElementById('spiffsUsed').textContent = formatBytes(data.spiffsUsed || 0);
                document.getElementById('spiffsTotal').textContent = formatBytes(data.spiffsTotal || 0);
                document.getElementById('qsoCount').textContent = (data.qsoCount || 0) + ' logs';

                // WiFi
                document.getElementById('wifiStatus').textContent = data.wifiConnected ? 'Connected' : 'Disconnected';
                document.getElementById('wifiSSID').textContent = data.wifiSSID || 'N/A';
                document.getElementById('wifiIP').textContent = data.wifiIP || 'N/A';

                const rssi = data.wifiRSSI || -100;
                let signalClass = 'signal-poor';
                if (rssi > -60) signalClass = 'signal-good';
                else if (rssi > -70) signalClass = 'signal-ok';
                document.getElementById('wifiSignal').innerHTML = '<span class="' + signalClass + '">' + rssi + ' dBm</span>';

                // Battery
                document.getElementById('battVoltage').textContent = (data.batteryVoltage || 0).toFixed(2) + ' V';
                document.getElementById('battPercent').textContent = (data.batteryPercent || 0).toFixed(0) + '%';
                document.getElementById('battMonitor').textContent = data.batteryMonitor || 'None';

                // Update timestamp
                const now = new Date();
                document.getElementById('lastUpdate').textContent = 'Last updated: ' + now.toLocaleTimeString();

            } catch (error) {
                console.error('Failed to load system info:', error);
            }
        }

        // Load on page load and refresh every 10 seconds
        loadSystemInfo();
        setInterval(loadSystemInfo, 10000);
    </script>
</body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
  });

  // ============================================
  // API Endpoints
  // ============================================

  // Device status endpoint
  webServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getDeviceStatusJSON());
  });

  // QSO logs list endpoint
  webServer.on("/api/qsos", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getQSOLogsJSON());
  });

  // ADIF export endpoint
  webServer.on("/api/export/adif", HTTP_GET, [](AsyncWebServerRequest *request) {
    String adif = generateADIF();
    AsyncWebServerResponse *response = request->beginResponse(200, "application/x-adif", adif);
    response->addHeader("Content-Disposition", "attachment; filename=vail-summit-logs.adi");
    request->send(response);
  });

  // CSV export endpoint
  webServer.on("/api/export/csv", HTTP_GET, [](AsyncWebServerRequest *request) {
    String csv = generateCSV();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csv);
    response->addHeader("Content-Disposition", "attachment; filename=vail-summit-logs.csv");
    request->send(response);
  });

  // ============================================
  // Station Settings Endpoints
  // ============================================

  // Get station settings
  webServer.on("/api/settings/station", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;

    Preferences prefs;
    prefs.begin("qso_operator", true);

    char callsign[11] = "";
    char gridsquare[9] = "";
    char pota[11] = "";

    prefs.getString("callsign", callsign, sizeof(callsign));
    prefs.getString("gridsquare", gridsquare, sizeof(gridsquare));
    prefs.getString("pota", pota, sizeof(pota));

    prefs.end();

    doc["callsign"] = callsign;
    doc["gridsquare"] = gridsquare;
    doc["pota"] = pota;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Save station settings
  webServer.on("/api/settings/station", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Parse JSON body
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Save to Preferences
      Preferences prefs;
      prefs.begin("qso_operator", false);

      if (doc.containsKey("callsign")) {
        prefs.putString("callsign", doc["callsign"].as<String>());
      }
      if (doc.containsKey("gridsquare")) {
        prefs.putString("gridsquare", doc["gridsquare"].as<String>());
      }
      if (doc.containsKey("pota")) {
        prefs.putString("pota", doc["pota"].as<String>());
      }

      prefs.end();

      Serial.println("Station settings saved via web interface");

      request->send(200, "application/json", "{\"success\":true}");
    });

  // ============================================
  // QSO CRUD Endpoints
  // ============================================

  // Create new QSO
  webServer.on("/api/qsos/create", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Parse JSON body
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Extract QSO data - initialize struct to zero first!
      QSO newQSO;
      memset(&newQSO, 0, sizeof(QSO));

      newQSO.id = doc["id"] | (unsigned long)millis();
      strlcpy(newQSO.callsign, doc["callsign"] | "", sizeof(newQSO.callsign));
      newQSO.frequency = doc["frequency"] | 0.0f;
      strlcpy(newQSO.mode, doc["mode"] | "", sizeof(newQSO.mode));

      // Calculate band from frequency
      String band = frequencyToBand(newQSO.frequency);
      strlcpy(newQSO.band, band.c_str(), sizeof(newQSO.band));

      strlcpy(newQSO.rst_sent, doc["rst_sent"] | "", sizeof(newQSO.rst_sent));
      strlcpy(newQSO.rst_rcvd, doc["rst_rcvd"] | "", sizeof(newQSO.rst_rcvd));
      strlcpy(newQSO.date, doc["date"] | "", sizeof(newQSO.date));
      strlcpy(newQSO.time_on, doc["time_on"] | "", sizeof(newQSO.time_on));
      strlcpy(newQSO.gridsquare, doc["gridsquare"] | "", sizeof(newQSO.gridsquare));
      strlcpy(newQSO.my_gridsquare, doc["my_gridsquare"] | "", sizeof(newQSO.my_gridsquare));
      strlcpy(newQSO.my_pota_ref, doc["my_pota_ref"] | "", sizeof(newQSO.my_pota_ref));
      strlcpy(newQSO.their_pota_ref, doc["their_pota_ref"] | "", sizeof(newQSO.their_pota_ref));
      strlcpy(newQSO.notes, doc["notes"] | "", sizeof(newQSO.notes));

      // If no date provided, use current date
      if (strlen(newQSO.date) == 0) {
        String dateTime = formatCurrentDateTime();
        strlcpy(newQSO.date, dateTime.substring(0, 8).c_str(), sizeof(newQSO.date));
        strlcpy(newQSO.time_on, dateTime.substring(9, 13).c_str(), sizeof(newQSO.time_on));
      }

      // Save QSO using existing function
      if (saveQSO(newQSO)) {
        Serial.println("QSO created via web interface");
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save QSO\"}");
      }
    });

  // Update existing QSO
  webServer.on("/api/qsos/update", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Parse JSON body
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Extract QSO data
      const char* date = doc["date"] | "";
      unsigned long id = doc["id"] | 0;

      if (strlen(date) == 0 || id == 0) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing date or id\"}");
        return;
      }

      // Load the day's log file
      String filename = "/logs/qso_" + String(date) + ".json";
      File file = FileSystem.open(filename, "r");
      if (!file) {
        request->send(404, "application/json", "{\"success\":false,\"error\":\"Log file not found\"}");
        return;
      }

      String content = file.readString();
      file.close();

      // Parse log file
      JsonDocument logDoc;
      error = deserializeJson(logDoc, content);
      if (error) {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to parse log file\"}");
        return;
      }

      // Find and update the QSO
      bool found = false;
      JsonArray logs = logDoc["logs"].as<JsonArray>();
      for (JsonObject qso : logs) {
        if (qso["id"].as<unsigned long>() == id) {
          // Update all fields
          qso["callsign"] = doc["callsign"];
          float frequency = doc["frequency"];
          qso["frequency"] = frequency;
          qso["mode"] = doc["mode"];

          // Calculate band from frequency
          String band = frequencyToBand(frequency);
          qso["band"] = band;

          qso["rst_sent"] = doc["rst_sent"];
          qso["rst_rcvd"] = doc["rst_rcvd"];
          qso["gridsquare"] = doc["gridsquare"];
          qso["my_gridsquare"] = doc["my_gridsquare"];
          qso["my_pota_ref"] = doc["my_pota_ref"];
          qso["their_pota_ref"] = doc["their_pota_ref"];
          qso["notes"] = doc["notes"];
          found = true;
          break;
        }
      }

      if (!found) {
        request->send(404, "application/json", "{\"success\":false,\"error\":\"QSO not found\"}");
        return;
      }

      // Save updated log file
      file = FileSystem.open(filename, "w");
      if (!file) {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to open file for writing\"}");
        return;
      }

      serializeJson(logDoc, file);
      file.close();

      Serial.println("QSO updated via web interface");
      request->send(200, "application/json", "{\"success\":true}");
    });

  // Delete QSO
  webServer.on("/api/qsos/delete", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("date") || !request->hasParam("id")) {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing date or id\"}");
      return;
    }

    String date = request->getParam("date")->value();
    unsigned long id = request->getParam("id")->value().toInt();

    // Load the day's log file
    String filename = "/logs/qso_" + date + ".json";
    File file = FileSystem.open(filename, "r");
    if (!file) {
      request->send(404, "application/json", "{\"success\":false,\"error\":\"Log file not found\"}");
      return;
    }

    String content = file.readString();
    file.close();

    // Parse log file
    JsonDocument logDoc;
    DeserializationError error = deserializeJson(logDoc, content);
    if (error) {
      request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to parse log file\"}");
      return;
    }

    // Find and remove the QSO
    bool found = false;
    JsonArray logs = logDoc["logs"].as<JsonArray>();
    for (size_t i = 0; i < logs.size(); i++) {
      if (logs[i]["id"].as<unsigned long>() == id) {
        logs.remove(i);
        found = true;
        break;
      }
    }

    if (!found) {
      request->send(404, "application/json", "{\"success\":false,\"error\":\"QSO not found\"}");
      return;
    }

    // Update count
    int newCount = logs.size();
    logDoc["count"] = newCount;

    // If no QSOs left, delete the file
    if (newCount == 0) {
      FileSystem.remove(filename);
      Serial.println("Log file deleted (no QSOs remaining)");
    } else {
      // Save updated log file
      file = FileSystem.open(filename, "w");
      if (!file) {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to open file for writing\"}");
        return;
      }

      serializeJson(logDoc, file);
      file.close();
    }

    // Note: Metadata will be refreshed when QSO logger loads on device
    // or when the page reloads (it counts logs dynamically in getQSOLogsJSON)

    Serial.println("QSO deleted via web interface");
    request->send(200, "application/json", "{\"success\":true}");
  });

  // ============================================
  // Radio Control API Endpoints
  // ============================================

  // Radio status endpoint
  webServer.on("/api/radio/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    extern MenuMode currentMode;
    extern bool radioOutputActive;

    JsonDocument doc;
    doc["active"] = (currentMode == MODE_RADIO_OUTPUT && radioOutputActive);
    doc["mode"] = (currentMode == MODE_RADIO_OUTPUT) ? "radio_output" : "other";

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Enter radio mode endpoint
  webServer.on("/api/radio/enter", HTTP_POST, [](AsyncWebServerRequest *request) {
    extern MenuMode currentMode;
    extern Adafruit_ST7789 tft;

    // Switch to radio output mode
    currentMode = MODE_RADIO_OUTPUT;
    startRadioOutput(tft);

    Serial.println("Switched to Radio Output mode via web interface");

    request->send(200, "application/json", "{\"success\":true}");
  });

  // Send morse message endpoint
  webServer.on("/api/radio/send", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Parse JSON body
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      const char* message = doc["message"] | "";
      if (strlen(message) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Message is empty\"}");
        return;
      }

      // Queue message for transmission
      if (queueRadioMessage(message)) {
        Serial.print("Queued radio message: ");
        Serial.println(message);
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Message queue is full\"}");
      }
    });

  // Get WPM speed endpoint
  webServer.on("/api/radio/wpm", HTTP_GET, [](AsyncWebServerRequest *request) {
    extern int cwSpeed;

    JsonDocument doc;
    doc["wpm"] = cwSpeed;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Set WPM speed endpoint
  webServer.on("/api/radio/wpm", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Parse JSON body
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      int wpm = doc["wpm"] | 0;
      if (wpm < 5 || wpm > 40) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"WPM must be between 5 and 40\"}");
        return;
      }

      // Update global CW speed
      extern int cwSpeed;
      cwSpeed = wpm;

      // Save to preferences
      saveCWSettings();

      Serial.print("CW speed updated to ");
      Serial.print(wpm);
      Serial.println(" WPM via web interface");

      request->send(200, "application/json", "{\"success\":true}");
    });

  // ============================================
  // Device Settings API Endpoints
  // ============================================

  // Get CW settings
  webServer.on("/api/settings/cw", HTTP_GET, [](AsyncWebServerRequest *request) {
    extern int cwSpeed;
    extern int cwTone;
    extern KeyType cwKeyType;

    JsonDocument doc;
    doc["wpm"] = cwSpeed;
    doc["tone"] = cwTone;
    doc["keyType"] = (int)cwKeyType;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Set CW settings
  webServer.on("/api/settings/cw", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Validate and update settings
      extern int cwSpeed;
      extern int cwTone;
      extern KeyType cwKeyType;

      if (doc.containsKey("wpm")) {
        int wpm = doc["wpm"];
        if (wpm < 5 || wpm > 40) {
          request->send(400, "application/json", "{\"success\":false,\"error\":\"WPM must be between 5 and 40\"}");
          return;
        }
        cwSpeed = wpm;
      }

      if (doc.containsKey("tone")) {
        int tone = doc["tone"];
        if (tone < 400 || tone > 1200) {
          request->send(400, "application/json", "{\"success\":false,\"error\":\"Tone must be between 400 and 1200 Hz\"}");
          return;
        }
        cwTone = tone;
      }

      if (doc.containsKey("keyType")) {
        int keyType = doc["keyType"];
        if (keyType < 0 || keyType > 2) {
          request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid key type\"}");
          return;
        }
        cwKeyType = (KeyType)keyType;
      }

      // Save to preferences
      saveCWSettings();

      Serial.println("CW settings updated via web interface");

      request->send(200, "application/json", "{\"success\":true}");
    });

  // Get volume
  webServer.on("/api/settings/volume", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["volume"] = getVolume();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Set volume
  webServer.on("/api/settings/volume", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      int volume = doc["volume"] | -1;
      if (volume < 0 || volume > 100) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Volume must be between 0 and 100\"}");
        return;
      }

      // Update volume
      setVolume(volume);

      Serial.print("Volume updated to ");
      Serial.print(volume);
      Serial.println("% via web interface");

      request->send(200, "application/json", "{\"success\":true}");
    });

  // Get callsign
  webServer.on("/api/settings/callsign", HTTP_GET, [](AsyncWebServerRequest *request) {
    extern String vailCallsign;

    JsonDocument doc;
    doc["callsign"] = vailCallsign;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Set callsign
  webServer.on("/api/settings/callsign", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      String callsign = doc["callsign"] | "";
      callsign.trim();
      callsign.toUpperCase();

      if (callsign.length() == 0) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Callsign cannot be empty\"}");
        return;
      }

      if (callsign.length() > 10) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Callsign too long (max 10 characters)\"}");
        return;
      }

      // Update callsign
      extern String vailCallsign;
      vailCallsign = callsign;
      saveCallsign(callsign);

      Serial.print("Callsign updated to ");
      Serial.print(callsign);
      Serial.println(" via web interface");

      request->send(200, "application/json", "{\"success\":true}");
    });

  // ============================================
  // System Info API Endpoint
  // ============================================

  // Get system info
  webServer.on("/api/system/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;

    // Firmware
    doc["firmware"] = FIRMWARE_VERSION;
    doc["firmwareDate"] = FIRMWARE_DATE;
    doc["firmwareName"] = FIRMWARE_NAME;

    // Chip info
    doc["chipModel"] = ESP.getChipModel();
    doc["chipRevision"] = ESP.getChipRevision();

    // System
    doc["uptime"] = millis();
    doc["cpuFreq"] = ESP.getCpuFreqMHz();
    doc["flashSize"] = ESP.getFlashChipSize();

    // Memory
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["minFreeHeap"] = ESP.getMinFreeHeap();
    doc["psramFound"] = psramFound();
    if (psramFound()) {
      doc["freePsram"] = ESP.getFreePsram();
      doc["minFreePsram"] = ESP.getMinFreePsram();
      doc["psramSize"] = ESP.getPsramSize();
    }

    // Storage
    doc["spiffsUsed"] = SPIFFS.usedBytes();
    doc["spiffsTotal"] = SPIFFS.totalBytes();
    doc["qsoCount"] = storageStats.totalLogs;

    // WiFi
    doc["wifiConnected"] = WiFi.isConnected();
    if (WiFi.isConnected()) {
      doc["wifiSSID"] = WiFi.SSID();
      doc["wifiIP"] = WiFi.localIP().toString();
      doc["wifiRSSI"] = WiFi.RSSI();
    }

    // Battery
    extern bool hasLC709203;
    extern bool hasMAX17048;
    extern Adafruit_LC709203F lc;
    extern Adafruit_MAX17048 maxlipo;

    float batteryVoltage = 0;
    float batteryPercent = 0;
    String batteryMonitor = "None";

    if (hasMAX17048) {
      batteryVoltage = maxlipo.cellVoltage();
      batteryPercent = maxlipo.cellPercent();
      batteryMonitor = "MAX17048";
    } else if (hasLC709203) {
      batteryVoltage = lc.cellVoltage();
      batteryPercent = lc.cellPercent();
      batteryMonitor = "LC709203F";
    }

    doc["batteryVoltage"] = batteryVoltage;
    doc["batteryPercent"] = batteryPercent;
    doc["batteryMonitor"] = batteryMonitor;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Start server
  webServer.begin();
  webServerRunning = true;

  Serial.println("Web server started successfully");
  Serial.print("Access at: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  Serial.print("Or via mDNS: http://");
  Serial.print(mdnsHostname);
  Serial.println(".local/");
}

/*
 * Stop the web server
 */
void stopWebServer() {
  if (!webServerRunning) {
    return;
  }

  webServer.end();
  MDNS.end();
  webServerRunning = false;
  Serial.println("Web server stopped");
}

/*
 * Get device status as JSON
 */
String getDeviceStatusJSON() {
  JsonDocument doc;

  // Battery status (using external variables)
  extern bool hasLC709203;
  extern bool hasMAX17048;
  extern Adafruit_LC709203F lc;
  extern Adafruit_MAX17048 maxlipo;

  float batteryVoltage = 0;
  float batteryPercent = 0;

  if (hasMAX17048) {
    batteryVoltage = maxlipo.cellVoltage();
    batteryPercent = maxlipo.cellPercent();
  } else if (hasLC709203) {
    batteryVoltage = lc.cellVoltage();
    batteryPercent = lc.cellPercent();
  }

  char batteryStr[32];
  snprintf(batteryStr, sizeof(batteryStr), "%.2fV (%.0f%%)", batteryVoltage, batteryPercent);
  doc["battery"] = batteryStr;

  // WiFi status
  doc["wifi"] = WiFi.isConnected() ? "Connected" : "Disconnected";
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();

  // QSO count
  doc["qsoCount"] = storageStats.totalLogs;

  // Firmware info
  doc["firmware"] = "1.0.0";

  String output;
  serializeJson(doc, output);
  return output;
}

/*
 * Get all QSO logs as JSON
 */
String getQSOLogsJSON() {
  JsonDocument doc;
  JsonArray logsArray = doc["logs"].to<JsonArray>();

  int totalCount = 0;

  // Iterate through all log files
  File root = FileSystem.open("/logs");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String filename = String(file.name());

        // Extract basename
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) {
          filename = filename.substring(lastSlash + 1);
        }

        // Process QSO log files
        if (filename.startsWith("qso_") && filename.endsWith(".json")) {
          File logFile = FileSystem.open(file.path(), "r");
          if (logFile) {
            String content = logFile.readString();
            logFile.close();

            JsonDocument logDoc;
            DeserializationError error = deserializeJson(logDoc, content);

            if (!error && logDoc.containsKey("logs")) {
              JsonArray qsos = logDoc["logs"].as<JsonArray>();
              for (JsonObject qso : qsos) {
                logsArray.add(qso);
                totalCount++;
              }
            }
          }
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }

  doc["total"] = totalCount;

  String output;
  serializeJson(doc, output);
  return output;
}

/*
 * Generate ADIF export file
 */
String generateADIF() {
  String adif = "";

  // ADIF header
  adif += "ADIF Export from VAIL SUMMIT\n";
  adif += "<PROGRAMID:11>VAIL SUMMIT\n";
  adif += "<PROGRAMVERSION:5>1.0.0\n";
  adif += "<ADIF_VER:5>3.1.4\n";
  adif += "<EOH>\n\n";

  // Iterate through all log files and generate ADIF records
  File root = FileSystem.open("/logs");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String filename = String(file.name());
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) {
          filename = filename.substring(lastSlash + 1);
        }

        if (filename.startsWith("qso_") && filename.endsWith(".json")) {
          File logFile = FileSystem.open(file.path(), "r");
          if (logFile) {
            String content = logFile.readString();
            logFile.close();

            JsonDocument logDoc;
            DeserializationError error = deserializeJson(logDoc, content);

            if (!error && logDoc.containsKey("logs")) {
              JsonArray qsos = logDoc["logs"].as<JsonArray>();
              for (JsonObject qso : qsos) {
                // Format each QSO as ADIF record
                const char* call = qso["callsign"] | "";
                if (strlen(call) > 0) {
                  adif += "<CALL:" + String(strlen(call)) + ">" + call + " ";
                }

                float freq = qso["frequency"] | 0.0f;
                if (freq > 0) {
                  String freqStr = String(freq, 6);
                  adif += "<FREQ:" + String(freqStr.length()) + ">" + freqStr + " ";
                }

                const char* mode = qso["mode"] | "";
                if (strlen(mode) > 0) {
                  adif += "<MODE:" + String(strlen(mode)) + ">" + mode + " ";
                }

                const char* date = qso["date"] | "";
                if (strlen(date) == 8) {
                  adif += "<QSO_DATE:8>" + String(date) + " ";
                }

                const char* time = qso["time_on"] | "";
                if (strlen(time) >= 4) {
                  String timeStr = String(time) + "00"; // Add seconds
                  adif += "<TIME_ON:6>" + timeStr + " ";
                }

                const char* rstS = qso["rst_sent"] | "";
                if (strlen(rstS) > 0) {
                  adif += "<RST_SENT:" + String(strlen(rstS)) + ">" + rstS + " ";
                }

                const char* rstR = qso["rst_rcvd"] | "";
                if (strlen(rstR) > 0) {
                  adif += "<RST_RCVD:" + String(strlen(rstR)) + ">" + rstR + " ";
                }

                const char* myGrid = qso["my_gridsquare"] | "";
                if (strlen(myGrid) > 0) {
                  adif += "<MY_GRIDSQUARE:" + String(strlen(myGrid)) + ">" + myGrid + " ";
                }

                const char* grid = qso["gridsquare"] | "";
                if (strlen(grid) > 0) {
                  adif += "<GRIDSQUARE:" + String(strlen(grid)) + ">" + grid + " ";
                }

                const char* myPota = qso["my_pota_ref"] | "";
                if (strlen(myPota) > 0) {
                  adif += "<MY_SIG:4>POTA <MY_SIG_INFO:" + String(strlen(myPota)) + ">" + myPota + " ";
                }

                const char* theirPota = qso["their_pota_ref"] | "";
                if (strlen(theirPota) > 0) {
                  adif += "<SIG:4>POTA <SIG_INFO:" + String(strlen(theirPota)) + ">" + theirPota + " ";
                }

                const char* notes = qso["notes"] | "";
                if (strlen(notes) > 0) {
                  adif += "<NOTES:" + String(strlen(notes)) + ">" + notes + " ";
                }

                adif += "<EOR>\n\n";
              }
            }
          }
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }

  return adif;
}

/*
 * Generate CSV export file
 */
String generateCSV() {
  String csv = "";

  // CSV header
  csv += "Date,Time,Callsign,Frequency,Band,Mode,RST Sent,RST Rcvd,My Grid,My POTA,Their Grid,Their POTA,Notes\n";

  // Iterate through all log files
  File root = FileSystem.open("/logs");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String filename = String(file.name());
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) {
          filename = filename.substring(lastSlash + 1);
        }

        if (filename.startsWith("qso_") && filename.endsWith(".json")) {
          File logFile = FileSystem.open(file.path(), "r");
          if (logFile) {
            String content = logFile.readString();
            logFile.close();

            JsonDocument logDoc;
            DeserializationError error = deserializeJson(logDoc, content);

            if (!error && logDoc.containsKey("logs")) {
              JsonArray qsos = logDoc["logs"].as<JsonArray>();
              for (JsonObject qso : qsos) {
                // Format date YYYYMMDD to YYYY-MM-DD
                String date = qso["date"] | "";
                if (date.length() == 8) {
                  date = date.substring(0,4) + "-" + date.substring(4,6) + "-" + date.substring(6,8);
                }

                // Format time HHMM to HH:MM
                String time = qso["time_on"] | "";
                if (time.length() >= 4) {
                  time = time.substring(0,2) + ":" + time.substring(2,4);
                }

                csv += date + ",";
                csv += time + ",";
                csv += String(qso["callsign"] | "") + ",";
                csv += String(qso["frequency"] | 0.0f, 3) + ",";
                csv += String(qso["band"] | "") + ",";
                csv += String(qso["mode"] | "") + ",";
                csv += String(qso["rst_sent"] | "") + ",";
                csv += String(qso["rst_rcvd"] | "") + ",";
                csv += String(qso["my_gridsquare"] | "") + ",";
                csv += String(qso["my_pota_ref"] | "") + ",";
                csv += String(qso["gridsquare"] | "") + ",";
                csv += String(qso["their_pota_ref"] | "") + ",";

                // Escape notes field (may contain commas)
                String notes = qso["notes"] | "";
                if (notes.indexOf(',') >= 0 || notes.indexOf('"') >= 0) {
                  notes.replace("\"", "\"\"");
                  csv += "\"" + notes + "\"";
                } else {
                  csv += notes;
                }
                csv += "\n";
              }
            }
          }
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }

  return csv;
}

#endif // WEB_SERVER_H
