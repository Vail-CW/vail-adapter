/*
 * Web Pages - WiFi Setup
 * Network scanning and connection configuration page
 */

#ifndef WEB_PAGES_WIFI_H
#define WEB_PAGES_WIFI_H

const char WIFI_SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi Setup - VAIL SUMMIT</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #fff;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 900px; margin: 0 auto; }
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
            background: rgba(255,255,255,0.95);
            color: #333;
            border-radius: 12px;
            padding: 25px;
            margin-bottom: 20px;
            box-shadow: 0 8px 20px rgba(0,0,0,0.3);
        }
        .card h2 { color: #1e3c72; margin-bottom: 15px; font-size: 1.5rem; }
        .warning-box {
            background: #fff3cd;
            border: 2px solid #ffc107;
            border-radius: 8px;
            padding: 15px;
            margin-bottom: 20px;
        }
        .warning-box strong { color: #856404; }
        .network-list { list-style: none; margin-top: 15px; }
        .network-item {
            background: #f8f9fa;
            border: 1px solid #dee2e6;
            border-radius: 8px;
            padding: 15px;
            margin-bottom: 10px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .network-info { flex: 1; }
        .network-name { font-size: 1.1rem; font-weight: bold; color: #1e3c72; }
        .network-signal { font-size: 0.9rem; color: #666; }
        .signal-strong { color: #28a745; }
        .signal-medium { color: #ffc107; }
        .signal-weak { color: #dc3545; }
        .btn {
            background: #1e3c72;
            color: #fff;
            border: none;
            padding: 8px 16px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 0.9rem;
        }
        .btn:hover { background: #2a5298; }
        .btn:disabled { background: #ccc; cursor: not-allowed; }
        .btn-scan { width: 100%; padding: 12px; font-size: 1rem; margin-bottom: 15px; }
        .input-group { margin-bottom: 15px; }
        .input-group label { display: block; margin-bottom: 5px; font-weight: bold; }
        .input-group input {
            width: 100%;
            padding: 10px;
            border: 1px solid #ccc;
            border-radius: 6px;
            font-size: 1rem;
        }
        .message {
            padding: 12px;
            border-radius: 6px;
            margin-bottom: 15px;
            display: none;
        }
        .message.success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .message.error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .message.info { background: #d1ecf1; color: #0c5460; border: 1px solid #bee5eb; }
        #connectModal {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0,0,0,0.7);
            z-index: 1000;
            justify-content: center;
            align-items: center;
        }
        .modal-content {
            background: #fff;
            border-radius: 12px;
            padding: 25px;
            max-width: 500px;
            width: 90%;
        }
        .modal-content h3 { color: #1e3c72; margin-bottom: 15px; }
        .btn-group { display: flex; gap: 10px; margin-top: 15px; }
        .btn-primary { background: #007bff; }
        .btn-primary:hover { background: #0056b3; }
        .btn-secondary { background: #6c757d; }
        .btn-secondary:hover { background: #5a6268; }
        .back-link { display: inline-block; margin-top: 20px; color: #fff; text-decoration: none; }
        .back-link:hover { text-decoration: underline; }
        .spinner { display: inline-block; width: 20px; height: 20px; border: 3px solid rgba(255,255,255,0.3); border-radius: 50%; border-top-color: #fff; animation: spin 1s ease-in-out infinite; }
        @keyframes spin { to { transform: rotate(360deg); } }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>📡 WiFi Setup</h1>
            <p>Configure network connectivity</p>
        </header>

        <div id="warningBox" class="card warning-box" style="display:none;">
            <strong>⚠️ Remote Connection Detected</strong>
            <p>You are connected to this device remotely via WiFi. Changing WiFi settings from here could disconnect you and make the device unreachable. Please use the device's physical WiFi settings menu for network changes.</p>
        </div>

        <div id="apModeBox" class="card warning-box" style="display:none;">
            <strong>ℹ️ Access Point Mode</strong>
            <p>The device is in AP mode. You can scan and connect to WiFi networks below.</p>
        </div>

        <div class="card">
            <h2>Available Networks</h2>
            <div id="message" class="message"></div>
            <button id="scanBtn" class="btn btn-scan" onclick="scanNetworks()">
                <span id="scanText">🔍 Scan for Networks</span>
                <span id="scanSpinner" class="spinner" style="display:none;"></span>
            </button>
            <ul id="networkList" class="network-list"></ul>
        </div>

        <a href="/" class="back-link">← Back to Dashboard</a>
    </div>

    <!-- Connection Modal -->
    <div id="connectModal">
        <div class="modal-content">
            <h3>Connect to Network</h3>
            <p><strong>SSID:</strong> <span id="modalSSID"></span></p>
            <div class="input-group">
                <label for="passwordInput">Password:</label>
                <input type="password" id="passwordInput" placeholder="Enter WiFi password">
            </div>
            <div class="btn-group">
                <button class="btn btn-primary" onclick="connectToNetwork()">Connect</button>
                <button class="btn btn-secondary" onclick="closeModal()">Cancel</button>
            </div>
        </div>
    </div>

    <!-- Success Modal (AP Mode) -->
    <div id="successModal" style="display:none;">
        <div class="modal-content" style="max-width: 600px;">
            <h3 style="color: #28a745; margin-bottom: 20px;">✓ Connection Command Sent</h3>
            <div style="background: #fff3cd; border: 2px solid #ffc107; border-radius: 8px; padding: 20px; margin-bottom: 20px;">
                <p style="font-size: 1.1rem; margin-bottom: 15px;"><strong>⚠️ Important: Check Your Device</strong></p>
                <p style="margin-bottom: 10px; color: #856404;">The Summit device is now attempting to connect to <strong id="successSSID"></strong>.</p>
                <p style="margin-bottom: 10px; color: #856404;">If successful, the device will disconnect from AP mode and this page will become unavailable.</p>
                <p style="margin-bottom: 0; color: #856404;"><strong>Please check the Summit's screen to verify the connection status.</strong></p>
            </div>
            <div style="background: #d1ecf1; border: 1px solid #bee5eb; border-radius: 8px; padding: 15px; margin-bottom: 20px;">
                <p style="margin-bottom: 10px; color: #0c5460;"><strong>What to look for on the device:</strong></p>
                <ul style="margin: 0; padding-left: 20px; color: #0c5460;">
                    <li>Check the WiFi icon in the status bar (should show connected)</li>
                    <li>The device may show a "Connected" message in WiFi settings</li>
                    <li>If connection failed, you'll see an error message on the device</li>
                </ul>
            </div>
            <button class="btn btn-primary" onclick="closeSuccessModal()" style="width: 100%;">OK, I'll Check the Device</button>
        </div>
    </div>

    <script>
        let selectedSSID = '';
        let isAPMode = false;
        let isRemoteConnection = false;

        // Check connection status on page load
        window.addEventListener('load', async () => {
            await checkConnectionStatus();
            if (!isRemoteConnection) {
                await scanNetworks();
            }
        });

        async function checkConnectionStatus() {
            try {
                const response = await fetch('/api/wifi/status');
                const data = await response.json();
                isAPMode = data.isAPMode;
                isRemoteConnection = data.isRemoteConnection;

                if (isRemoteConnection) {
                    document.getElementById('warningBox').style.display = 'block';
                    document.getElementById('scanBtn').disabled = true;
                    document.getElementById('scanBtn').textContent = '🔒 WiFi Changes Disabled (Remote Connection)';
                } else if (isAPMode) {
                    document.getElementById('apModeBox').style.display = 'block';
                }
            } catch (error) {
                console.error('Error checking connection status:', error);
            }
        }

        async function scanNetworks() {
            if (isRemoteConnection) {
                showMessage('WiFi changes are disabled when connected remotely.', 'error');
                return;
            }

            const scanBtn = document.getElementById('scanBtn');
            const scanText = document.getElementById('scanText');
            const scanSpinner = document.getElementById('scanSpinner');
            const networkList = document.getElementById('networkList');

            scanBtn.disabled = true;
            scanText.style.display = 'none';
            scanSpinner.style.display = 'inline-block';
            networkList.innerHTML = '<li style="padding: 20px; text-align: center; color: #666;">Scanning...</li>';

            try {
                const response = await fetch('/api/wifi/scan');
                const data = await response.json();

                if (data.success) {
                    displayNetworks(data.networks);
                    showMessage(`Found ${data.networks.length} network(s)`, 'success');
                } else {
                    showMessage('Scan failed: ' + data.error, 'error');
                    networkList.innerHTML = '';
                }
            } catch (error) {
                showMessage('Error: ' + error.message, 'error');
                networkList.innerHTML = '';
            } finally {
                scanBtn.disabled = false;
                scanText.style.display = 'inline';
                scanSpinner.style.display = 'none';
            }
        }

        function displayNetworks(networks) {
            const networkList = document.getElementById('networkList');
            networkList.innerHTML = '';

            if (networks.length === 0) {
                networkList.innerHTML = '<li style="padding: 20px; text-align: center; color: #666;">No networks found</li>';
                return;
            }

            networks.forEach(network => {
                const li = document.createElement('li');
                li.className = 'network-item';

                const signalClass = network.rssi > -60 ? 'signal-strong' : network.rssi > -70 ? 'signal-medium' : 'signal-weak';
                const lockIcon = network.encrypted ? '🔒' : '📡';

                // Create info div
                const infoDiv = document.createElement('div');
                infoDiv.className = 'network-info';
                infoDiv.innerHTML = `
                    <div class="network-name">${lockIcon} ${escapeHtml(network.ssid)}</div>
                    <div class="network-signal ${signalClass}">Signal: ${network.rssi} dBm</div>
                `;

                // Create button with proper event listener (avoids string injection issues)
                const btn = document.createElement('button');
                btn.className = 'btn';
                btn.textContent = 'Connect';
                btn.onclick = () => showConnectModal(network.ssid, network.encrypted);

                li.appendChild(infoDiv);
                li.appendChild(btn);
                networkList.appendChild(li);
            });
        }

        // Helper function to escape HTML
        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }

        function showConnectModal(ssid, encrypted) {
            selectedSSID = ssid;
            document.getElementById('modalSSID').textContent = ssid;
            document.getElementById('passwordInput').value = '';
            document.getElementById('connectModal').style.display = 'flex';

            // If open network, auto-fill empty password
            if (!encrypted) {
                document.getElementById('passwordInput').placeholder = 'Open network (no password required)';
            } else {
                document.getElementById('passwordInput').placeholder = 'Enter WiFi password';
            }
        }

        function closeModal() {
            document.getElementById('connectModal').style.display = 'none';
            selectedSSID = '';
        }

        async function connectToNetwork() {
            const password = document.getElementById('passwordInput').value;

            // Validate SSID is not empty
            if (!selectedSSID || selectedSSID.trim().length === 0) {
                showMessage('Error: No network selected', 'error');
                console.error('Empty SSID:', selectedSSID);
                return;
            }

            // IMPORTANT: Save SSID to local variable BEFORE closing modal
            // (closeModal() clears selectedSSID)
            const ssid = selectedSSID;

            console.log('Attempting to connect to:', ssid);
            console.log('Password length:', password.length);

            closeModal();
            showMessage('Connecting to ' + ssid + '...', 'info');

            try {
                const payload = { ssid: ssid, password: password };
                console.log('Sending payload:', JSON.stringify(payload));

                const response = await fetch('/api/wifi/connect', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                });

                console.log('Response status:', response.status);
                const data = await response.json();
                console.log('Response data:', data);

                if (data.success) {
                    showMessage('Successfully connected to ' + ssid + '!', 'success');

                    // If in AP mode, show important message about checking device
                    if (isAPMode) {
                        setTimeout(() => {
                            showConnectionSuccessModal(ssid);
                        }, 1000);
                    }
                } else {
                    showMessage('Connection failed: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Connection error:', error);
                showMessage('Error: ' + error.message, 'error');
            }
        }

        function showMessage(text, type) {
            const message = document.getElementById('message');
            message.textContent = text;
            message.className = 'message ' + type;
            message.style.display = 'block';

            if (type === 'success' || type === 'error') {
                setTimeout(() => {
                    message.style.display = 'none';
                }, 5000);
            }
        }

        function showConnectionSuccessModal(ssid) {
            document.getElementById('successSSID').textContent = ssid;
            document.getElementById('successModal').style.display = 'flex';
        }

        function closeSuccessModal() {
            document.getElementById('successModal').style.display = 'none';
        }
    </script>
</body>
</html>
)rawliteral";

void serveWiFiSetup(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", WIFI_SETUP_HTML);
}

#endif // WEB_PAGES_WIFI_H
