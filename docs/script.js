// Wizard state management
const wizardState = {
    currentStep: 1,
    device: null,     // 'adapter' or 'summit'
    model: null,      // 'basic_pcb', 'advanced_pcb', or 'non_pcb' (for adapter only)
    board: null,      // 'qtpy' or 'xiao' (for adapter only)
};

// Firmware file mapping
function getFirmwareFile() {
    // Vail Lite only has one firmware variant (Trinkey)
    if (wizardState.model === 'vail_lite') {
        return {
            url: 'firmware_files/trinkey_vail_adapter.uf2',
            filename: 'trinkey_vail_adapter.uf2'
        };
    }

    // Other models require board selection
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
        'vail_lite': 'Vail Lite',
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
    let targetStepId = `step${stepNumber}`;

    // Handle step 1.5 (adapter model selection)
    if (stepNumber === 1.5) {
        targetStepId = 'step1_5';
    }

    const targetStep = document.getElementById(targetStepId);
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

        // Initialize ESP flasher if navigating to step 4 (Summit)
        if (stepNumber === 4) {
            // Wait a moment for the DOM to update
            setTimeout(() => {
                if (typeof window.initializeESPFlasher === 'function') {
                    window.initializeESPFlasher();
                }
            }, 100);
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
    let configText;
    if (wizardState.model === 'vail_lite') {
        // Vail Lite only has one variant, no need to show board
        configText = getModelName(wizardState.model);
    } else {
        configText = `${getModelName(wizardState.model)} + ${getBoardName(wizardState.board)}`;
    }
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

// Fetch recent commits from GitHub for "What's New" section
// deviceType: 'adapter' or 'summit'
async function fetchRecentUpdates(deviceType) {
    const repoName = deviceType === 'summit' ? 'vail-summit' : 'vail-adapter';
    const deviceLabel = deviceType === 'summit' ? 'Vail Summit' : 'Vail Adapter';

    // Show the section
    const section = document.getElementById('whatsNewSection');
    if (section) {
        section.style.display = 'block';
    }

    // Update the device name in the header
    const deviceElement = document.getElementById('whatsNewDevice');
    if (deviceElement) {
        deviceElement.textContent = deviceLabel;
    }

    // Update manual link (hide for Summit since it doesn't have a manual page)
    const manualLink = document.getElementById('manualLink');
    if (manualLink) {
        manualLink.style.display = deviceType === 'adapter' ? 'block' : 'none';
    }

    // Show/hide Button Hat warning (only for Adapter)
    const buttonHatWarning = document.getElementById('buttonHatWarning');
    if (buttonHatWarning) {
        buttonHatWarning.style.display = deviceType === 'adapter' ? 'block' : 'none';
    }

    try {
        const response = await fetch(`https://api.github.com/repos/Vail-CW/${repoName}/commits?per_page=20`);
        if (!response.ok) {
            console.log('Could not fetch commits');
            return;
        }
        const commits = await response.json();

        // Filter out automated/CI commits and get meaningful ones
        const meaningfulCommits = commits.filter(commit => {
            const message = commit.commit.message.toLowerCase();
            // Skip CI commits, merge commits, and minor fixes
            if (message.includes('[skip ci]') ||
                message.includes('merge pull request') ||
                message.includes('merge branch') ||
                message.startsWith('update summit firmware from') ||
                message.startsWith('co-authored-by:')) {
                return false;
            }

            // For adapter repo, also skip website/updater-related commits
            if (deviceType === 'adapter') {
                if (message.includes('favicon') ||
                    message.includes('updater') ||
                    message.includes("what's new") ||
                    message.includes('whats new') ||
                    message.includes('button hat warning') ||
                    message.includes('esp flasher') ||
                    message.includes('esp32 flasher') ||
                    message.includes('summit flasher') ||
                    message.includes('web flasher') ||
                    message.includes('github pages') ||
                    message.includes('website')) {
                    return false;
                }
            }

            return true;
        });

        if (meaningfulCommits.length === 0) {
            return;
        }

        // Get the most recent commit date
        const latestDate = new Date(meaningfulCommits[0].commit.author.date);
        const dateStr = latestDate.toLocaleDateString('en-US', {
            year: 'numeric',
            month: 'long',
            day: 'numeric'
        });

        // Update the date display
        const dateElement = document.getElementById('lastUpdateDate');
        if (dateElement) {
            dateElement.textContent = dateStr;
        }

        // Build the commit list (show up to 5 recent meaningful commits)
        const listElement = document.getElementById('recentCommitsList');
        if (listElement) {
            const items = meaningfulCommits.slice(0, 5).map(commit => {
                // Get first line of commit message
                const message = commit.commit.message.split('\n')[0];
                // Truncate if too long
                const displayMsg = message.length > 100 ? message.substring(0, 100) + '...' : message;
                return `<li>${displayMsg}</li>`;
            });
            listElement.innerHTML = items.join('');
        }
    } catch (err) {
        console.log('Error fetching commits:', err.message);
        // Leave the loading text, it will just show "Loading..."
    }
}

// Hide the What's New section and Button Hat warning
function hideWhatsNew() {
    const section = document.getElementById('whatsNewSection');
    if (section) {
        section.style.display = 'none';
    }
    const buttonHatWarning = document.getElementById('buttonHatWarning');
    if (buttonHatWarning) {
        buttonHatWarning.style.display = 'none';
    }
}

// Initialize wizard on page load
document.addEventListener('DOMContentLoaded', () => {
    // Step 1: Device selection (Adapter vs Summit)
    document.querySelectorAll('#step1 .selection-card').forEach(card => {
        card.addEventListener('click', () => {
            // Remove selected state from all cards
            document.querySelectorAll('#step1 .selection-card').forEach(c => {
                c.classList.remove('selected');
            });

            // Mark this card as selected
            card.classList.add('selected');
            wizardState.device = card.dataset.device;

            // Navigate based on device type
            setTimeout(() => {
                if (wizardState.device === 'adapter') {
                    // Go to adapter model selection (step 1.5)
                    goToStep(1.5);
                    fetchRecentUpdates('adapter');
                } else if (wizardState.device === 'summit') {
                    // Go directly to Summit flash page (step 4)
                    goToStep(4);
                    fetchRecentUpdates('summit');
                }
            }, 300);
        });
    });

    // Step 1.5: Adapter Model selection
    document.querySelectorAll('#step1_5 .selection-card').forEach(card => {
        card.addEventListener('click', () => {
            // Remove selected state from all cards
            document.querySelectorAll('#step1_5 .selection-card').forEach(c => {
                c.classList.remove('selected');
            });

            // Mark this card as selected
            card.classList.add('selected');
            wizardState.model = card.dataset.model;

            // Vail Lite skips board selection (only one hardware variant)
            if (wizardState.model === 'vail_lite') {
                wizardState.board = 'trinkey'; // Set board for internal tracking
                setTimeout(() => {
                    goToStep(3); // Go directly to firmware flash step
                }, 300);
            } else {
                // Other models need board selection
                setTimeout(() => {
                    goToStep(2);
                }, 300);
            }
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
    document.getElementById('backToStep1FromModel')?.addEventListener('click', () => {
        hideWhatsNew();
        goToStep(1);
    });

    document.getElementById('backToStep1')?.addEventListener('click', () => {
        goToStep(1.5);
    });

    document.getElementById('backToStep2')?.addEventListener('click', () => {
        // If user selected Vail Lite, skip board selection step when going back
        if (wizardState.model === 'vail_lite') {
            goToStep(1.5); // Go back to model selection
        } else {
            goToStep(2); // Go back to board selection
        }
    });

    document.getElementById('backToStep1FromSummit')?.addEventListener('click', () => {
        hideWhatsNew();
        goToStep(1);
    });

    // Start over buttons
    document.getElementById('startOver')?.addEventListener('click', () => {
        wizardState.device = null;
        wizardState.model = null;
        wizardState.board = null;
        document.querySelectorAll('.selection-card').forEach(card => {
            card.classList.remove('selected');
        });
        hideWhatsNew();
        goToStep(1);
    });

    document.getElementById('startOverFromSummit')?.addEventListener('click', () => {
        wizardState.device = null;
        wizardState.model = null;
        wizardState.board = null;
        document.querySelectorAll('.selection-card').forEach(card => {
            card.classList.remove('selected');
        });
        hideWhatsNew();
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
