// Wizard state management
const wizardState = {
    currentStep: 1,
    device: null,     // 'adapter' or 'summit'
    model: null,      // 'basic_pcb', 'advanced_pcb', or 'non_pcb' (for adapter only)
    board: null,      // 'qtpy' or 'xiao' (for adapter only)
};

// CORS proxy for cross-origin firmware fetches (Arduino Micro WebSerial flow).
// The same Cloudflare Worker that serves Summit; generalized to accept a repo
// segment: /<repo>/<tag>/<file>. See tools/firmware-proxy/worker.js.
const ADAPTER_FIRMWARE_PROXY = 'https://vail-firmware-proxy.brett-hollifield.workers.dev';

// --- Adapter release/version selector ------------------------------------
// Mirrors the Summit ESP flasher: a version dropdown + "Show test release"
// checkbox driven by GitHub Releases. Pre-releases are surfaced as the test
// option. Per-version firmware comes from the release's attached assets,
// which the build workflow stamps with the tag (e.g. xiao_basic_pcb_v2_v5.0.uf2).
const adapterReleases = {
    stable: [],          // published, non-prerelease releases that carry firmware assets
    testRelease: null,   // most recent pre-release with firmware assets (the "test" build)
    selected: null,      // currently selected release, or null = repository fallback
    fetched: false,

    hasFirmware(release) {
        return release.assets && release.assets.some(a => /\.(uf2|hex)$/i.test(a.name));
    },

    async fetch() {
        if (this.fetched) return;
        this.fetched = true;
        try {
            const resp = await fetch('https://api.github.com/repos/Vail-CW/vail-adapter/releases?per_page=50');
            if (!resp.ok) { this.populate(); return; }
            const all = await resp.json();
            this.stable = all.filter(r => !r.prerelease && !r.draft && this.hasFirmware(r));
            const pre = all.filter(r => r.prerelease && !r.draft && this.hasFirmware(r));
            this.testRelease = pre.length ? pre[0] : null;
            this.populate();
            // Default to the latest stable release; fall back to the repository
            // firmware_files/ build if no release carries assets yet.
            this.select(this.stable[0] || null);
        } catch (err) {
            console.log('Error fetching adapter releases:', err.message);
            this.populate();
        }
    },

    populate() {
        const select = document.getElementById('adapterVersionSelect');
        if (!select) return;
        select.innerHTML = '';

        this.stable.forEach((release, index) => {
            const option = document.createElement('option');
            option.value = release.tag_name;
            const date = new Date(release.published_at).toLocaleDateString('en-US', {
                year: 'numeric', month: 'short', day: 'numeric'
            });
            const label = release.name || release.tag_name;
            option.textContent = `${label} (${date})${index === 0 ? ' — Latest' : ''}`;
            select.appendChild(option);
        });

        // Always offer the live repository build as a resilient fallback
        const fallback = document.createElement('option');
        fallback.value = '__fallback__';
        fallback.textContent = 'Latest (from repository)';
        select.appendChild(fallback);

        select.disabled = false;
        if (this.stable.length === 0) select.value = '__fallback__';

        select.onchange = () => {
            const tag = select.value;
            if (tag === '__test__') this.select(this.testRelease);
            else if (tag === '__fallback__') this.select(null);
            else this.select(this.stable.find(r => r.tag_name === tag) || null);
        };

        // Hide the "Show test release" checkbox entirely when there is no test build
        const checkbox = document.getElementById('adapterShowTestRelease');
        if (checkbox) {
            const container = checkbox.closest('.test-release-toggle');
            if (!this.testRelease && container) container.style.display = 'none';
            else if (container) container.style.display = '';
            checkbox.onchange = () => this.toggleTestRelease(checkbox.checked);
        }
    },

    toggleTestRelease(show) {
        const select = document.getElementById('adapterVersionSelect');
        const warning = document.getElementById('adapterTestReleaseWarning');
        if (!select) return;

        const existing = select.querySelector('option[value="__test__"]');
        if (existing) existing.remove();

        if (show && this.testRelease) {
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
            this.select(this.testRelease);
            if (warning) warning.style.display = 'block';
        } else {
            if (this.stable.length > 0) {
                select.value = this.stable[0].tag_name;
                this.select(this.stable[0]);
            } else {
                select.value = '__fallback__';
                this.select(null);
            }
            if (warning) warning.style.display = 'none';
        }
    },

    select(release) {
        this.selected = release;
        this.updateInfo(release);
        // Re-resolve the active download for the new version
        if (wizardState.currentStep === 3) updateStep3Content();
        if (wizardState.currentStep === 3.1) updateStep3MicroContent();
    },

    updateInfo(release) {
        const info = document.getElementById('adapterReleaseInfo');
        const badge = document.getElementById('adapterReleaseVersionBadge');
        const dateEl = document.getElementById('adapterReleaseDate');
        const notes = document.getElementById('adapterReleaseNotes');
        if (!info) return;

        if (!release) {
            // Repository fallback — no specific release metadata to show
            info.style.display = 'none';
            return;
        }
        info.style.display = 'block';
        if (badge) {
            badge.textContent = release.tag_name;
            badge.className = 'firmware-version' + (release.prerelease ? ' test-version' : '');
        }
        if (dateEl) {
            dateEl.textContent = new Date(release.published_at).toLocaleDateString('en-US', {
                year: 'numeric', month: 'long', day: 'numeric'
            });
        }
        if (notes) {
            const items = releaseBodyToItems(release.body);
            notes.innerHTML = items.length ? `<ul>${items.join('')}</ul>` : '';
        }
    },

    // Find the asset for a given base firmware name (e.g. "xiao_basic_pcb_v2")
    // within the selected release. Asset names are tag-stamped, so match the
    // base plus an optional "_<tag>" suffix.
    findAsset(base, ext) {
        if (!this.selected || !this.selected.assets) return null;
        const re = new RegExp('^' + base.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + '(_.+)?\\.' + ext + '$', 'i');
        return this.selected.assets.find(a => re.test(a.name)) || null;
    },
};

// Resolve the base firmware name (no extension) and extension for the current
// board/model selection. Returns null if the selection is incomplete.
function getFirmwareBase() {
    if (wizardState.model === 'vail_lite') return { base: 'trinkey_vail_adapter', ext: 'uf2' };
    if (wizardState.board === 'micro') return { base: 'arduino_micro', ext: 'hex' };
    if (!wizardState.model || !wizardState.board) return null;
    if (wizardState.model === 'basic_pcb') return { base: `${wizardState.board}_basic_pcb_v2`, ext: 'uf2' };
    if (wizardState.model === 'advanced_pcb') return { base: `${wizardState.board}_advanced_pcb`, ext: 'uf2' };
    return { base: `${wizardState.board}_non_pcb`, ext: 'uf2' };
}

// Resolve the firmware download for the current selection + selected version.
// Prefers the selected release's tag-stamped asset; falls back to the live
// repository build (docs/firmware_files/) when no release/asset is available.
// .hex (Arduino Micro) is fetched in JS, so its asset URL is routed through the
// CORS proxy; .uf2 is a direct anchor download and needs no proxy.
function getFirmwareFile() {
    const sel = getFirmwareBase();
    if (!sel) return null;
    const { base, ext } = sel;
    const plain = `${base}.${ext}`;

    // "Latest (from repository)" fallback option — serve the live repo build.
    if (!adapterReleases.selected) {
        return { url: `firmware_files/${plain}`, filename: plain, fallbackUrl: `firmware_files/${plain}` };
    }

    // A specific release is selected: require its matching asset. Don't silently
    // serve the latest build — that would mislabel an old version's download.
    const asset = adapterReleases.findAsset(base, ext);
    if (!asset) {
        return { unavailable: true, version: adapterReleases.selected.tag_name, board: plain };
    }
    const url = ext === 'hex'
        ? `${ADAPTER_FIRMWARE_PROXY}/vail-adapter/${adapterReleases.selected.tag_name}/${asset.name}`
        : asset.browser_download_url;
    return { url, filename: asset.name, fallbackUrl: `firmware_files/${plain}` };
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
        'xiao': 'Seeeduino XIAO SAMD21',
        'micro': 'Arduino Micro (experimental)'
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

    // Handle step 3.1 (Arduino Micro WebSerial flasher)
    if (stepNumber === 3.1) {
        targetStepId = 'step3_micro';
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

        // Update Micro step content if navigating there
        if (stepNumber === 3.1) {
            updateStep3MicroContent();
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
    // The Arduino Micro is an experimental DIY/breadboard target, so its board
    // card only appears on the "DIY No PCB" path.
    const microCard = document.getElementById('microCard');
    if (microCard) {
        microCard.style.display = wizardState.model === 'non_pcb' ? '' : 'none';
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

function updateStep3MicroContent() {
    // Display the selected config on the Micro flash page
    const configEl = document.getElementById('selectedConfigMicro');
    if (configEl) {
        const modelLabel = getModelName(wizardState.model) || 'DIY';
        configEl.textContent = `${modelLabel} + ${getBoardName('micro')}`;
    }
}

// --- Arduino Micro WebSerial flasher state ---
const microFlasher = {
    appPort: null,          // running-application serial port (pre-touch)
    lastLoggedLine: '',
};

function microLog(message) {
    console.log('[micro]', message);
    const logArea = document.getElementById('microFlashLog');
    if (logArea) {
        logArea.textContent += message + '\n';
        logArea.scrollTop = logArea.scrollHeight;
    }
}

function microSerialLog(message) {
    console.log('[micro-boot]', message);
    const logArea = document.getElementById('microSerialLog');
    if (logArea) {
        logArea.textContent += message + '\n';
        logArea.scrollTop = logArea.scrollHeight;
    }
}

async function triggerMicroBootloader() {
    if (!('serial' in navigator)) {
        alert('WebSerial API not supported. Please use Chrome, Edge, or Opera.');
        return;
    }

    try {
        microSerialLog('Requesting the Arduino Micro port…');
        const port = await navigator.serial.requestPort();
        microSerialLog('Port selected. Performing 1200-baud touch…');
        await window.avr109Touch1200(port);
        microSerialLog('✅ Touch sent. The Micro should now re-enumerate as the Caterina bootloader for ~8 seconds.');
        microSerialLog('Proceed to step 3 and select the NEW port (different COM/ttyACM number).');
    } catch (err) {
        microSerialLog(`❌ ${err.message}`);
        if (err.name !== 'NotFoundError') {
            alert(`Bootloader touch failed: ${err.message}`);
        }
    }
}

async function flashMicroFirmware() {
    if (!('serial' in navigator)) {
        alert('WebSerial API not supported. Please use Chrome, Edge, or Opera.');
        return;
    }

    const firmware = getFirmwareFile();
    if (!firmware) {
        alert('No firmware file available for this configuration.');
        return;
    }
    if (firmware.unavailable) {
        alert(`Arduino Micro firmware is not available in ${firmware.version}. Choose a newer version.`);
        return;
    }

    const progressContainer = document.getElementById('microProgressContainer');
    const progressBar = document.getElementById('microProgressBar');
    const progressLabel = document.getElementById('microProgressLabel');
    const progressPercent = document.getElementById('microProgressPercent');

    let port = null;
    let flasher = null;

    try {
        microLog(`Downloading ${firmware.filename} from ${firmware.url}…`);
        let resp = await fetch(firmware.url, { cache: 'no-cache' }).catch(() => null);
        // If the proxied release asset is unavailable (proxy down, asset/tag
        // missing), fall back to the live repository build so flashing still works.
        if ((!resp || !resp.ok) && firmware.fallbackUrl && firmware.fallbackUrl !== firmware.url) {
            microLog('Release asset unavailable — falling back to the latest repository build…');
            resp = await fetch(firmware.fallbackUrl, { cache: 'no-cache' }).catch(() => null);
        }
        if (!resp || !resp.ok) {
            throw new Error(`Firmware fetch failed: HTTP ${resp ? resp.status : 'network error'}`);
        }
        const hexText = await resp.text();
        microLog(`Firmware fetched (${hexText.length} chars of Intel HEX).`);

        microLog('Select the BOOTLOADER port (different from your running-app port).');
        port = await navigator.serial.requestPort();

        progressContainer.style.display = 'block';
        progressLabel.textContent = 'Opening bootloader port…';

        flasher = new window.AVR109Flasher({
            log: microLog,
            progress: (current, total) => {
                const pct = Math.round((current / total) * 100);
                progressBar.style.width = pct + '%';
                progressPercent.textContent = `${pct}% (block ${current}/${total})`;
                progressLabel.textContent = 'Writing firmware…';
            },
        });

        await flasher.openPort(port, 57600);
        await flasher.flashHex(hexText);

        progressLabel.textContent = '✅ Done';
        progressPercent.textContent = '100%';
        progressBar.style.width = '100%';
        alert('Firmware flashed successfully! The Micro will restart running the Vail Adapter firmware.');
    } catch (err) {
        microLog(`❌ Flash failed: ${err.message}`);
        progressLabel.textContent = '❌ Error';
        alert(`Flash failed: ${err.message}`);
    } finally {
        if (flasher) {
            try { await flasher.close(); } catch (e) { /* ignore */ }
        }
    }
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

    if (firmwareFile && firmwareFile.unavailable) {
        // The selected release predates this board / has no matching asset
        downloadButton.removeAttribute('href');
        downloadButton.removeAttribute('download');
        downloadButton.classList.add('disabled');
        downloadButton.setAttribute('aria-disabled', 'true');
        downloadText.textContent = `${firmwareFile.board} not available in ${firmwareFile.version}`;
    } else if (firmwareFile) {
        const savedName = firmwareFile.filename;
        downloadButton.href = firmwareFile.url;
        downloadButton.download = savedName;
        downloadText.textContent = `Download ${savedName}`;
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

// Escape HTML special characters so release-note text can't inject markup
function escapeHtml(s) {
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

// Strip the small subset of Markdown that shows up in release notes
function stripMarkdown(s) {
    return s
        .replace(/\[([^\]]+)\]\([^)]+\)/g, '$1') // [text](url) -> text
        .replace(/\*\*([^*]+)\*\*/g, '$1')        // **bold** -> bold
        .replace(/`([^`]+)`/g, '$1')              // `code` -> code
        .trim();
}

// Turn a release-note body (Markdown) into <li> items for the What's New list
function releaseBodyToItems(body) {
    const items = [];
    for (const raw of (body || '').split(/\r?\n/)) {
        const line = raw.trim();
        if (!line) continue;
        // Skip the "Go to https://update.vailadapter.com/" promo lines
        if (/^go to\s+https?:\/\//i.test(line) || /update\.vailadapter\.com/i.test(line)) continue;

        const heading = line.match(/^#{1,6}\s+(.*)$/);
        if (heading) {
            const text = escapeHtml(stripMarkdown(heading[1]));
            items.push(`<li style="list-style:none;margin-left:-20px;font-weight:600;">${text}</li>`);
            continue;
        }

        const bullet = line.match(/^[-*]\s+(.*)$/);
        const text = escapeHtml(stripMarkdown(bullet ? bullet[1] : line));
        if (text) items.push(`<li>${text}</li>`);
    }
    return items;
}

// Populate the "What's New" section from the latest GitHub Release for the
// selected device. Both repos publish firmware as GitHub Releases; if a repo
// has no published release yet (e.g. Summit's official channel), the section
// is hidden rather than falling back to raw commit history.
// deviceType: 'adapter' or 'summit'
async function fetchRecentUpdates(deviceType) {
    const repoName = deviceType === 'summit' ? 'vail-summit' : 'vail-adapter';
    const deviceLabel = deviceType === 'summit' ? 'Vail Summit' : 'Vail Adapter';

    const section = document.getElementById('whatsNewSection');
    const dateElement = document.getElementById('lastUpdateDate');
    const listElement = document.getElementById('recentCommitsList');

    // Update the global page title to match the selected device
    const pageTitle = document.getElementById('pageTitle');
    if (pageTitle) {
        pageTitle.textContent = `${deviceLabel} Firmware Update`;
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

    // Hide the "full release notes" link until this fetch resolves with a URL
    const releaseNotesLink = document.getElementById('releaseNotesLink');
    if (releaseNotesLink) releaseNotesLink.style.display = 'none';

    // Show the section in a loading state, clearing any stale content from a
    // previously selected device so the other device's notes never show through
    if (section) section.style.display = 'block';
    if (dateElement) dateElement.textContent = 'Loading...';
    if (listElement) listElement.innerHTML = '<li>Loading latest release...</li>';

    try {
        const response = await fetch(`https://api.github.com/repos/Vail-CW/${repoName}/releases/latest`);
        if (response.status === 404) {
            // No published release for this device yet — hide the section
            if (section) section.style.display = 'none';
            return;
        }
        if (!response.ok) {
            console.log('Could not fetch latest release');
            if (section) section.style.display = 'none';
            return;
        }
        const release = await response.json();

        // Header: "<Version> — <date>", e.g. "V4.4 — October 3, 2025"
        const versionLabel = release.name || release.tag_name || '';
        const publishedDate = new Date(release.published_at);
        const dateStr = publishedDate.toLocaleDateString('en-US', {
            year: 'numeric',
            month: 'long',
            day: 'numeric'
        });
        if (dateElement) {
            dateElement.textContent = versionLabel ? `${versionLabel} — ${dateStr}` : dateStr;
        }

        // Body: render the release notes as a bullet list
        if (listElement) {
            const items = releaseBodyToItems(release.body);
            listElement.innerHTML = items.length
                ? items.join('')
                : '<li>See the release notes on GitHub for details.</li>';
        }

        // Link to the full release notes on GitHub (nothing is silently cut off,
        // but this is the canonical source for the complete changelog)
        if (releaseNotesLink && release.html_url) {
            const anchor = releaseNotesLink.querySelector('a');
            if (anchor) anchor.href = release.html_url;
            releaseNotesLink.style.display = 'block';
        }
    } catch (err) {
        console.log('Error fetching latest release:', err.message);
        if (section) section.style.display = 'none';
    }
}

// Hide the What's New section and reset the page title
function hideWhatsNew() {
    const section = document.getElementById('whatsNewSection');
    if (section) {
        section.style.display = 'none';
    }
    const pageTitle = document.getElementById('pageTitle');
    if (pageTitle) {
        pageTitle.textContent = 'Vail Firmware Update';
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
                    adapterReleases.fetch();
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

            // Arduino Micro uses a different flash flow (WebSerial AVR109, not UF2)
            const nextStep = wizardState.board === 'micro' ? 3.1 : 3;
            setTimeout(() => {
                goToStep(nextStep);
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

    // Arduino Micro (WebSerial AVR109) buttons
    document.getElementById('microBootModeButton')?.addEventListener('click', triggerMicroBootloader);
    document.getElementById('microFlashButton')?.addEventListener('click', flashMicroFirmware);
    document.getElementById('backToStep2FromMicro')?.addEventListener('click', () => goToStep(2));
    document.getElementById('startOverFromMicro')?.addEventListener('click', () => {
        wizardState.device = null;
        wizardState.model = null;
        wizardState.board = null;
        document.querySelectorAll('.selection-card').forEach(card => {
            card.classList.remove('selected');
        });
        hideWhatsNew();
        goToStep(1);
    });

    // Download button click handler (for disabled state)
    document.getElementById('downloadButton')?.addEventListener('click', function(event) {
        if (this.classList.contains('disabled')) {
            event.preventDefault();
            alert("Please complete the previous steps first.");
        }
    });
});
