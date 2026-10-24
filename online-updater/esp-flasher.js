// ESP32 Web Flasher for Vail Summit
// Uses esptool-js for web-based ESP32 flashing

// CORS proxy for GitHub release asset downloads
// Deploy the Cloudflare Worker in tools/firmware-proxy/ and set this URL
const FIRMWARE_PROXY_URL = 'https://vail-firmware-proxy.brett-hollifield.workers.dev';

// The Summit is built with USBMode=hwcdc, so it talks over the ESP32-S3's
// built-in USB-Serial-JTAG port: Espressif 303A:1001, the same device whether
// the firmware is running or the chip is sitting in its ROM bootloader. That
// port ignores the old 1200 baud trick, and esptool-js already knows how to
// reset it into the bootloader (it checks for PID 0x1001 itself), so for
// current firmware all we do is pick the port and hand it over.
//
// Adafruit 239A shows up for a factory-fresh Feather or old firmware that ran
// its own USB stack. Those do need the 1200 baud restart, and come back as a
// different USB device, which the browser makes the user pick again.
const ESPRESSIF_VID = 0x303A;
const ADAFRUIT_VID = 0x239A;
const USB_JTAG_SERIAL_PID = 0x1001;
const SUMMIT_PORT_FILTERS = [{ usbVendorId: ESPRESSIF_VID }, { usbVendorId: ADAFRUIT_VID }];

function summitPortKind(port) {
    try {
        const info = port.getInfo();
        if (info.usbVendorId === ESPRESSIF_VID) {
            return info.usbProductId === USB_JTAG_SERIAL_PID ? 'jtag' : 'rom';
        }
        if (info.usbVendorId === ADAFRUIT_VID) return 'tinyusb';
    } catch (_) {}
    return 'unknown';
}

function summitPortLabel(port) {
    try {
        const i = port.getInfo();
        if (i && i.usbVendorId) {
            return `USB ${i.usbVendorId.toString(16).padStart(4, '0')}:${(i.usbProductId || 0).toString(16).padStart(4, '0')}`;
        }
    } catch (_) {}
    return 'serial device';
}

const sleep = (ms) => new Promise(resolve => setTimeout(resolve, ms));

class ESP32Flasher {
    constructor() {
        this.port = null;
        this.device = null;
        this.transport = null;
        this.esploader = null;
        this.connected = false;
        this.chipName = null;
        this.esptoolModule = null;
        this.state = 'idle';

        // Releases data
        this.releases = [];        // Stable releases (>= v0.51)
        this.testRelease = null;   // Most recent pre-release
        this.selectedRelease = null;

        // Firmware files will be set when a release is selected
        this.firmwareFiles = [];

        // Fetch releases on init
        this.fetchReleases();
    }

    // Parse version string like "v0.51" or "0.51" into a comparable number
    parseVersion(tagName) {
        const match = tagName.match(/v?(\d+)\.(\d+)/);
        if (!match) return 0;
        return parseFloat(`${match[1]}.${match[2]}`);
    }

    // Fetch available releases from GitHub
    async fetchReleases() {
        try {
            const response = await fetch('https://api.github.com/repos/Vail-CW/vail-summit/releases?per_page=50');
            if (!response.ok) {
                console.log('Could not fetch releases');
                this.setFallbackFirmware();
                return;
            }
            const allReleases = await response.json();

            // Separate stable and pre-releases, filtering to >= v0.51
            const minVersion = 0.51;

            this.releases = allReleases.filter(r =>
                !r.prerelease &&
                !r.draft &&
                this.parseVersion(r.tag_name) >= minVersion &&
                r.assets && r.assets.length >= 3
            );

            // Find the most recent pre-release (test release)
            const preReleases = allReleases.filter(r =>
                r.prerelease &&
                !r.draft &&
                r.assets && r.assets.length >= 3
            );
            this.testRelease = preReleases.length > 0 ? preReleases[0] : null;

            this.populateVersionSelector();

            // Auto-select the latest stable release
            if (this.releases.length > 0) {
                this.selectRelease(this.releases[0]);
            } else {
                this.setFallbackFirmware();
            }
        } catch (err) {
            console.log('Error fetching releases:', err.message);
            this.setFallbackFirmware();
        }
    }

    // Fallback to raw GitHub URLs if releases API fails
    setFallbackFirmware() {
        const base = 'https://raw.githubusercontent.com/Vail-CW/vail-summit/main/firmware_files/';
        this.firmwareFiles = [
            { address: 0x0, file: base + 'bootloader.bin' },
            { address: 0x8000, file: base + 'partitions.bin' },
            { address: 0x10000, file: base + 'vail-summit.bin' }
        ];

        const select = document.getElementById('versionSelect');
        if (select) {
            select.innerHTML = '<option value="fallback">Latest (from repository)</option>';
            select.disabled = false;
        }
    }

    // Populate the version dropdown
    populateVersionSelector() {
        const select = document.getElementById('versionSelect');
        if (!select) return;

        select.innerHTML = '';

        // Add stable releases
        this.releases.forEach((release, index) => {
            const option = document.createElement('option');
            option.value = release.tag_name;
            const date = new Date(release.published_at).toLocaleDateString('en-US', {
                year: 'numeric', month: 'short', day: 'numeric'
            });
            const label = release.name || release.tag_name;
            option.textContent = `${label} (${date})${index === 0 ? ' — Latest' : ''}`;
            select.appendChild(option);
        });

        select.disabled = false;

        // Listen for changes
        select.addEventListener('change', () => {
            const tag = select.value;
            if (tag === '__test__') {
                this.selectRelease(this.testRelease);
            } else {
                const release = this.releases.find(r => r.tag_name === tag);
                if (release) this.selectRelease(release);
            }
        });

        // Set up test release checkbox
        const checkbox = document.getElementById('showTestRelease');
        if (checkbox) {
            // Hide checkbox entirely if no test release exists
            const toggleContainer = checkbox.closest('.test-release-toggle');
            if (!this.testRelease && toggleContainer) {
                toggleContainer.style.display = 'none';
            }

            checkbox.addEventListener('change', () => {
                this.toggleTestRelease(checkbox.checked);
            });
        }
    }

    // Toggle test release visibility in the dropdown
    toggleTestRelease(show) {
        const select = document.getElementById('versionSelect');
        const warning = document.getElementById('testReleaseWarning');
        if (!select) return;

        // Remove existing test release option
        const existing = select.querySelector('option[value="__test__"]');
        if (existing) existing.remove();

        if (show && this.testRelease) {
            // Add test release at the top
            const option = document.createElement('option');
            option.value = '__test__';
            const date = new Date(this.testRelease.published_at).toLocaleDateString('en-US', {
                year: 'numeric', month: 'short', day: 'numeric'
            });
            const label = this.testRelease.name || this.testRelease.tag_name;
            option.textContent = `${label} (${date}) — Test Release`;
            option.className = 'test-release-option';
            select.insertBefore(option, select.firstChild);
            select.value = '__test__';
            this.selectRelease(this.testRelease);

            if (warning) warning.style.display = 'block';
        } else {
            // Revert to latest stable
            if (this.releases.length > 0) {
                select.value = this.releases[0].tag_name;
                this.selectRelease(this.releases[0]);
            }
            if (warning) warning.style.display = 'none';
        }
    }

    // Select a specific release and update firmware URLs + UI
    selectRelease(release) {
        this.selectedRelease = release;

        // Build firmware URLs via CORS proxy pointed at GitHub release assets
        const base = `${FIRMWARE_PROXY_URL}/${release.tag_name}/`;
        this.firmwareFiles = [
            { address: 0x0, file: base + 'bootloader.bin', name: 'bootloader.bin' },
            { address: 0x8000, file: base + 'partitions.bin', name: 'partitions.bin' },
            { address: 0x10000, file: base + 'vail-summit.bin', name: 'vail-summit.bin' }
        ];

        // Update release info display
        this.updateReleaseInfo(release);
    }

    // Update the release notes and date display
    updateReleaseInfo(release) {
        const infoContainer = document.getElementById('releaseInfo');
        const versionBadge = document.getElementById('releaseVersionBadge');
        const dateDisplay = document.getElementById('releaseDateDisplay');
        const notesContainer = document.getElementById('releaseNotes');

        if (!infoContainer) return;

        infoContainer.style.display = 'block';

        if (versionBadge) {
            versionBadge.textContent = release.tag_name;
            // Color the badge differently for test releases
            if (release.prerelease) {
                versionBadge.className = 'firmware-version test-version';
            } else {
                versionBadge.className = 'firmware-version';
            }
        }

        if (dateDisplay) {
            const date = new Date(release.published_at);
            dateDisplay.textContent = 'Released ' + date.toLocaleDateString('en-US', {
                year: 'numeric', month: 'long', day: 'numeric'
            });
        }

        if (notesContainer) {
            if (release.body) {
                // Convert markdown-like text to simple HTML
                notesContainer.innerHTML = this.renderReleaseNotes(release.body);
            } else {
                notesContainer.innerHTML = '<p class="no-notes">No release notes available.</p>';
            }
        }
    }

    // Simple markdown-to-HTML renderer for release notes
    renderReleaseNotes(markdown) {
        let html = markdown
            // Escape HTML
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            // Headers
            .replace(/^### (.+)$/gm, '<h4>$1</h4>')
            .replace(/^## (.+)$/gm, '<h3>$1</h3>')
            // Bold
            .replace(/\*\*(.+?)\*\*/g, '<strong>$1</strong>')
            // Italic
            .replace(/\*(.+?)\*/g, '<em>$1</em>')
            // Inline code
            .replace(/`(.+?)`/g, '<code>$1</code>')
            // Unordered list items
            .replace(/^[-*] (.+)$/gm, '<li>$1</li>')
            // Wrap consecutive <li> in <ul>
            .replace(/((?:<li>.*<\/li>\n?)+)/g, '<ul>$1</ul>')
            // Paragraphs (double newlines)
            .replace(/\n\n/g, '</p><p>')
            // Single newlines within paragraphs
            .replace(/\n/g, '<br>');

        // Wrap in paragraph if not already wrapped
        if (!html.startsWith('<h') && !html.startsWith('<ul') && !html.startsWith('<p')) {
            html = '<p>' + html + '</p>';
        }

        return html;
    }

    log(message) {
        console.log(message);
        const logArea = document.getElementById('espFlashLog');
        if (logArea) {
            const timestamp = new Date().toLocaleTimeString();
            logArea.textContent += `[${timestamp}] ${message}\n`;
            logArea.scrollTop = logArea.scrollHeight;
        }
    }

    updateProgress(percentage, label) {
        const progressBar = document.getElementById('progressBar');
        const progressPercent = document.getElementById('progressPercent');
        const progressLabel = document.getElementById('progressLabel');
        const progressContainer = document.getElementById('progressContainer');

        if (progressContainer) {
            progressContainer.style.display = 'block';
        }
        if (progressBar) {
            progressBar.style.width = `${percentage}%`;
        }
        if (progressPercent) {
            progressPercent.textContent = `${Math.round(percentage)}%`;
        }
        if (progressLabel && label) {
            progressLabel.textContent = label;
        }
    }

    // ---------------------------------------------------------------
    // Screen state. Every button's visibility goes through here, so the
    // page can never show Connect and Flash Firmware at the same time.
    //   idle        nothing connected, Connect visible
    //   connecting  picker open or esptool talking to the chip
    //   reselect    old-style board restarted into install mode, pick again
    //   connected   install options visible
    //   busy        flashing or erasing, everything locked
    // ---------------------------------------------------------------
    setState(state, message, tone) {
        this.state = state;
        const live = state === 'connected' || state === 'busy';

        const show = (id, on) => {
            const el = document.getElementById(id);
            if (el) el.style.display = on ? 'inline-block' : 'none';
        };
        const enable = (id, on) => {
            const el = document.getElementById(id);
            if (el) el.disabled = !on;
        };

        show('enterBootloaderButton', !live);
        ['programButton', 'eraseAndFlashButton', 'eraseButton', 'disconnectButton'].forEach(id => {
            show(id, live);
            enable(id, state === 'connected');
        });

        const hero = document.getElementById('enterBootloaderButton');
        if (hero) {
            hero.disabled = state === 'connecting';
            hero.textContent = state === 'connecting' ? 'Connecting\u2026'
                : state === 'reselect' ? 'Connect again'
                : 'Connect to Summit';
        }
        ['directConnectButton', 'summitManualPortButton'].forEach(id => enable(id, !live && state !== 'connecting'));

        if (message !== undefined) this.hint(message, tone);
    }

    // The one line people actually read. The log stays for troubleshooting.
    hint(text, tone) {
        const el = document.getElementById('espHint');
        if (!el) return;
        el.style.display = text ? '' : 'none';
        el.textContent = text || '';
        el.classList.toggle('ok', tone === 'ok');
        el.classList.toggle('err', tone === 'err');
    }

    // Reuse a Summit this browser already has permission for, otherwise open
    // a picker filtered to Espressif and Adafruit USB devices so Bluetooth COM
    // ports and other boards stay out of the list. showAll skips both.
    async pickPort({ showAll = false } = {}) {
        if (!showAll) {
            const granted = await navigator.serial.getPorts();
            const known = granted.find(p => summitPortKind(p) === 'jtag') ||
                          granted.find(p => summitPortKind(p) === 'rom');
            if (known) {
                this.log(`Reusing remembered Summit (${summitPortLabel(known)})`);
                return known;
            }
        }

        this.hint(showAll
            ? 'Pick your Summit from the full list of serial ports.'
            : 'Pick your Summit in the popup. It shows up as "USB JTAG/serial debug unit".');
        try {
            return showAll
                ? await navigator.serial.requestPort()
                : await navigator.serial.requestPort({ filters: SUMMIT_PORT_FILTERS });
        } catch (err) {
            if (err.name === 'NotFoundError') return null;   // closed the picker
            throw err;
        }
    }

    // Older firmware with its own USB stack only restarts into the bootloader
    // when the port is opened at 1200 baud and closed again.
    async touch1200(port) {
        try {
            if (port.readable || port.writable) await port.close();
            await port.open({ baudRate: 1200 });
            await sleep(100);
            await port.close();
            this.log('Sent 1200 baud restart');
        } catch (e) {
            this.log(`1200 baud restart note: ${e.message}`);
        }
    }

    friendlyConnectError(err) {
        const msg = (err && err.message) || '';
        if (/open|in use|access|busy/i.test(msg) || (err && err.name === 'NetworkError')) {
            return 'The port is in use by another program. Close Arduino IDE or any serial monitor, then click Connect again.';
        }
        if (/timed out|no serial data|failed to connect|wrong boot mode/i.test(msg)) {
            return 'The Summit did not answer. Hold BOOT, tap RESET, let go of BOOT, then click Connect again.';
        }
        return `Could not connect: ${msg}. Unplug the Summit, plug it back in, and click Connect again.`;
    }

    // Main entry point for every connect button on the page.
    async connectToSummit({ showAll = false } = {}) {
        if (!("serial" in navigator)) {
            this.setState('idle', 'This browser cannot talk to USB devices. Use Chrome, Edge, or Opera on a computer.', 'err');
            return false;
        }
        if (this.state === 'connecting' || this.state === 'busy') return false;

        this.setState('connecting', 'Looking for your Summit\u2026');
        try {
            if (this.transport) await this.disconnect({ quiet: true });

            const port = await this.pickPort({ showAll });
            if (!port) {
                this.setState('idle', 'No Summit picked. If it was not in the list, check the cable (some only charge), or use "Pick port manually" under Advanced options.');
                return false;
            }

            const kind = summitPortKind(port);
            this.log(`Port selected (${summitPortLabel(port)}, ${kind})`);

            if (kind === 'tinyusb') {
                await this.touch1200(port);
                this.setState('reselect',
                    'Your Summit is restarting into install mode. Click Connect again and pick "USB JTAG/serial debug unit".');
                return false;
            }

            this.hint('Connecting to your Summit\u2026');
            await this.connect(port);

            const v = this.selectedRelease
                ? (this.selectedRelease.name || this.selectedRelease.tag_name)
                : 'the selected version';
            this.setState('connected', `Connected. Click Flash Firmware to install ${v}.`, 'ok');
            return true;
        } catch (err) {
            this.log(`Connection error: ${err.message}`);
            await this.disconnect({ quiet: true });
            this.setState('idle', this.friendlyConnectError(err), 'err');
            return false;
        }
    }

    // Open the port and get esptool talking to the chip. esptool resets the
    // board into its bootloader on its own, so there is no separate step.
    async connect(port) {
        if (!this.esptoolModule) {
            this.log("Loading esptool-js library...");
            this.esptoolModule = await import('https://unpkg.com/esptool-js@0.4.1/bundle.js');
        }

        this.device = port;
        if (this.device.readable || this.device.writable) {
            try { await this.device.close(); await sleep(500); } catch (e) { this.log(`Note: ${e.message}`); }
        }

        this.log("Opening port at 115200 baud...");
        this.transport = new this.esptoolModule.Transport(this.device, true);
        this.esploader = new this.esptoolModule.ESPLoader({
            transport: this.transport,
            baudrate: 115200,
            romBaudrate: 115200,
            terminal: {
                clean: () => {},
                writeLine: (data) => this.log(data),
                write: (data) => this.log(data)
            },
            enableTracing: false
        });

        this.log("Connecting to the chip...");
        this.chipName = await this.esploader.main();
        this.log(`Connected to ${this.chipName}`);
        this.connected = true;

        if (this.selectedRelease) {
            const label = this.selectedRelease.name || this.selectedRelease.tag_name;
            this.log(`Selected firmware: ${label}${this.selectedRelease.prerelease ? ' (Test Release)' : ''}`);
        }
    }

    // After a successful write the chip has reset into the new firmware, so
    // the bootloader session is gone. Drop it and say what happens next.
    async finishAfterFlash() {
        await this.disconnect({ quiet: true });
        this.setState('idle',
            'Done. Your Summit is restarting with the new firmware. If the screen stays dark, unplug it and plug it back in.',
            'ok');
    }

    async loadFirmwareFile(url, displayName) {
        const label = displayName || url.split('/').pop();
        this.log(`Downloading ${label}...`);
        try {
            const response = await fetch(url);
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            // Get as ArrayBuffer, then convert to binary string properly
            const arrayBuffer = await response.arrayBuffer();
            const bytes = new Uint8Array(arrayBuffer);

            // Convert to binary string (each byte as a character)
            let binaryString = '';
            for (let i = 0; i < bytes.length; i++) {
                binaryString += String.fromCharCode(bytes[i]);
            }

            this.log(`Downloaded ${label} (${binaryString.length} bytes)`);
            return binaryString;
        } catch (err) {
            this.log(`Error downloading ${label}: ${err.message}`);
            throw err;
        }
    }

    async flashFirmware() {
        if (!this.connected) {
            this.log("Error: Not connected to device");
            return;
        }

        try {
            this.log("Starting firmware flash process...");
            this.updateProgress(0, "Preparing to flash...");

            // Load all firmware files
            const fileArray = [];
            for (let i = 0; i < this.firmwareFiles.length; i++) {
                const fw = this.firmwareFiles[i];
                const label = fw.name || fw.file.split('/').pop();
                this.updateProgress((i / this.firmwareFiles.length) * 20, `Loading ${label}...`);

                const data = await this.loadFirmwareFile(fw.file, fw.name);

                this.log(`Prepared ${label}: ${data.length} bytes at 0x${fw.address.toString(16)}`);

                fileArray.push({
                    data: data,
                    address: fw.address
                });
            }

            this.log("All firmware files loaded successfully");
            this.updateProgress(25, "Starting flash operation...");

            // Write files to flash using esptool-js format
            const flashOptions = {
                fileArray: fileArray,
                flashSize: "keep",
                flashMode: "keep",
                flashFreq: "keep",
                eraseAll: false,
                compress: true,
                // Disable MD5 verification to avoid issues
                calculateMD5Hash: null,
                reportProgress: (fileIndex, written, total) => {
                    if (total > 0) {
                        const baseProgress = 25 + (fileIndex * 25);
                        const fileProgress = (written / total) * 25;
                        const totalProgress = baseProgress + fileProgress;
                        const fwLabel = this.firmwareFiles[fileIndex].name || this.firmwareFiles[fileIndex].file.split('/').pop();
                        this.updateProgress(totalProgress, `Flashing ${fwLabel}...`);
                    }
                }
            };

            this.log("Calling writeFlash...");
            await this.esploader.writeFlash(flashOptions);

            this.updateProgress(100, "Flash complete!");
            this.log("✅ Firmware flashed successfully!");
            this.log("Resetting device...");

            // Reset device using DTR/RTS
            try {
                // Try esptool's hardReset first
                await this.esploader.hardReset();
                this.log("Device reset command sent!");

                // Additional manual reset sequence
                await new Promise(resolve => setTimeout(resolve, 100));
                await this.transport.setRTS(true);
                await this.transport.setDTR(false);
                await new Promise(resolve => setTimeout(resolve, 100));
                await this.transport.setRTS(false);

                this.log("Hardware reset triggered - device should reboot now");
            } catch (resetErr) {
                this.log(`Reset note: ${resetErr.message}`);
                this.log("⚠️ Automatic reset may not work - please press RESET button on device");
            }

            await this.finishAfterFlash();

        } catch (err) {
            this.log(`❌ Flash error: ${err.message}`);
            await this.disconnect({ quiet: true });
            this.setState('idle', `Flashing failed: ${err.message}. Click Connect and try again, or use Full Erase & Flash.`, 'err');
        }
    }

    async eraseFlash() {
        if (!this.connected) {
            this.log("Error: Not connected to device");
            return;
        }

        if (!confirm("This will erase all data on the device. Continue?")) {
            return;
        }

        try {
            this.log("Erasing flash...");
            this.updateProgress(0, "Erasing...");

            await this.esploader.eraseFlash();

            this.updateProgress(100, "Erase complete!");
            this.log("✅ Flash erased successfully");
            this.setState('connected', 'Erased. Click Flash Firmware to install.', 'ok');

        } catch (err) {
            this.log(`❌ Erase error: ${err.message}`);
            this.setState('connected', `Erase failed: ${err.message}`, 'err');
        }
    }

    // Full erase and flash - recommended for devices with USB issues
    async eraseAndFlash() {
        if (!this.connected) {
            this.log("Error: Not connected to device");
            return;
        }

        if (!confirm("This will COMPLETELY ERASE the device and flash fresh firmware.\n\nThis is recommended if you're having USB/serial connection issues.\n\nContinue?")) {
            return;
        }

        try {
            this.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            this.log("FULL ERASE & FLASH");
            this.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            this.log("");
            this.log("Step 1/2: Erasing entire flash...");
            this.log("This removes all old data including USB configuration.");
            this.updateProgress(0, "Erasing entire flash...");

            await this.esploader.eraseFlash();

            this.log("✓ Flash erased completely");
            this.updateProgress(20, "Erase complete, preparing firmware...");

            // Small delay to let the chip settle
            await new Promise(resolve => setTimeout(resolve, 500));

            this.log("");
            this.log("Step 2/2: Flashing fresh firmware...");

            // Now flash the firmware
            await this.flashFirmwareInternal();

            this.log("");
            this.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            this.log("✅ FULL ERASE & FLASH COMPLETE!");
            this.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            this.log("");
            this.log("IMPORTANT: Unplug and replug your device.");
            this.log("The USB configuration has been completely reset.");
            this.log("Future updates via web serial should now work properly.");

            await this.finishAfterFlash();

        } catch (err) {
            this.log(`❌ Error: ${err.message}`);
            await this.disconnect({ quiet: true });
            this.setState('idle', `Full erase & flash failed: ${err.message}. Click Connect and try again.`, 'err');
        }
    }

    // Internal flash method (no prompts, used by eraseAndFlash)
    async flashFirmwareInternal() {
        this.log("Loading firmware files...");

        // Load all firmware files
        const fileArray = [];
        for (let i = 0; i < this.firmwareFiles.length; i++) {
            const fw = this.firmwareFiles[i];
            const label = fw.name || fw.file.split('/').pop();
            const progress = 20 + (i / this.firmwareFiles.length) * 15;
            this.updateProgress(progress, `Loading ${label}...`);

            const data = await this.loadFirmwareFile(fw.file, fw.name);

            this.log(`Loaded ${label}: ${data.length} bytes at 0x${fw.address.toString(16)}`);

            fileArray.push({
                data: data,
                address: fw.address
            });
        }

        this.log("All firmware files loaded, starting flash...");
        this.updateProgress(40, "Starting flash operation...");

        const flashOptions = {
            fileArray: fileArray,
            flashSize: "keep",
            flashMode: "keep",
            flashFreq: "keep",
            eraseAll: false,
            compress: true,
            calculateMD5Hash: null,
            reportProgress: (fileIndex, written, total) => {
                if (total > 0) {
                    const baseProgress = 40 + (fileIndex * 20);
                    const fileProgress = (written / total) * 20;
                    const totalProgress = baseProgress + fileProgress;
                    const fwLabel = this.firmwareFiles[fileIndex].name || this.firmwareFiles[fileIndex].file.split('/').pop();
                    this.updateProgress(totalProgress, `Flashing ${fwLabel}...`);
                }
            }
        };

        await this.esploader.writeFlash(flashOptions);

        this.updateProgress(100, "Flash complete!");
        this.log("✓ Firmware flashed successfully!");

        // Reset device
        try {
            await this.esploader.hardReset();
            this.log("Device reset command sent");
        } catch (resetErr) {
            this.log("Note: Auto-reset may not work - please press RESET or unplug/replug");
        }
    }

    async disconnect({ quiet = false } = {}) {
        this.log("Disconnecting...");

        if (this.transport) {
            try { await this.transport.disconnect(); }
            catch (err) { this.log(`Transport disconnect error: ${err.message}`); }
            this.transport = null;
        }

        // Close, but keep the browser's permission. Forgetting the port meant
        // every visit started with a fresh picker; keeping it lets the next
        // Connect find the Summit without asking.
        if (this.device) {
            try {
                if (this.device.readable || this.device.writable) await this.device.close();
            } catch (err) {
                this.log(`Device close error: ${err.message}`);
            }
            this.device = null;
        }

        this.connected = false;
        this.esploader = null;
        if (!quiet) this.setState('idle', '');
        this.log("Disconnected");
    }
}

// Initialize flasher when page loads
let espFlasher = null;

function initializeESPFlasher() {
    if (!espFlasher) {
        espFlasher = new ESP32Flasher();
    }
    // Reachable from the console, which helps when someone reports a problem.
    window.espFlasher = espFlasher;

    const wire = (id, handler) => {
        const el = document.getElementById(id);
        if (!el || el.dataset.listenerAttached) return;
        el.dataset.listenerAttached = 'true';
        el.addEventListener('click', handler);
    };

    // Flash and erase lock the page while they run, then put it back to
    // whatever state the operation left behind.
    const busy = async (id, label, fn) => {
        const el = document.getElementById(id);
        const original = el ? el.textContent : '';
        espFlasher.setState('busy', 'Working on it. Keep the Summit plugged in until this finishes.');
        if (el) el.textContent = label;
        try {
            await fn();
        } finally {
            if (el) el.textContent = original;
            if (espFlasher.state === 'busy') {
                espFlasher.setState(espFlasher.connected ? 'connected' : 'idle');
            }
        }
    };

    wire('enterBootloaderButton', () => espFlasher.connectToSummit());
    wire('directConnectButton', () => espFlasher.connectToSummit());
    wire('summitManualPortButton', () => espFlasher.connectToSummit({ showAll: true }));
    wire('disconnectButton', () => espFlasher.disconnect());
    wire('programButton', () => busy('programButton', 'Flashing\u2026', () => espFlasher.flashFirmware()));
    wire('eraseButton', () => busy('eraseButton', 'Erasing\u2026', () => espFlasher.eraseFlash()));
    wire('eraseAndFlashButton', () => busy('eraseAndFlashButton', 'Erasing & flashing\u2026', () => espFlasher.eraseAndFlash()));

    espFlasher.setState(espFlasher.connected ? 'connected' : 'idle');
}

// Export for use by main script
window.initializeESPFlasher = initializeESPFlasher;

// Try to initialize on load
console.log('ESP Flasher module loaded, readyState:', document.readyState);
if (document.readyState === 'loading') {
    console.log('Waiting for DOMContentLoaded...');
    document.addEventListener('DOMContentLoaded', initializeESPFlasher);
} else {
    console.log('DOM already loaded, initializing immediately');
    initializeESPFlasher();
}
