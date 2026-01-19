// ESP32 Web Flasher for Vail Summit
// Uses esptool-js for web-based ESP32 flashing

class ESP32Flasher {
    constructor() {
        this.port = null;
        this.device = null;
        this.transport = null;
        this.esploader = null;
        this.connected = false;
        this.chipName = null;
        this.esptoolModule = null;
        this.bootloaderModeReady = false;

        // Firmware file URLs from the Vail Summit repository
        const summitFirmwareBase = 'https://raw.githubusercontent.com/Vail-CW/vail-summit/main/firmware_files/';
        this.firmwareFiles = [
            { address: 0x0, file: summitFirmwareBase + 'bootloader.bin' },
            { address: 0x8000, file: summitFirmwareBase + 'partitions.bin' },
            { address: 0x10000, file: summitFirmwareBase + 'vail-summit.bin' }
        ];
    }

    async waitForNewPort(originalPorts, maxWaitMs = 10000) {
        const startTime = Date.now();
        this.log("Waiting for device to reconnect in bootloader mode...");

        while (Date.now() - startTime < maxWaitMs) {
            await new Promise(resolve => setTimeout(resolve, 500));

            const currentPorts = await navigator.serial.getPorts();

            // Find new port that wasn't in original list
            const newPort = currentPorts.find(port => !originalPorts.includes(port));
            if (newPort) {
                this.log("✓ New port detected (device in bootloader mode)");
                return newPort;
            }
        }

        this.log("⚠️ New port not detected within timeout");
        return null;
    }

    async enterBootloaderMode() {
        if (!("serial" in navigator)) {
            this.log("Error: Web Serial API not supported. Please use Chrome, Edge, or Opera.");
            alert("Web Serial API not supported. Please use Chrome, Edge, or Opera browser.");
            return false;
        }

        try {
            // First, close all previously granted ports
            const ports = await navigator.serial.getPorts();
            this.log(`Found ${ports.length} previously granted port(s)`);
            for (const port of ports) {
                if (port.readable || port.writable) {
                    this.log("Closing a previously open port...");
                    try {
                        await port.close();
                    } catch (e) {
                        this.log(`Note: ${e.message}`);
                    }
                }
            }

            this.log("Select your Vail Summit device");
            const originalPorts = await navigator.serial.getPorts();

            // Select the device in normal mode
            const normalDevice = await navigator.serial.requestPort();

            // Check if this specific port is already open
            if (normalDevice.readable || normalDevice.writable) {
                this.log("Port is already open, closing it first...");
                try {
                    await normalDevice.close();
                    await new Promise(resolve => setTimeout(resolve, 500));
                } catch (e) {
                    this.log(`Note: ${e.message}`);
                }
            }

            this.log("Performing 1200-bps touch reset to enter bootloader mode...");

            try {
                await normalDevice.open({ baudRate: 1200 });
                await new Promise(resolve => setTimeout(resolve, 100));
                await normalDevice.close();
                this.log("✓ Reset signal sent");
            } catch (openErr) {
                this.log(`Error: ${openErr.message}`);
                this.log("⚠️ Port may be in use by another application (Arduino IDE?)");
                throw new Error("Failed to send reset signal. Close other applications and try again.");
            }

            // Wait for new port to appear
            this.device = await this.waitForNewPort(originalPorts);

            if (!this.device) {
                this.log("⚠️ Device did not reconnect automatically");
                this.log("Try manually entering bootloader mode:");
                this.log("1. Hold BOOT button");
                this.log("2. Press and release RESET");
                this.log("3. Release BOOT");
                this.log("4. Click 'Connect and Flash' below");

                // Still allow proceeding with manual selection
                this.bootloaderModeReady = true;
                document.getElementById('enterBootloaderButton').style.display = 'none';
                document.getElementById('connectButton').style.display = 'inline-block';
                return true;
            }

            this.log("✓ Device ready in bootloader mode");
            this.bootloaderModeReady = true;

            // Skip step 2, go directly to connect
            document.getElementById('enterBootloaderButton').style.display = 'none';
            document.getElementById('connectButton').style.display = 'inline-block';

            return true;

        } catch (err) {
            this.log(`Error: ${err.message}`);

            if (err.name === 'NotFoundError') {
                this.log("No device selected");
                return false;
            }

            // Allow manual retry
            this.log("Click button again to retry, or enter bootloader mode manually");
            return false;
        }
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

    async connect() {
        if (!("serial" in navigator)) {
            this.log("Error: Web Serial API not supported. Please use Chrome, Edge, or Opera.");
            alert("Web Serial API not supported. Please use Chrome, Edge, or Opera browser.");
            return false;
        }

        try {
            // Disconnect any existing connections first
            if (this.transport) {
                this.log("Closing existing connection...");
                await this.disconnect();
                await new Promise(resolve => setTimeout(resolve, 500));
            }

            // Load esptool module if not already loaded
            if (!this.esptoolModule) {
                this.log("Loading esptool-js library...");
                this.esptoolModule = await import('https://unpkg.com/esptool-js@0.4.1/bundle.js');
                this.log("Esptool-js loaded successfully");
            }

            // If we don't have a device from enterBootloaderMode, prompt for one
            if (!this.device) {
                this.log("Requesting device selection...");
                this.device = await navigator.serial.requestPort();
            } else {
                this.log("Using previously detected device in bootloader mode");
            }

            // Check if port is already open and close it
            if (this.device.readable || this.device.writable) {
                this.log("Port is already open, closing it first...");
                try {
                    await this.device.close();
                    this.log("Port closed, waiting...");
                    await new Promise(resolve => setTimeout(resolve, 1000));
                } catch (e) {
                    this.log(`Error closing port: ${e.message}`);
                }
            }

            // Initialize transport - this will open the port
            this.log("Initializing transport (opening port at 115200 baud)...");
            this.transport = new this.esptoolModule.Transport(this.device, true);

            this.log("Creating ESPLoader...");
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

            this.log("Connecting to ESP32 in bootloader mode...");
            this.chipName = await this.esploader.main();
            this.log(`✓ Connected to ${this.chipName}`);

            this.connected = true;

            // Update UI
            document.getElementById('connectButton').style.display = 'none';
            document.getElementById('disconnectButton').style.display = 'inline-block';
            document.getElementById('programButton').style.display = 'inline-block';
            document.getElementById('eraseButton').style.display = 'inline-block';

            return true;

        } catch (err) {
            this.log(`Connection error: ${err.message}`);
            alert(`Failed to connect: ${err.message}\n\nTry:\n1. Click 'Step 1' again to retry\n2. Manually enter bootloader mode (hold BOOT, press RESET)\n3. Make sure no other programs are using the port`);
            await this.disconnect();
            return false;
        }
    }

    async loadFirmwareFile(url) {
        this.log(`Downloading ${url}...`);
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

            this.log(`Downloaded ${url} (${binaryString.length} bytes)`);
            return binaryString;
        } catch (err) {
            this.log(`Error downloading ${url}: ${err.message}`);
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
                const { address, file } = this.firmwareFiles[i];
                this.updateProgress((i / this.firmwareFiles.length) * 20, `Loading ${file}...`);

                const data = await this.loadFirmwareFile(file);

                this.log(`Prepared ${file}: ${data.length} bytes at 0x${address.toString(16)}`);

                fileArray.push({
                    data: data,
                    address: address
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
                        this.updateProgress(
                            totalProgress,
                            `Flashing ${this.firmwareFiles[fileIndex].file}...`
                        );
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

            alert("Firmware flashed successfully!\n\nPlease unplug and replug your Vail Summit to complete the update.");

        } catch (err) {
            this.log(`❌ Flash error: ${err.message}`);
            alert(`Flashing failed: ${err.message}`);
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
            alert("Flash erased successfully!");

        } catch (err) {
            this.log(`❌ Erase error: ${err.message}`);
            alert(`Erase failed: ${err.message}`);
        }
    }

    async disconnect() {
        this.log("Disconnecting...");

        if (this.transport) {
            try {
                await this.transport.disconnect();
            } catch (err) {
                this.log(`Transport disconnect error: ${err.message}`);
            }
            this.transport = null;
        }

        if (this.device) {
            try {
                // Close the port if it's open
                if (this.device.readable || this.device.writable) {
                    await this.device.close();
                }
                // Forget the device to fully release it
                if (this.device.forget) {
                    await this.device.forget();
                }
            } catch (err) {
                this.log(`Device close error: ${err.message}`);
            }
            this.device = null;
        }

        this.connected = false;
        this.esploader = null;

        // Reset UI
        const connectBtn = document.getElementById('connectButton');
        const disconnectBtn = document.getElementById('disconnectButton');
        const programBtn = document.getElementById('programButton');
        const eraseBtn = document.getElementById('eraseButton');

        if (connectBtn) connectBtn.style.display = 'inline-block';
        if (disconnectBtn) disconnectBtn.style.display = 'none';
        if (programBtn) programBtn.style.display = 'none';
        if (eraseBtn) eraseBtn.style.display = 'none';

        this.log("Disconnected");
    }
}

// Initialize flasher when page loads
let espFlasher = null;

function initializeESPFlasher() {
    if (!espFlasher) {
        espFlasher = new ESP32Flasher();
    }

    // Enter Bootloader button (Step 1)
    const enterBootloaderButton = document.getElementById('enterBootloaderButton');
    if (enterBootloaderButton && !enterBootloaderButton.dataset.listenerAttached) {
        enterBootloaderButton.dataset.listenerAttached = 'true';
        console.log('ESP Flasher: Attaching click listener to enter bootloader button');
        enterBootloaderButton.addEventListener('click', async () => {
            console.log('ESP Flasher: Enter bootloader button clicked!');
            enterBootloaderButton.disabled = true;
            enterBootloaderButton.textContent = 'Triggering bootloader...';

            const success = await espFlasher.enterBootloaderMode();

            if (!success) {
                enterBootloaderButton.disabled = false;
                enterBootloaderButton.textContent = 'Step 1: Enter Bootloader Mode';
            }
        });
    }

    // Connect button (Step 2)
    const connectButton = document.getElementById('connectButton');
    if (connectButton && !connectButton.dataset.listenerAttached) {
        connectButton.dataset.listenerAttached = 'true';
        console.log('ESP Flasher: Attaching click listener to connect button');
        connectButton.addEventListener('click', async () => {
            console.log('ESP Flasher: Connect button clicked!');
            connectButton.disabled = true;
            connectButton.textContent = 'Connecting...';

            const success = await espFlasher.connect();

            connectButton.disabled = false;
            if (!success) {
                connectButton.textContent = 'Step 2: Connect in Bootloader Mode';
            }
        });
    } else if (!connectButton) {
        console.error('ESP Flasher: Connect button not found!');
    } else {
        console.log('ESP Flasher: Listener already attached');
    }

    // Disconnect button
    const disconnectButton = document.getElementById('disconnectButton');
    if (disconnectButton && !disconnectButton.dataset.listenerAttached) {
        disconnectButton.dataset.listenerAttached = 'true';
        disconnectButton.addEventListener('click', async () => {
            disconnectButton.disabled = true;
            await espFlasher.disconnect();
            disconnectButton.disabled = false;
        });
    }

    // Program button
    const programButton = document.getElementById('programButton');
    if (programButton && !programButton.dataset.listenerAttached) {
        programButton.dataset.listenerAttached = 'true';
        programButton.addEventListener('click', async () => {
            programButton.disabled = true;
            const originalText = programButton.textContent;
            programButton.textContent = 'Flashing...';

            await espFlasher.flashFirmware();

            programButton.disabled = false;
            programButton.textContent = originalText;
        });
    }

    // Erase button
    const eraseButton = document.getElementById('eraseButton');
    if (eraseButton && !eraseButton.dataset.listenerAttached) {
        eraseButton.dataset.listenerAttached = 'true';
        eraseButton.addEventListener('click', async () => {
            eraseButton.disabled = true;
            const originalText = eraseButton.textContent;
            eraseButton.textContent = 'Erasing...';

            await espFlasher.eraseFlash();

            eraseButton.disabled = false;
            eraseButton.textContent = originalText;
        });
    }
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
