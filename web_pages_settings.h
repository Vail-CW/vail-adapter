/*
 * Web Page - Device Settings
 * HTML/CSS/JavaScript for device settings page
 * Includes CW, audio, and station settings
 */

#ifndef WEB_PAGES_SETTINGS_H
#define WEB_PAGES_SETTINGS_H

const char SETTINGS_HTML[] PROGMEM = R"rawliteral(
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

#endif // WEB_PAGES_SETTINGS_H
