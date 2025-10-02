const uf2Files = {
    "xiao_non_pcb": { url: "firmware_files/xiao_non_pcb.uf2", filename: "xiao_non_pcb.uf2" },
    "xiao_basic_pcb_v1": { url: "firmware_files/xiao_basic_pcb_v1.uf2", filename: "xiao_basic_pcb_v1.uf2" },
    "xiao_basic_pcb_v2": { url: "firmware_files/xiao_basic_pcb_v2.uf2", filename: "xiao_basic_pcb_v2.uf2" },
    "qtpy_non_pcb": { url: "firmware_files/qtpy_non_pcb.uf2", filename: "qtpy_non_pcb.uf2" },
    "qtpy_basic_pcb_v1": { url: "firmware_files/qtpy_basic_pcb_v1.uf2", filename: "qtpy_basic_pcb_v1.uf2" },
    "qtpy_basic_pcb_v2": { url: "firmware_files/qtpy_basic_pcb_v2.uf2", filename: "qtpy_basic_pcb_v2.uf2" },
    "qtpy_advanced_pcb": { url: "firmware_files/qtpy_advanced_pcb.uf2", filename: "qtpy_advanced_pcb.uf2" }
};

// Ensure download button is disabled on page load if no version is pre-selected
document.addEventListener('DOMContentLoaded', () => {
    const bootModeButton = document.getElementById('bootModeButton');
    const downloadButton = document.getElementById('downloadButton');
    const adapterVersionRadios = document.querySelectorAll('input[name="adapter_version"]');

    // Setup event listener for bootModeButton
    if (bootModeButton) {
        // The old enterBootMode function and its listener should have been removed.
        // This specifically targets the WebSerial function.
        bootModeButton.addEventListener('click', triggerBootloaderViaWebSerial);
    } else {
        console.error("bootModeButton not found during event listener setup.");
    }

    // Setup event listeners for radio buttons
    if (adapterVersionRadios && downloadButton) { // Ensure buttons exist
        adapterVersionRadios.forEach(radio => {
            radio.addEventListener('change', function() {
                const selectedVersion = this.value;
                if (uf2Files[selectedVersion]) {
                    const fileInfo = uf2Files[selectedVersion];
                    downloadButton.href = fileInfo.url;
                    downloadButton.download = fileInfo.filename;
                    downloadButton.textContent = `Download ${fileInfo.filename}`;
                    downloadButton.classList.remove('disabled');
                    downloadButton.removeAttribute('aria-disabled');
                } else {
                    downloadButton.href = "#";
                    downloadButton.removeAttribute('download');
                    downloadButton.textContent = 'Download UF2 File';
                    downloadButton.classList.add('disabled');
                    downloadButton.setAttribute('aria-disabled', 'true');
                }
            });
        });

        // Initial check for download button state
        let isVersionSelected = false;
        adapterVersionRadios.forEach(radio => {
            if (radio.checked) {
                isVersionSelected = true;
                radio.dispatchEvent(new Event('change')); // Trigger change to update download button
            }
        });

        if (!isVersionSelected) { // Redundant check if downloadButton doesn't exist, but safe
            downloadButton.classList.add('disabled');
            downloadButton.setAttribute('aria-disabled', 'true');
            downloadButton.href = "#";
            downloadButton.addEventListener('click', function(event) {
                if (this.classList.contains('disabled')) {
                    event.preventDefault();
                    alert("Please select an adapter version first.");
                }
            });
        } else { // If a version is selected, ensure the click handler for enabled state is also correct
            downloadButton.addEventListener('click', function(event) {
                if (this.classList.contains('disabled')) { // Should not be the case if version selected
                    event.preventDefault();
                    alert("Please select an adapter version first.");
                }
            });
        }
    } else {
        if (!adapterVersionRadios) console.error("adapterVersionRadios not found.");
        if (!downloadButton) console.error("downloadButton not found.");
    }
});

let port; // To store the serial port object

// Add a log function similar to the example, targetting a new log area in HTML
// We'll add the HTML element in the next plan step. For now, just console.log.
function logToPage(message) {
    console.log(message); // Placeholder for actual page logging
    const logArea = document.getElementById('serialLog'); // Assume this ID will exist
    if (logArea) {
        logArea.textContent += message + '\n';
        logArea.scrollTop = logArea.scrollHeight;
    }
}

async function triggerBootloaderViaWebSerial() {
    logToPage("Attempting to trigger bootloader via WebSerial...");

    if (!("serial" in navigator)) {
        logToPage("Error: WebSerial API not supported by this browser. Please use a compatible browser like Chrome or Edge.");
        alert("WebSerial API not supported. Please use Chrome or Edge.");
        return;
    }

    try {
        // If a port is already open from a previous attempt, try to close it first.
        // This is tricky because the user might have selected a different physical device.
        // For simplicity in this step, we'll assume 'port' is the one we want to reuse or re-request.
        if (port && port.readable) { // Check if port is defined and seems open
            logToPage("Closing previously opened port (if any)...");
            try {
                // Await readable and writable to be closed if they exist
                if (port.readable) {
                     // Best effort to cancel any pending reads if a reader was active
                    const reader = port.readable.getReader();
                    reader.cancel();
                    reader.releaseLock();
                }
                await port.close();
                logToPage("Previously opened port closed.");
            } catch (closeErr) {
                logToPage(`Note: Error closing previous port, this might be okay: ${closeErr.message}`);
            }
            port = null; // Reset port variable
        }

        logToPage("Linux users: If multiple ports are listed, please select the one that typically represents your Arduino device (e.g., /dev/ttyACM0 or /dev/ttyUSB0).");
        logToPage("Requesting serial port selection...");
        port = await navigator.serial.requestPort();
        logToPage("Port selected.");

        logToPage("Attempting 1200bps touch to trigger bootloader...");
        await port.open({ baudRate: 1200 });
        logToPage("Port opened at 1200bps.");

        await port.close();
        logToPage("Port closed after 1200bps touch.");
        logToPage("SUCCESS: Bootloader mode command sent! Your device should have disconnected and shortly reappear as a storage drive (e.g., ADAPTERBOOT, QTPYBOOT). Check your computer now.");
        alert("Bootloader mode command sent! Check your device. It should now appear as a storage drive.");
        
        // Update button states (assuming bootModeButton is the trigger)
        const bootModeButton = document.getElementById('bootModeButton');
        if (bootModeButton) {
            // bootModeButton.disabled = true; // Or re-enable to try again
        }
        port = null; // Release the port

    } catch (err) {
        logToPage(`Error: ${err.message}`);
        if (err.name === 'NotFoundError') {
            alert("Serial port selection cancelled or no compatible device found.");
        } else if (err.name === 'InvalidStateError' && port && !port.readable) {
             alert("Serial port is already closed. Please try again.");
        } else {
            alert(`An error occurred: ${err.message}`);
        }
        // Reset port if an error occurred and it's defined
        if (port) {
            try { await port.close(); } catch (e) { /* ignore */ }
            port = null;
        }
    }
}

// Assuming this runs after DOM is loaded, or bootModeButton is already in the DOM
// The event listener setup for bootModeButton is now inside DOMContentLoaded.
// This section is removed to avoid re-declaration and ensure DOM readiness.
