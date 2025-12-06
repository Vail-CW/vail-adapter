/*
 * Web Pages - Setup Splash
 * Minimal page for first-time setup when web files are not on SD card
 * This page is stored in flash (~5KB) and serves as the fallback
 */

#ifndef WEB_PAGES_SETUP_H
#define WEB_PAGES_SETUP_H

const char SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>VAIL SUMMIT - Setup</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #fff;
            padding: 20px;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .container { max-width: 600px; width: 100%; }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 20px;
            padding: 40px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
            text-align: center;
        }
        h1 { font-size: 2rem; margin-bottom: 10px; }
        .subtitle { font-size: 1.1rem; opacity: 0.9; margin-bottom: 30px; }
        .icon { font-size: 4rem; margin-bottom: 20px; }
        .message {
            background: rgba(255,255,255,0.1);
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 30px;
            line-height: 1.6;
        }
        .message h3 { margin-bottom: 10px; color: #00d4ff; }
        .btn {
            display: inline-block;
            padding: 15px 30px;
            background: #00d4ff;
            color: #1e3c72;
            text-decoration: none;
            border-radius: 10px;
            border: none;
            font-size: 1.1rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
            margin: 10px;
            min-width: 200px;
        }
        .btn:hover { background: #00b8e6; transform: translateY(-2px); }
        .btn:disabled { opacity: 0.5; cursor: not-allowed; transform: none; }
        .btn-secondary {
            background: rgba(255,255,255,0.2);
            color: #fff;
            border: 1px solid rgba(255,255,255,0.3);
        }
        .btn-secondary:hover { background: rgba(255,255,255,0.3); }
        .progress-container {
            display: none;
            margin-top: 30px;
        }
        .progress-bar {
            background: rgba(255,255,255,0.2);
            border-radius: 10px;
            height: 20px;
            overflow: hidden;
            margin-bottom: 15px;
        }
        .progress-fill {
            background: linear-gradient(90deg, #00d4ff, #00ff88);
            height: 100%;
            width: 0%;
            transition: width 0.3s;
            border-radius: 10px;
        }
        .progress-text { font-size: 0.9rem; opacity: 0.9; }
        .status { margin-top: 20px; padding: 15px; border-radius: 10px; display: none; }
        .status.error { background: rgba(255,0,0,0.2); border: 1px solid rgba(255,0,0,0.4); }
        .status.success { background: rgba(0,255,0,0.2); border: 1px solid rgba(0,255,0,0.4); }
        .divider {
            display: flex;
            align-items: center;
            margin: 25px 0;
            opacity: 0.6;
        }
        .divider::before, .divider::after {
            content: '';
            flex: 1;
            height: 1px;
            background: rgba(255,255,255,0.3);
        }
        .divider span { padding: 0 15px; font-size: 0.9rem; }
        .upload-form { margin-top: 20px; }
        .file-input { display: none; }
        .requirements {
            margin-top: 30px;
            padding: 20px;
            background: rgba(255,200,0,0.1);
            border: 1px solid rgba(255,200,0,0.3);
            border-radius: 10px;
            text-align: left;
        }
        .requirements h4 { color: #ffd700; margin-bottom: 10px; }
        .requirements ul { margin-left: 20px; line-height: 1.8; }
        .upload-options { margin-top: 15px; }
        .upload-options label {
            display: block;
            margin: 10px 0;
            padding: 15px;
            background: rgba(255,255,255,0.1);
            border-radius: 10px;
            cursor: pointer;
            transition: all 0.2s;
        }
        .upload-options label:hover { background: rgba(255,255,255,0.2); }
        .upload-options input[type="file"] { display: none; }
        .upload-hint { font-size: 0.85rem; opacity: 0.7; margin-top: 5px; }
        .wifi-status {
            display: inline-block;
            padding: 5px 15px;
            border-radius: 20px;
            font-size: 0.9rem;
            margin-bottom: 20px;
        }
        .wifi-connected { background: rgba(0,255,0,0.2); }
        .wifi-disconnected { background: rgba(255,0,0,0.2); }
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
            <div class="icon">📡</div>
            <h1>VAIL SUMMIT</h1>
            <p class="subtitle">Web Interface Setup</p>

            <div class="wifi-status" id="wifiStatus">Checking WiFi...</div>

            <div class="message">
                <h3>Web Interface Files Required</h3>
                <p>The web interface files need to be installed on your SD card. Choose one of the options below to get started.</p>
            </div>

            <div id="mainButtons">
                <button class="btn" id="downloadBtn" onclick="startDownload()">
                    Download from GitHub
                </button>

                <div class="divider"><span>OR UPLOAD MANUALLY</span></div>

                <div class="upload-options">
                    <label>
                        <input type="file" id="zipInput" accept=".zip" onchange="uploadZip(this.files[0])">
                        <strong>📦 Upload ZIP Archive</strong>
                        <div class="upload-hint">Upload a single .zip file containing all web interface files (recommended)</div>
                    </label>
                    <label>
                        <input type="file" id="fileInput" multiple accept=".html,.css,.js,.json,.txt,.ico,.png,.jpg,.svg" onchange="uploadFiles(this.files)">
                        <strong>📄 Upload Individual Files</strong>
                        <div class="upload-hint">Select multiple files at once (Ctrl+Click or Shift+Click)</div>
                    </label>
                </div>
            </div>

            <div class="progress-container" id="progressContainer">
                <div class="progress-bar">
                    <div class="progress-fill" id="progressFill"></div>
                </div>
                <p class="progress-text" id="progressText">Preparing download...</p>
                <button class="btn btn-secondary" onclick="cancelDownload()" id="cancelBtn">Cancel</button>
            </div>

            <div class="status" id="statusBox"></div>

            <div class="requirements">
                <h4>Requirements</h4>
                <ul>
                    <li>FAT32 formatted SD card (4-32 GB recommended)</li>
                    <li>WiFi connection (for GitHub download or ZIP library)</li>
                    <li>Web files will be stored in /www/ folder on SD card</li>
                    <li>ZIP upload: extracts files in browser, uploads to device</li>
                    <li>Multi-file upload: select all files at once (Ctrl+A in file dialog)</li>
                </ul>
            </div>
        </div>
    </div>

    <script>
        let downloadInterval = null;

        // Check WiFi status on load
        async function checkWiFi() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                const wifiStatus = document.getElementById('wifiStatus');

                if (data.wifiConnected) {
                    wifiStatus.textContent = 'WiFi Connected';
                    wifiStatus.className = 'wifi-status wifi-connected';
                    document.getElementById('downloadBtn').disabled = false;
                } else {
                    wifiStatus.textContent = 'WiFi Not Connected';
                    wifiStatus.className = 'wifi-status wifi-disconnected';
                    document.getElementById('downloadBtn').disabled = true;
                }
            } catch (e) {
                document.getElementById('wifiStatus').textContent = 'Status Unknown';
            }
        }

        async function startDownload() {
            document.getElementById('mainButtons').style.display = 'none';
            document.getElementById('progressContainer').style.display = 'block';
            document.getElementById('statusBox').style.display = 'none';

            try {
                // Start download
                const response = await fetch('/api/webfiles/download', { method: 'POST' });

                if (!response.ok) {
                    throw new Error('Failed to start download');
                }

                // Poll for progress
                downloadInterval = setInterval(updateProgress, 500);
            } catch (error) {
                showError('Failed to start download: ' + error.message);
            }
        }

        async function updateProgress() {
            try {
                const response = await fetch('/api/webfiles/progress');
                const data = await response.json();

                const progressFill = document.getElementById('progressFill');
                const progressText = document.getElementById('progressText');

                progressFill.style.width = data.overallProgress + '%';

                if (data.state === 1) { // FETCHING_MANIFEST
                    progressText.textContent = 'Fetching file list...';
                } else if (data.state === 2) { // IN_PROGRESS
                    progressText.textContent = `Downloading ${data.currentFile}/${data.totalFiles}: ${data.currentFileName}`;
                } else if (data.state === 3) { // COMPLETE
                    clearInterval(downloadInterval);
                    progressFill.style.width = '100%';
                    progressText.textContent = 'Restarting server...';

                    // Request server restart to switch from setup mode to normal mode
                    try {
                        await fetch('/api/webfiles/restart', { method: 'POST' });
                    } catch (e) {
                        // Server might disconnect during restart, that's ok
                    }

                    showSuccess('Download complete! Redirecting...');
                    // Give server time to restart before redirecting
                    setTimeout(() => window.location.href = '/', 3000);
                } else if (data.state === 4) { // ERROR
                    clearInterval(downloadInterval);
                    showError(data.error || 'Download failed');
                }
            } catch (error) {
                clearInterval(downloadInterval);
                showError('Lost connection to device');
            }
        }

        async function cancelDownload() {
            clearInterval(downloadInterval);
            await fetch('/api/webfiles/cancel', { method: 'POST' });
            document.getElementById('mainButtons').style.display = 'block';
            document.getElementById('progressContainer').style.display = 'none';
        }

        async function uploadFiles(files) {
            if (!files || files.length === 0) return;

            document.getElementById('mainButtons').style.display = 'none';
            document.getElementById('progressContainer').style.display = 'block';

            const progressFill = document.getElementById('progressFill');
            const progressText = document.getElementById('progressText');
            document.getElementById('cancelBtn').style.display = 'none';

            let uploaded = 0;
            const total = files.length;

            for (const file of files) {
                progressText.textContent = `Uploading ${uploaded + 1}/${total}: ${file.name}`;
                progressFill.style.width = ((uploaded / total) * 100) + '%';

                const formData = new FormData();
                formData.append('file', file);
                formData.append('path', '/www/' + file.name);

                try {
                    const response = await fetch('/api/webfiles/upload', {
                        method: 'POST',
                        body: formData
                    });
                    if (!response.ok) throw new Error('Upload failed');
                    uploaded++;
                } catch (error) {
                    showError('Failed to upload ' + file.name);
                    return;
                }
            }

            progressFill.style.width = '100%';
            progressText.textContent = 'Restarting server...';

            // Request server restart to switch from setup mode to normal mode
            try {
                await fetch('/api/webfiles/restart', { method: 'POST' });
            } catch (e) {
                // Server might disconnect during restart, that's ok
            }

            showSuccess('Upload complete! Redirecting...');
            // Give server time to restart before redirecting
            setTimeout(() => window.location.href = '/', 3000);
        }

        async function uploadZip(file) {
            if (!file) return;

            document.getElementById('mainButtons').style.display = 'none';
            document.getElementById('progressContainer').style.display = 'block';

            const progressFill = document.getElementById('progressFill');
            const progressText = document.getElementById('progressText');
            document.getElementById('cancelBtn').style.display = 'none';

            progressText.textContent = 'Reading ZIP file...';
            progressFill.style.width = '10%';

            try {
                // Load JSZip library dynamically (from CDN)
                if (typeof JSZip === 'undefined') {
                    progressText.textContent = 'Loading ZIP library...';
                    await loadScript('https://cdnjs.cloudflare.com/ajax/libs/jszip/3.10.1/jszip.min.js');
                }

                progressText.textContent = 'Extracting ZIP contents...';
                progressFill.style.width = '20%';

                const zip = await JSZip.loadAsync(file);
                const files = [];

                // Collect all files from ZIP
                zip.forEach((relativePath, zipEntry) => {
                    if (!zipEntry.dir) {
                        // Handle files in subdirectories (e.g., www/index.html -> index.html)
                        let fileName = relativePath;
                        // Remove common root folders like 'www/' or 'web/www/'
                        if (fileName.startsWith('www/')) {
                            fileName = fileName.substring(4);
                        } else if (fileName.startsWith('web/www/')) {
                            fileName = fileName.substring(8);
                        }
                        // Only include web files, skip hidden and system files
                        if (fileName && !fileName.startsWith('.') && !fileName.startsWith('__')) {
                            files.push({ path: relativePath, name: fileName, entry: zipEntry });
                        }
                    }
                });

                if (files.length === 0) {
                    throw new Error('No valid files found in ZIP');
                }

                progressText.textContent = `Found ${files.length} files. Uploading...`;
                progressFill.style.width = '30%';

                let uploaded = 0;
                for (const fileInfo of files) {
                    progressText.textContent = `Uploading ${uploaded + 1}/${files.length}: ${fileInfo.name}`;
                    progressFill.style.width = (30 + (uploaded / files.length) * 70) + '%';

                    // Extract file content
                    const content = await fileInfo.entry.async('blob');
                    const formData = new FormData();
                    formData.append('file', content, fileInfo.name);
                    formData.append('path', '/www/' + fileInfo.name);

                    const response = await fetch('/api/webfiles/upload', {
                        method: 'POST',
                        body: formData
                    });

                    if (!response.ok) throw new Error('Upload failed for ' + fileInfo.name);
                    uploaded++;
                }

                progressFill.style.width = '100%';
                progressText.textContent = 'Restarting server...';

                // Request server restart to switch from setup mode to normal mode
                try {
                    await fetch('/api/webfiles/restart', { method: 'POST' });
                } catch (e) {
                    // Server might disconnect during restart, that's ok
                }

                showSuccess(`Uploaded ${files.length} files! Redirecting...`);
                // Give server time to restart before redirecting
                setTimeout(() => window.location.href = '/', 3000);

            } catch (error) {
                showError('Failed to process ZIP: ' + error.message);
            }
        }

        // Helper to load external scripts
        function loadScript(src) {
            return new Promise((resolve, reject) => {
                const script = document.createElement('script');
                script.src = src;
                script.onload = resolve;
                script.onerror = () => reject(new Error('Failed to load ' + src));
                document.head.appendChild(script);
            });
        }

        function showError(message) {
            const statusBox = document.getElementById('statusBox');
            statusBox.className = 'status error';
            statusBox.textContent = message;
            statusBox.style.display = 'block';
            document.getElementById('mainButtons').style.display = 'block';
            document.getElementById('progressContainer').style.display = 'none';
        }

        function showSuccess(message) {
            const statusBox = document.getElementById('statusBox');
            statusBox.className = 'status success';
            statusBox.textContent = message;
            statusBox.style.display = 'block';
        }

        // Initialize
        checkWiFi();
        setInterval(checkWiFi, 10000);
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGES_SETUP_H
