// ESP32 Web Flasher for Vail Summit
// Uses esptool-js for web-based ESP32 flashing

class ESP32Flasher {
    constructor() {
        this.port = null;
        this.transport = null;
        this.esploader = null;
        this.connected = false;
        this.chipName = null;

        // Firmware file paths (relative to docs/)
        this.firmwareFiles = [
            { address: 0x0, file: 'firmware_files/summit/bootloader.bin' },
            { address: 0x8000, file: 'firmware_files/summit/partitions.bin' },
            { address: 0x10000, file: 'firmware_files/summit/vail-summit.bin' }
        ];
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
            this.log("Requesting serial port...");
            this.port = await navigator.serial.requestPort({
                filters: [
                    { usbVendorId: 0x303A }, // Espressif
                ]
            });

            this.log("Opening port...");
            await this.port.open({ baudRate: 115200 });

            // Import ESPLoader dynamically
            const { Transport } = await import('https://unpkg.com/esptool-js@0.4.1/bundle.js');

            this.log("Initializing transport...");
            this.transport = new Transport(this.port, true);

            // Import and create ESPLoader
            const { ESPLoader } = await import('https://unpkg.com/esptool-js@0.4.1/bundle.js');
            this.esploader = new ESPLoader({
                transport: this.transport,
                baudrate: 115200,
                terminal: {
                    clean: () => {},
                    writeLine: (data) => this.log(data),
                    write: (data) => this.log(data)
                }
            });

            this.log("Connecting to ESP32...");
            this.chipName = await this.esploader.main();
            this.log(`Connected to ${this.chipName}`);

            this.connected = true;

            // Update UI
            document.getElementById('connectButton').style.display = 'none';
            document.getElementById('programButton').style.display = 'inline-block';
            document.getElementById('eraseButton').style.display = 'inline-block';

            return true;

        } catch (err) {
            this.log(`Connection error: ${err.message}`);
            alert(`Failed to connect: ${err.message}`);
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
            const blob = await response.blob();
            const arrayBuffer = await blob.arrayBuffer();
            this.log(`Downloaded ${url} (${arrayBuffer.byteLength} bytes)`);
            return arrayBuffer;
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
                fileArray.push({
                    data: data,
                    address: address
                });
            }

            this.log("All firmware files loaded successfully");
            this.updateProgress(25, "Erasing flash...");

            // Write files to flash
            await this.esploader.writeFlash({
                fileArray: fileArray,
                flashSize: "keep",
                eraseAll: false,
                compress: true,
                reportProgress: (fileIndex, written, total) => {
                    const baseProgress = 25 + (fileIndex * 25);
                    const fileProgress = (written / total) * 25;
                    const totalProgress = baseProgress + fileProgress;
                    this.updateProgress(
                        totalProgress,
                        `Flashing ${this.firmwareFiles[fileIndex].file}...`
                    );
                }
            });

            this.updateProgress(100, "Flash complete!");
            this.log("✅ Firmware flashed successfully!");
            alert("Firmware flashed successfully! Your device will now reboot.");

            // Reset device
            await this.esploader.hardReset();

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
        if (this.transport) {
            try {
                await this.transport.disconnect();
            } catch (err) {
                this.log(`Disconnect error: ${err.message}`);
            }
        }

        if (this.port) {
            try {
                await this.port.close();
            } catch (err) {
                // Port may already be closed
            }
        }

        this.connected = false;
        this.port = null;
        this.transport = null;
        this.esploader = null;

        // Reset UI
        document.getElementById('connectButton').style.display = 'inline-block';
        document.getElementById('programButton').style.display = 'none';
        document.getElementById('eraseButton').style.display = 'none';

        this.log("Disconnected");
    }
}

// Initialize flasher when page loads
let espFlasher = null;

window.addEventListener('DOMContentLoaded', () => {
    espFlasher = new ESP32Flasher();

    // Connect button
    const connectButton = document.getElementById('connectButton');
    if (connectButton) {
        connectButton.addEventListener('click', async () => {
            connectButton.disabled = true;
            connectButton.textContent = 'Connecting...';

            const success = await espFlasher.connect();

            if (!success) {
                connectButton.disabled = false;
                connectButton.textContent = 'Connect to Vail Summit';
            }
        });
    }

    // Program button
    const programButton = document.getElementById('programButton');
    if (programButton) {
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
    if (eraseButton) {
        eraseButton.addEventListener('click', async () => {
            eraseButton.disabled = true;
            const originalText = eraseButton.textContent;
            eraseButton.textContent = 'Erasing...';

            await espFlasher.eraseFlash();

            eraseButton.disabled = false;
            eraseButton.textContent = originalText;
        });
    }
});
