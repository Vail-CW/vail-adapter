// Wizard state management
const wizardState = {
    currentStep: 1,
    model: null,      // 'basic_pcb', 'advanced_pcb', or 'non_pcb'
    board: null,      // 'qtpy' or 'xiao'
};

// Firmware file mapping
function getFirmwareFile() {
    if (!wizardState.model || !wizardState.board) return null;

    // Build firmware filename based on selections
    // For Basic PCB, use v2 as default (latest version)
    let filename;
    if (wizardState.model === 'basic_pcb') {
        filename = `${wizardState.board}_basic_pcb_v2.uf2`;
    } else if (wizardState.model === 'advanced_pcb') {
        filename = `${wizardState.board}_advanced_pcb.uf2`;
    } else {
        filename = `${wizardState.board}_non_pcb.uf2`;
    }

    return {
        url: `firmware_files/${filename}`,
        filename: filename
    };
}

// Get friendly names for display
function getModelName(model) {
    const names = {
        'basic_pcb': 'Basic PCB',
        'advanced_pcb': 'Advanced PCB',
        'non_pcb': 'DIY No PCB'
    };
    return names[model] || model;
}

function getBoardName(board) {
    const names = {
        'qtpy': 'Adafruit QT Py SAMD21',
        'xiao': 'Seeeduino XIAO SAMD21'
    };
    return names[board] || board;
}

// Step navigation functions
function goToStep(stepNumber) {
    // Hide all steps
    document.querySelectorAll('.wizard-step').forEach(step => {
        step.classList.remove('active');
    });

    // Show target step
    const targetStep = document.getElementById(`step${stepNumber}`);
    if (targetStep) {
        targetStep.classList.add('active');
        wizardState.currentStep = stepNumber;
        updateProgressBar(stepNumber);

        // Update step 2 content if navigating there
        if (stepNumber === 2) {
            updateStep2Content();
        }

        // Update step 3 content if navigating there
        if (stepNumber === 3) {
            updateStep3Content();
        }
    }
}

function updateStep2Content() {
    // Show/hide QT Py hint based on model selection
    const qtpyHint = document.getElementById('qtpyHint');
    if (qtpyHint) {
        if (wizardState.model === 'non_pcb') {
            qtpyHint.style.display = 'none';
        } else {
            qtpyHint.style.display = 'block';
        }
    }
}

function updateProgressBar(stepNumber) {
    document.querySelectorAll('.progress-step').forEach(step => {
        const stepNum = parseInt(step.dataset.step);
        if (stepNum < stepNumber) {
            step.classList.add('completed');
            step.classList.remove('active');
        } else if (stepNum === stepNumber) {
            step.classList.add('active');
            step.classList.remove('completed');
        } else {
            step.classList.remove('active', 'completed');
        }
    });
}

function updateStep3Content() {
    // Update selected configuration display
    const configText = `${getModelName(wizardState.model)} + ${getBoardName(wizardState.board)}`;
    document.getElementById('selectedConfig').textContent = configText;

    // Update download button
    const downloadButton = document.getElementById('downloadButton');
    const downloadText = document.getElementById('downloadText');
    const firmwareFile = getFirmwareFile();

    if (firmwareFile) {
        downloadButton.href = firmwareFile.url;
        downloadButton.download = firmwareFile.filename;
        downloadText.textContent = `Download ${firmwareFile.filename}`;
        downloadButton.classList.remove('disabled');
        downloadButton.removeAttribute('aria-disabled');
    }
}

// WebSerial boot mode functionality
let port;

function logToPage(message) {
    console.log(message);
    const logArea = document.getElementById('serialLog');
    if (logArea) {
        logArea.textContent += message + '\n';
        logArea.scrollTop = logArea.scrollHeight;
    }
}

async function triggerBootloaderViaWebSerial() {
    logToPage("Attempting to trigger bootloader via WebSerial...");

    if (!("serial" in navigator)) {
        logToPage("Error: WebSerial API not supported by this browser. Please use Chrome, Edge, or Opera.");
        alert("WebSerial API not supported. Please use Chrome, Edge, or Opera browser, or try the manual reset method.");
        return;
    }

    try {
        // Close previous port if open
        if (port && port.readable) {
            logToPage("Closing previously opened port...");
            try {
                if (port.readable) {
                    const reader = port.readable.getReader();
                    reader.cancel();
                    reader.releaseLock();
                }
                await port.close();
                logToPage("Previously opened port closed.");
            } catch (closeErr) {
                logToPage(`Note: Error closing previous port: ${closeErr.message}`);
            }
            port = null;
        }

        logToPage("Linux users: Select the port that represents your Arduino (e.g., /dev/ttyACM0 or /dev/ttyUSB0).");
        logToPage("Requesting serial port selection...");
        port = await navigator.serial.requestPort();
        logToPage("Port selected.");

        logToPage("Attempting 1200bps touch to trigger bootloader...");
        await port.open({ baudRate: 1200 });
        logToPage("Port opened at 1200bps.");

        await port.close();
        logToPage("Port closed after 1200bps touch.");
        logToPage("✅ SUCCESS: Bootloader mode command sent!");
        logToPage("Your device should now appear as a storage drive (QTPYBOOT, XIAOBOOT, or ADAPTERBOOT).");
        alert("Bootloader mode activated! Your device should now appear as a USB storage drive. Proceed to download the firmware.");

        port = null;

    } catch (err) {
        logToPage(`❌ Error: ${err.message}`);
        if (err.name === 'NotFoundError') {
            alert("Serial port selection cancelled or no compatible device found.");
        } else if (err.name === 'InvalidStateError') {
            alert("Serial port is already closed. Please try again.");
        } else {
            alert(`An error occurred: ${err.message}`);
        }

        if (port) {
            try { await port.close(); } catch (e) { /* ignore */ }
            port = null;
        }
    }
}

// Initialize wizard on page load
document.addEventListener('DOMContentLoaded', () => {
    // Step 1: Model selection
    document.querySelectorAll('#step1 .selection-card').forEach(card => {
        card.addEventListener('click', () => {
            // Remove selected state from all cards
            document.querySelectorAll('#step1 .selection-card').forEach(c => {
                c.classList.remove('selected');
            });

            // Mark this card as selected
            card.classList.add('selected');
            wizardState.model = card.dataset.model;

            // Advance to step 2 after a short delay
            setTimeout(() => {
                goToStep(2);
            }, 300);
        });
    });

    // Step 2: Board selection
    document.querySelectorAll('#step2 .selection-card').forEach(card => {
        card.addEventListener('click', () => {
            // Remove selected state from all cards
            document.querySelectorAll('#step2 .selection-card').forEach(c => {
                c.classList.remove('selected');
            });

            // Mark this card as selected
            card.classList.add('selected');
            wizardState.board = card.dataset.board;

            // Advance to step 3 after a short delay
            setTimeout(() => {
                goToStep(3);
            }, 300);
        });
    });

    // Back buttons
    document.getElementById('backToStep1')?.addEventListener('click', () => {
        goToStep(1);
    });

    document.getElementById('backToStep2')?.addEventListener('click', () => {
        goToStep(2);
    });

    // Start over button
    document.getElementById('startOver')?.addEventListener('click', () => {
        wizardState.model = null;
        wizardState.board = null;
        document.querySelectorAll('.selection-card').forEach(card => {
            card.classList.remove('selected');
        });
        goToStep(1);
    });

    // Boot mode button
    document.getElementById('bootModeButton')?.addEventListener('click', triggerBootloaderViaWebSerial);

    // Download button click handler (for disabled state)
    document.getElementById('downloadButton')?.addEventListener('click', function(event) {
        if (this.classList.contains('disabled')) {
            event.preventDefault();
            alert("Please complete the previous steps first.");
        }
    });
});
