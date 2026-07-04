// Vail firmware updater — wizard flow + one-click WebSerial flashing.
//
// Port handling philosophy: the user should pick their adapter at most ONCE.
// Web Serial permissions persist per USB device (vendor/product/serial), so
// every port we've ever been granted comes back via navigator.serial.getPorts().
// The app firmware and the bootloader enumerate as different USB devices
// (Adafruit/Seeed/Arduino convention: app PID has the 0x8000 bit set, the
// bootloader PID is the same value without it), so we classify granted ports
// by their USB IDs and only fall back to the browser picker when we've never
// seen the device before.

// Wizard state
const wizardState = {
    step: 'device',
    device: null,     // 'adapter' | 'summit'
    model: null,      // 'basic_pcb' | 'advanced_pcb' | 'vail_lite' | 'non_pcb'
    board: null,      // 'qtpy' | 'xiao' | 'micro' | 'trinkey'
    flashMethod: 'serial', // 'serial' | 'uf2'
};

const STORAGE_KEY = 'vailUpdaterSetup';

// CORS proxy for cross-origin firmware fetches. The same Cloudflare Worker that
// serves Summit; generalized to accept a repo segment: /<repo>/<tag>/<file>.
const ADAPTER_FIRMWARE_PROXY = 'https://vail-firmware-proxy.brett-hollifield.workers.dev';

// Only surface releases at or above this version. Earlier releases used a
// different firmware layout/board set and shouldn't be offered for flashing.
const MIN_ADAPTER_VERSION = 5.0;

const webSerialSupported = ('serial' in navigator);

// --- Adapter release/version selector ---------------------------------------

const adapterReleases = {
    stable: [],
    testRelease: null,
    selected: null,
    fetched: false,

    hasFirmware(release) {
        return release.assets && release.assets.some(a => /\.(uf2|hex)$/i.test(a.name));
    },

    parseVersion(tag) {
        const m = String(tag || '').match(/(\d+(?:\.\d+)?)/);
        return m ? parseFloat(m[1]) : 0;
    },

    eligible(release) {
        return !release.draft &&
            this.hasFirmware(release) &&
            this.parseVersion(release.tag_name) >= MIN_ADAPTER_VERSION;
    },

    async fetch() {
        if (this.fetched) return;
        this.fetched = true;
        try {
            const resp = await fetch('https://api.github.com/repos/Vail-CW/vail-adapter/releases?per_page=50');
            if (!resp.ok) { this.populate(); return; }
            const all = await resp.json();
            this.stable = all.filter(r => !r.prerelease && this.eligible(r));
            const pre = all.filter(r => r.prerelease && this.eligible(r));
            this.testRelease = pre.length ? pre[0] : null;
            this.populate();
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

        if (this.stable.length === 0 && !this.testRelease) {
            const none = document.createElement('option');
            none.value = '';
            none.textContent = 'No releases available';
            select.appendChild(none);
        }

        select.disabled = false;

        select.onchange = () => {
            const tag = select.value;
            if (tag === '__test__') this.select(this.testRelease);
            else this.select(this.stable.find(r => r.tag_name === tag) || null);
        };

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
                this.select(null);
            }
            if (warning) warning.style.display = 'none';
        }
    },

    select(release) {
        this.selected = release;
        this.updateInfo(release);
        if (wizardState.step === 'update') updateUpdateScreen();
    },

    updateInfo(release) {
        const details = document.getElementById('adapterReleaseDetails');
        const dateEl = document.getElementById('adapterReleaseDate');
        const notes = document.getElementById('adapterReleaseNotes');
        if (!details) return;

        if (!release) {
            details.style.display = 'none';
            return;
        }
        details.style.display = 'block';
        if (dateEl) {
            dateEl.textContent = new Date(release.published_at).toLocaleDateString('en-US', {
                year: 'numeric', month: 'long', day: 'numeric'
            });
        }
        if (notes) {
            const items = releaseBodyToItems(release.body);
            notes.innerHTML = items.length ? `<ul>${items.join('')}</ul>` : '<p class="no-notes">No notes for this release.</p>';
        }
    },

    // Find the asset for a given base firmware name within the selected release.
    // Asset names are tag-stamped, so match the base plus an optional "_<tag>".
    findAsset(base, ext) {
        if (!this.selected || !this.selected.assets) return null;
        const re = new RegExp('^' + base.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + '(_.+)?\\.' + ext + '$', 'i');
        return this.selected.assets.find(a => re.test(a.name)) || null;
    },
};

// Resolve the base firmware name (no extension) and extension for the current
// board/model selection. `base` is the short 8.3-safe asset code; `legacyBase`
// keeps older, not-yet-restamped releases working.
function getFirmwareBase() {
    if (wizardState.model === 'vail_lite') return { base: 'vl', legacyBase: 'trinkey_vail_adapter', ext: 'uf2' };
    if (wizardState.board === 'micro') return { base: 'mic', legacyBase: 'arduino_micro', ext: 'hex' };
    if (!wizardState.model || !wizardState.board) return null;
    const p = wizardState.board === 'xiao' ? 'x' : 'q';
    if (wizardState.model === 'basic_pcb') return { base: `${p}b2`, legacyBase: `${wizardState.board}_basic_pcb_v2`, ext: 'uf2' };
    if (wizardState.model === 'advanced_pcb') return { base: `${p}ad`, legacyBase: `${wizardState.board}_advanced_pcb`, ext: 'uf2' };
    return { base: `${p}np`, legacyBase: `${wizardState.board}_non_pcb`, ext: 'uf2' };
}

// Resolve the firmware download for the current selection + selected version.
function getFirmwareFile() {
    const sel = getFirmwareBase();
    if (!sel) return null;
    const { base, legacyBase, ext } = sel;

    if (!adapterReleases.selected) {
        return { unavailable: true, version: '(none)', board: `${base}.${ext}` };
    }
    const asset = adapterReleases.findAsset(base, ext) ||
        (legacyBase ? adapterReleases.findAsset(legacyBase, ext) : null);
    if (!asset) {
        return { unavailable: true, version: adapterReleases.selected.tag_name, board: `${base}.${ext}` };
    }
    const url = ext === 'hex'
        ? `${ADAPTER_FIRMWARE_PROXY}/vail-adapter/${adapterReleases.selected.tag_name}/${asset.name}`
        : asset.browser_download_url;
    return { url, filename: asset.name, ext };
}

// Friendly names
function getModelName(model) {
    return ({
        basic_pcb: 'Basic PCB',
        advanced_pcb: 'Advanced PCB',
        vail_lite: 'Vail Lite',
        non_pcb: 'DIY No PCB',
    })[model] || model;
}

function getBoardName(board) {
    return ({
        qtpy: 'QT Py',
        xiao: 'XIAO',
        micro: 'Arduino Micro',
        trinkey: 'Trinkey',
    })[board] || board;
}

function getConfigText() {
    if (wizardState.model === 'vail_lite') return getModelName(wizardState.model);
    if (wizardState.device === 'summit') return 'Vail Summit';
    return `${getModelName(wizardState.model)} · ${getBoardName(wizardState.board)}`;
}

function getSelectedVersionLabel() {
    const r = adapterReleases.selected;
    return r ? (r.name || r.tag_name) : 'the latest release';
}

// --- Step navigation ---------------------------------------------------------

const STEP_SECTIONS = {
    device: 'step1',
    model: 'step1_5',
    board: 'step2',
    update: 'stepUpdate',
    summit: 'step4',
};

function goToStep(step) {
    document.querySelectorAll('.wizard-step').forEach(s => s.classList.remove('active'));
    const section = document.getElementById(STEP_SECTIONS[step]);
    if (!section) return;
    section.classList.add('active');
    wizardState.step = step;

    if (step === 'board') updateBoardCards();
    if (step === 'update') updateUpdateScreen();
    if (step === 'summit') {
        setTimeout(() => {
            if (typeof window.initializeESPFlasher === 'function') window.initializeESPFlasher();
        }, 100);
    }
    renderCrumbs();
    updateHeader();
}

function updateHeader() {
    const sub = document.getElementById('pageSub');
    if (sub) sub.style.display = wizardState.step === 'device' ? '' : 'none';
}

function updateBoardCards() {
    const qtpyHint = document.getElementById('qtpyHint');
    if (qtpyHint) qtpyHint.style.display = wizardState.model === 'non_pcb' ? 'none' : '';
    // The Arduino Micro is an experimental DIY target: only on the No-PCB path.
    const microCard = document.getElementById('microCard');
    if (microCard) microCard.style.display = wizardState.model === 'non_pcb' ? '' : 'none';
}

// Breadcrumb chips: one per decision already made, click to change it.
function renderCrumbs() {
    const bar = document.getElementById('crumbBar');
    if (!bar) return;
    const crumbs = [];

    if (wizardState.device) {
        crumbs.push({ label: wizardState.device === 'summit' ? 'Vail Summit' : 'Vail Adapter', step: 'device' });
    }
    if (wizardState.device === 'adapter' && wizardState.model) {
        crumbs.push({ label: getModelName(wizardState.model), step: 'model' });
    }
    if (wizardState.device === 'adapter' && wizardState.board && wizardState.model !== 'vail_lite') {
        crumbs.push({ label: getBoardName(wizardState.board), step: 'board' });
    }

    if (!crumbs.length || wizardState.step === 'device') {
        bar.style.display = 'none';
        return;
    }

    bar.style.display = '';
    bar.innerHTML = '';
    crumbs.forEach(c => {
        const chip = document.createElement('button');
        chip.className = 'crumb-chip';
        chip.innerHTML = `${escapeHtml(c.label)} <span class="crumb-edit">change</span>`;
        chip.addEventListener('click', () => goToStep(c.step));
        bar.appendChild(chip);
    });
}

function saveSetup() {
    try {
        localStorage.setItem(STORAGE_KEY, JSON.stringify({
            device: wizardState.device,
            model: wizardState.model,
            board: wizardState.board,
            flashMethod: wizardState.flashMethod,
        }));
    } catch (_) { /* private mode etc. */ }
}

function loadSetup() {
    try {
        const raw = localStorage.getItem(STORAGE_KEY);
        if (!raw) return null;
        const s = JSON.parse(raw);
        if (s.device === 'summit') return s;
        if (s.device === 'adapter' && s.model) return s;
        return null;
    } catch (_) { return null; }
}

// --- Update screen -------------------------------------------------------------

function setMethod(method) {
    wizardState.flashMethod = method;
    document.getElementById('tabSerial')?.classList.toggle('active', method === 'serial');
    document.getElementById('tabUf2')?.classList.toggle('active', method === 'uf2');
    document.getElementById('methodSerialPanel')?.classList.toggle('active', method === 'serial');
    document.getElementById('methodUf2Panel')?.classList.toggle('active', method === 'uf2');
    saveSetup();
}

function updateUpdateScreen() {
    const isMicro = wizardState.board === 'micro';

    // Arduino Micro flashes over serial only (Intel HEX, no UF2 bootloader).
    const tabUf2 = document.getElementById('tabUf2');
    if (tabUf2) tabUf2.style.display = isMicro ? 'none' : '';
    const toggle = document.getElementById('methodToggle');
    if (toggle) toggle.style.display = isMicro ? 'none' : '';

    if (isMicro) {
        setMethod('serial');
    } else if (!webSerialSupported && wizardState.flashMethod === 'serial') {
        setMethod('uf2');
    } else {
        setMethod(wizardState.flashMethod || (webSerialSupported ? 'serial' : 'uf2'));
    }

    // Browser-support note
    const note = document.getElementById('noWebSerialNote');
    if (note) note.style.display = webSerialSupported ? 'none' : '';
    const heroBtn = document.getElementById('flashNowButton');
    if (heroBtn) heroBtn.disabled = !webSerialSupported;

    // Hero copy
    const heroText = document.getElementById('flashHeroText');
    if (heroText) {
        heroText.textContent =
            `Plug in your adapter and click once. This installs ${getSelectedVersionLabel()} on your ${getConfigText()}. ` +
            `The first time, your browser asks which device to use. After that it's fully automatic.`;
    }

    // UF2 download link
    updateDownloadButton();

    // Test-only erase tool
    maybeRevealEraseTest();
}

function updateDownloadButton() {
    const downloadButton = document.getElementById('downloadButton');
    const downloadText = document.getElementById('downloadText');
    if (!downloadButton) return;
    const firmwareFile = getFirmwareFile();

    if (!firmwareFile || firmwareFile.unavailable) {
        downloadButton.removeAttribute('href');
        downloadButton.removeAttribute('download');
        downloadButton.classList.add('disabled');
        downloadButton.setAttribute('aria-disabled', 'true');
        downloadText.textContent = firmwareFile
            ? `Not available in ${firmwareFile.version}`
            : 'Download UF2 file';
    } else {
        downloadButton.href = firmwareFile.url;
        downloadButton.download = firmwareFile.filename;
        downloadText.textContent = `Download ${firmwareFile.filename}`;
        downloadButton.classList.remove('disabled');
        downloadButton.removeAttribute('aria-disabled');
    }
}

// --- Port intelligence ---------------------------------------------------------
//
// Adafruit, Seeed, and Arduino all follow the same convention: the application
// runs on PID 0x80xx and the bootloader on the matching 0x00xx. That lets us
// tell "running adapter" from "bootloader" without opening anything.

const KNOWN_VENDORS = [
    0x239A, // Adafruit (QT Py, Trinkey / Vail Lite)
    0x2886, // Seeed (XIAO)
    0x2341, // Arduino (Micro)
    0x2A03, // Arduino.org (older Micro clones)
    0x1B4F, // SparkFun (32U4 clones)
];

// Vendor filters for the browser's port picker, narrowed to the selected board
// so the list doesn't fill up with Bluetooth COM ports and other serial junk.
// The unfiltered list stays available via "Pick port manually".
function boardFilters() {
    const vendors = ({
        qtpy: [0x239A],
        trinkey: [0x239A],
        xiao: [0x2886],
        micro: [0x2341, 0x2A03, 0x1B4F],
    })[wizardState.board] || KNOWN_VENDORS;
    return vendors.map(v => ({ usbVendorId: v }));
}

function classifyPort(port) {
    try {
        const info = port.getInfo();
        if (!info || !info.usbVendorId) return 'unknown';
        if (!KNOWN_VENDORS.includes(info.usbVendorId)) return 'unknown';
        return (info.usbProductId & 0x8000) ? 'app' : 'bootloader';
    } catch (_) { return 'unknown'; }
}

function portLabel(port) {
    try {
        const i = port.getInfo();
        if (i && i.usbVendorId) {
            return `USB ${i.usbVendorId.toString(16).padStart(4, '0')}:${(i.usbProductId || 0).toString(16).padStart(4, '0')}`;
        }
    } catch (_) {}
    return 'serial device';
}

// Open at 1200 baud and close: the "magic touch" that asks the app to reboot
// into its bootloader. Works for SAMD21 (uf2-samdx1) and ATmega32U4 (Caterina).
async function touch1200(port) {
    try { await port.close(); } catch (_) { /* wasn't open */ }
    await port.open({ baudRate: 1200 });
    await new Promise(r => setTimeout(r, 100));
    await port.close();
}

// After a 1200-baud touch the board re-enumerates as a different USB device.
// If we've EVER been granted that bootloader before, it shows up in getPorts()
// on its own — no picker. Poll for it, and also listen for the connect event.
async function waitForBootloaderPort(previousPorts, timeoutMs) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
        const ports = await navigator.serial.getPorts();
        const found = ports.find(p => classifyPort(p) === 'bootloader') ||
            ports.find(p => !previousPorts.includes(p));
        if (found) return found;
        await new Promise(r => setTimeout(r, 300));
    }
    return null;
}

// --- One-click flash engine ------------------------------------------------------

const flashUI = {
    stages: ['find', 'boot', 'write', 'done'],

    reset() {
        const tl = document.getElementById('flashTimeline');
        if (tl) {
            tl.style.display = 'none';
            tl.querySelectorAll('li').forEach(li => li.classList.remove('active', 'done', 'error'));
        }
        this.progress(0, 0);
        const track = document.getElementById('serialProgressTrack');
        if (track) track.style.display = 'none';
        const res = document.getElementById('flashResult');
        if (res) { res.style.display = 'none'; res.innerHTML = ''; res.classList.remove('ok', 'err'); }
        this.hint('');
    },

    stage(name, state) {
        const tl = document.getElementById('flashTimeline');
        if (!tl) return;
        tl.style.display = '';
        const idx = this.stages.indexOf(name);
        tl.querySelectorAll('li').forEach(li => {
            const i = this.stages.indexOf(li.dataset.stage);
            li.classList.remove('active', 'error');
            if (i < idx) li.classList.add('done');
            if (i === idx) li.classList.add(state === 'error' ? 'error' : 'active');
            if (i === idx && state === 'done') { li.classList.remove('active'); li.classList.add('done'); }
        });
    },

    progress(cur, total) {
        const bar = document.getElementById('serialFlashProgressBar');
        const track = document.getElementById('serialProgressTrack');
        if (!bar) return;
        if (total > 0) {
            if (track) track.style.display = '';
            bar.style.width = Math.round((cur / total) * 100) + '%';
        } else {
            bar.style.width = '0%';
        }
    },

    hint(text) {
        const el = document.getElementById('flashHint');
        if (!el) return;
        el.style.display = text ? '' : 'none';
        el.textContent = text || '';
    },

    result(html, ok) {
        const res = document.getElementById('flashResult');
        if (!res) return;
        res.style.display = '';
        res.classList.toggle('ok', !!ok);
        res.classList.toggle('err', !ok);
        res.innerHTML = html;
    },

    button(label, disabled) {
        const btn = document.getElementById('flashNowButton');
        if (!btn) return;
        btn.textContent = label;
        btn.disabled = !!disabled;
        btn.classList.toggle('attention', /select/i.test(label));
    },
};

function flashLog(message) {
    console.log('[flash]', message);
    const el = document.getElementById('serialFlashLog');
    if (el) { el.textContent += message + '\n'; el.scrollTop = el.scrollHeight; }
}

const flashEngine = {
    running: false,
    pendingPick: null, // resolver waiting for a user click to open the picker

    // Called by the hero button. Either starts a run or satisfies a pending
    // "we need one more user gesture to show the picker" state.
    onHeroClick() {
        if (this.pendingPick) {
            const resolve = this.pendingPick;
            this.pendingPick = null;
            resolve();
            return;
        }
        if (this.running) return;
        this.run({ manualPick: false });
    },

    // Wait for the user to click the hero button so we regain a user gesture
    // (requestPort needs one). Used only when the bootloader device has never
    // been granted before.
    waitForGesture(buttonLabel, hintText) {
        flashUI.button(buttonLabel, false);
        flashUI.hint(hintText);
        return new Promise(resolve => { this.pendingPick = resolve; });
    },

    async requestPortFiltered() {
        try {
            return await navigator.serial.requestPort({ filters: boardFilters() });
        } catch (err) {
            if (err.name === 'NotFoundError') {
                // Nothing matched the filters (unusual bootloader IDs) — offer
                // the unfiltered list once.
                await this.waitForGesture('Select the new device', 'Your adapter was not in the list. Click to see every port.');
                return await navigator.serial.requestPort();
            }
            throw err;
        }
    },

    async run({ manualPick }) {
        if (!webSerialSupported) return;
        this.running = true;
        flashUI.reset();
        flashUI.button('Working…', true);

        const isMicro = wizardState.board === 'micro';

        try {
            // Validate firmware selection up front and start the download early.
            const firmware = getFirmwareFile();
            if (!firmware || firmware.unavailable) {
                throw new Error(firmware
                    ? `This board has no firmware in ${firmware.version}. Pick a different version above.`
                    : 'Pick a version above first.');
            }
            flashLog(`Fetching ${firmware.filename}…`);
            const firmwarePromise = this.fetchFirmware(firmware);

            // Stage 1: find the adapter
            flashUI.stage('find', 'active');
            let port = null;

            if (!manualPick) {
                const granted = await navigator.serial.getPorts();
                port = granted.find(p => classifyPort(p) === 'bootloader') ||
                       granted.find(p => classifyPort(p) === 'app');
                if (port) flashLog(`Reusing remembered device (${portLabel(port)}).`);
            }
            if (!port) {
                flashUI.hint('Pick your adapter in the popup. The list only shows devices that look like yours.');
                // Filtered to the selected board's USB vendor so unrelated COM
                // ports (Bluetooth etc.) never appear. Manual mode shows all.
                port = manualPick
                    ? await navigator.serial.requestPort()
                    : await navigator.serial.requestPort({ filters: boardFilters() });
                flashUI.hint('');
                flashLog(`Port selected (${portLabel(port)}).`);
            }

            let cls = classifyPort(port);
            if (manualPick && cls === 'unknown') {
                // Manual mode: trust the user, assume it's already the bootloader.
                cls = 'bootloader';
            }
            flashUI.stage('find', 'done');

            // Stage 2: get into the bootloader
            flashUI.stage('boot', 'active');
            let bootPort = null;

            if (cls === 'bootloader') {
                flashLog('Device is already in bootloader mode.');
                bootPort = port;
            } else if (cls === 'unknown' && !isMicro) {
                // Unrecognized USB IDs (DIY builds): probe SAM-BA first — it may
                // already be a bootloader we don't recognize.
                flashLog('Unrecognized device, probing for a bootloader…');
                bootPort = (await this.probeSamba(port)) ? port : null;
                if (!bootPort) {
                    flashLog('Not a bootloader. Sending reboot command…');
                    bootPort = await this.rebootAndReacquire(port, isMicro);
                }
            } else {
                flashLog('Adapter is running normally. Sending reboot-to-bootloader command…');
                bootPort = await this.rebootAndReacquire(port, isMicro);
            }
            flashUI.stage('boot', 'done');

            // Stage 3: write firmware
            flashUI.stage('write', 'active');
            const fw = await firmwarePromise;
            if (isMicro) {
                await this.flashAvr109(bootPort, fw);
            } else {
                await this.flashSamba(bootPort, fw);
            }
            flashUI.stage('write', 'done');

            // Done
            flashUI.stage('done', 'done');
            flashUI.progress(1, 1);
            flashUI.result(
                `<strong>Firmware installed.</strong> Your adapter is rebooting into ${escapeHtml(getSelectedVersionLabel())}. ` +
                `Next stop: the <a href="https://vailadapter.com/gettingstarted" target="_blank" rel="noopener">Getting Started guide</a> to set keyer type, speed, and tone.`,
                true
            );
            flashUI.button('Update again', false);
        } catch (err) {
            if (err && err.name === 'NotFoundError') {
                flashLog('Port selection cancelled.');
                flashUI.reset();
                flashUI.hint('No device picked. If your adapter was missing from the list, check the cable (some only charge), or use "Pick port manually" under advanced options to see every port.');
                flashUI.button('Update firmware', false);
            } else {
                flashLog(`❌ ${err.message}`);
                const stage = document.querySelector('#flashTimeline li.active');
                if (stage) flashUI.stage(stage.dataset.stage, 'error');
                flashUI.result(
                    `<strong>That didn't work.</strong> ${escapeHtml(err.message)}<br>` +
                    `Unplug the adapter, plug it back in, and try again. The activity log under advanced options has the details, ` +
                    `and the Download file method always works as a backup.`,
                    false
                );
                flashUI.button('Try again', false);
            }
        } finally {
            this.running = false;
            this.pendingPick = null;
        }
    },

    async fetchFirmware(firmware) {
        const tag = adapterReleases.selected && adapterReleases.selected.tag_name;
        const proxyUrl = `${ADAPTER_FIRMWARE_PROXY}/vail-adapter/${tag}/${firmware.filename}`;
        const resp = await fetch(proxyUrl, { cache: 'no-cache' });
        if (!resp.ok) throw new Error(`Firmware download failed (HTTP ${resp.status}). Check your connection and try again.`);
        if (firmware.ext === 'hex') {
            return { kind: 'hex', text: await resp.text() };
        }
        const { bin, baseAddr } = window.uf2ToBin(await resp.arrayBuffer());
        flashLog(`Firmware ready: ${bin.length} bytes @ 0x${baseAddr.toString(16)}.`);
        return { kind: 'bin', bin, baseAddr };
    },

    // Quick, quiet SAM-BA handshake check used for unrecognized devices.
    async probeSamba(port) {
        const probe = new window.SAMBAFlasher({ log: () => {} });
        try {
            await probe.open(port);
            await probe.connect();
            await probe.close();
            return true;
        } catch (_) {
            try { await probe.close(); } catch (_) {}
            return false;
        }
    },

    // 1200-baud touch, then get the bootloader port back WITHOUT a second
    // picker whenever possible.
    async rebootAndReacquire(appPort, isMicro) {
        const before = await navigator.serial.getPorts();
        await touch1200(appPort);
        flashLog('Reboot command sent. Waiting for the bootloader to appear…');
        flashUI.hint('The adapter is restarting into bootloader mode…');

        const found = await waitForBootloaderPort(before, isMicro ? 6000 : 8000);
        if (found) {
            flashUI.hint('');
            flashLog(`Bootloader found automatically (${portLabel(found)}).`);
            return found;
        }

        // Never-granted bootloader: we need one click to show the picker.
        // (Chrome forgets the user gesture after a few seconds of waiting.)
        flashLog('Bootloader needs a one-time permission grant.');
        await this.waitForGesture(
            'Select the new device',
            'One more click: the adapter reappeared as a new device. Click the button, then pick it in the popup. You only do this once.'
        );
        flashUI.button('Working…', true);
        const picked = await this.requestPortFiltered();
        flashUI.hint('');
        flashLog(`Bootloader selected (${portLabel(picked)}).`);
        return picked;
    },

    async flashSamba(port, fw) {
        if (fw.kind !== 'bin') throw new Error('Wrong firmware type for this board.');
        const flasher = new window.SAMBAFlasher({
            log: flashLog,
            progress: (cur, total) => flashUI.progress(cur, total),
        });
        try {
            await flasher.open(port);
            await flasher.connect();
            await flasher.eraseApp();
            await flasher.writeFirmware(fw.bin, fw.baseAddr);
            await flasher.resetDevice();
        } finally {
            try { await flasher.close(); } catch (_) {}
        }
    },

    async flashAvr109(port, fw) {
        if (fw.kind !== 'hex') throw new Error('Wrong firmware type for this board.');
        const flasher = new window.AVR109Flasher({
            log: flashLog,
            progress: (cur, total) => flashUI.progress(cur, total),
        });
        try {
            await flasher.openPort(port, 57600);
            await flasher.flashHex(fw.text);
        } finally {
            try { await flasher.close(); } catch (_) {}
        }
    },
};

// --- Advanced tools ----------------------------------------------------------

// Reboot into bootloader mode without flashing (feeds the UF2 flow too).
// logFn/hintEl let the UF2 panel and the advanced panel share this.
async function enterBootloaderOnly(logFn, hintElId) {
    const hintEl = document.getElementById(hintElId);
    const setHint = (t, ok) => {
        if (!hintEl) return;
        hintEl.style.display = t ? '' : 'none';
        hintEl.textContent = t || '';
        hintEl.classList.toggle('ok', !!ok);
    };

    if (!webSerialSupported) {
        setHint('This browser cannot talk to USB devices. Double-tap the reset button on the adapter instead.');
        return;
    }

    try {
        let port = null;
        const granted = await navigator.serial.getPorts();
        port = granted.find(p => classifyPort(p) === 'app');
        if (granted.find(p => classifyPort(p) === 'bootloader')) {
            setHint('Your adapter is already in bootloader mode. The boot drive should be visible now.', true);
            return;
        }
        if (port) {
            logFn(`Reusing remembered device (${portLabel(port)}).`);
        } else {
            setHint('Pick your adapter in the popup.');
            port = await navigator.serial.requestPort({ filters: boardFilters() });
        }
        logFn('Sending reboot-to-bootloader command (1200 baud touch)…');
        await touch1200(port);
        logFn('✅ Done. The boot drive (QTPYBOOT / XIAOBOOT / ADAPTERBOOT) should appear in a few seconds.');
        setHint('Done. Watch for the boot drive to appear, then continue to the next step.', true);
    } catch (err) {
        if (err.name === 'NotFoundError') { setHint(''); return; }
        logFn(`❌ ${err.message}`);
        setHint(`That failed: ${err.message}. You can always double-tap the reset button instead.`);
    }
}

function uf2Log(message) {
    console.log('[uf2]', message);
    const el = document.getElementById('serialLog');
    if (el) { el.textContent += message + '\n'; el.scrollTop = el.scrollHeight; }
}

async function forgetRememberedPorts() {
    const hint = document.getElementById('flashHint');
    try {
        const ports = await navigator.serial.getPorts();
        let n = 0;
        for (const p of ports) {
            if (typeof p.forget === 'function') { await p.forget(); n++; }
        }
        flashLog(`Forgot ${n} remembered device${n === 1 ? '' : 's'}.`);
        if (hint) { hint.style.display = ''; hint.textContent = `Forgot ${n} remembered device${n === 1 ? '' : 's'}. The next update will ask you to pick again.`; }
    } catch (err) {
        flashLog(`❌ ${err.message}`);
    }
}

// The "Erase app" tool is test-only — reveal it only when the URL hash opts in.
function maybeRevealEraseTest() {
    const el = document.getElementById('serialEraseTest');
    if (el) el.style.display = /test|erase|debug/i.test(location.hash) ? 'block' : 'none';
}

// Testing helper: erase just the app region so the bootloader finds no valid
// app — reproduces the "stuck in bootloader" state. Fully recoverable.
async function eraseAdapterAppForTest() {
    if (!webSerialSupported) return;
    if (!confirm('TEST ONLY: this erases the adapter\'s firmware so it boots into bootloader (storage) mode every time. You can recover it with the update button or a UF2 file. Continue?')) {
        return;
    }
    let flasher = null;
    try {
        let port = (await navigator.serial.getPorts()).find(p => classifyPort(p) === 'bootloader');
        if (!port) {
            flashLog("Select your adapter's bootloader COM port…");
            port = await navigator.serial.requestPort({ filters: boardFilters() });
        }
        flasher = new window.SAMBAFlasher({ log: flashLog });
        await flasher.open(port);
        await flasher.connect();
        await flasher.eraseApp();
        await flasher.resetDevice();
        flashLog('✅ App erased. The adapter now boots into bootloader mode on every plug-in until you reflash it.');
    } catch (err) {
        flashLog(`❌ ${err.message}`);
    } finally {
        if (flasher) { try { await flasher.close(); } catch (_) {} }
    }
}

// --- Markdown helpers for release notes ---------------------------------------

function escapeHtml(s) {
    return String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

function stripMarkdown(s) {
    return s
        .replace(/\[([^\]]+)\]\([^)]+\)/g, '$1')
        .replace(/\*\*([^*]+)\*\*/g, '$1')
        .replace(/`([^`]+)`/g, '$1')
        .trim();
}

function releaseBodyToItems(body) {
    const items = [];
    for (const raw of (body || '').split(/\r?\n/)) {
        const line = raw.trim();
        if (!line) continue;
        if (/^go to\s+https?:\/\//i.test(line) || /update\.vailadapter\.com/i.test(line)) continue;

        const heading = line.match(/^#{1,6}\s+(.*)$/);
        if (heading) {
            const text = escapeHtml(stripMarkdown(heading[1]));
            items.push(`<li class="note-heading">${text}</li>`);
            continue;
        }

        const bullet = line.match(/^[-*]\s+(.*)$/);
        const text = escapeHtml(stripMarkdown(bullet ? bullet[1] : line));
        if (text) items.push(`<li>${text}</li>`);
    }
    return items;
}

// --- What's New ---------------------------------------------------------------

async function fetchRecentUpdates(deviceType) {
    const repoName = deviceType === 'summit' ? 'vail-summit' : 'vail-adapter';
    const deviceLabel = deviceType === 'summit' ? 'Vail Summit' : 'Vail Adapter';

    const section = document.getElementById('whatsNewSection');
    const dateElement = document.getElementById('lastUpdateDate');
    const listElement = document.getElementById('recentCommitsList');

    const pageTitle = document.getElementById('pageTitle');
    if (pageTitle) pageTitle.textContent = `Update your ${deviceLabel}`;

    const deviceElement = document.getElementById('whatsNewDevice');
    if (deviceElement) deviceElement.textContent = deviceLabel;

    const manualLink = document.getElementById('manualLink');
    if (manualLink) manualLink.style.display = deviceType === 'adapter' ? 'block' : 'none';

    const releaseNotesLink = document.getElementById('releaseNotesLink');
    if (releaseNotesLink) releaseNotesLink.style.display = 'none';

    if (section) section.style.display = 'block';
    if (dateElement) dateElement.textContent = 'Loading...';
    if (listElement) listElement.innerHTML = '<li>Loading latest release...</li>';

    try {
        const response = await fetch(`https://api.github.com/repos/Vail-CW/${repoName}/releases/latest`);
        if (!response.ok) {
            if (section) section.style.display = 'none';
            return;
        }
        const release = await response.json();

        const versionLabel = release.name || release.tag_name || '';
        const publishedDate = new Date(release.published_at);
        const dateStr = publishedDate.toLocaleDateString('en-US', {
            year: 'numeric', month: 'long', day: 'numeric'
        });
        if (dateElement) {
            dateElement.textContent = versionLabel ? `${versionLabel} — ${dateStr}` : dateStr;
        }

        if (listElement) {
            const items = releaseBodyToItems(release.body);
            listElement.innerHTML = items.length
                ? items.join('')
                : '<li>See the release notes on GitHub for details.</li>';
        }

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

function hideWhatsNew() {
    const section = document.getElementById('whatsNewSection');
    if (section) section.style.display = 'none';
    const pageTitle = document.getElementById('pageTitle');
    if (pageTitle) pageTitle.textContent = 'Update your Vail device';
}

// --- Selection handling ---------------------------------------------------------

function selectDevice(device) {
    wizardState.device = device;
    if (device === 'adapter') {
        fetchRecentUpdates('adapter');
        adapterReleases.fetch();
        goToStep('model');
    } else {
        wizardState.model = null;
        wizardState.board = null;
        fetchRecentUpdates('summit');
        saveSetup();
        goToStep('summit');
    }
}

function selectModel(model) {
    if (wizardState.model !== model) wizardState.board = null;
    wizardState.model = model;
    if (model === 'vail_lite') {
        wizardState.board = 'trinkey';
        saveSetup();
        goToStep('update');
    } else {
        goToStep('board');
    }
}

function selectBoard(board) {
    wizardState.board = board;
    saveSetup();
    goToStep('update');
}

function wireCards(sectionId, dataKey, handler) {
    document.querySelectorAll(`#${sectionId} .selection-card`).forEach(card => {
        const pick = () => {
            document.querySelectorAll(`#${sectionId} .selection-card`).forEach(c => c.classList.remove('selected'));
            card.classList.add('selected');
            handler(card.dataset[dataKey]);
        };
        card.addEventListener('click', pick);
        card.addEventListener('keydown', (e) => {
            if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); pick(); }
        });
    });
}

// --- Init -----------------------------------------------------------------------

document.addEventListener('DOMContentLoaded', () => {
    wireCards('step1', 'device', selectDevice);
    wireCards('step1_5', 'model', selectModel);
    wireCards('step2', 'board', selectBoard);

    // Method tabs
    document.getElementById('tabSerial')?.addEventListener('click', () => {
        if (webSerialSupported) setMethod('serial');
    });
    document.getElementById('tabUf2')?.addEventListener('click', () => setMethod('uf2'));
    document.getElementById('switchToUf2')?.addEventListener('click', () => setMethod('uf2'));

    // One-click flash
    document.getElementById('flashNowButton')?.addEventListener('click', () => flashEngine.onHeroClick());

    // Advanced tools
    document.getElementById('bootOnlyButton')?.addEventListener('click', () => enterBootloaderOnly(flashLog, 'flashHint'));
    document.getElementById('manualPortButton')?.addEventListener('click', () => {
        if (!flashEngine.running) flashEngine.run({ manualPick: true });
    });
    document.getElementById('forgetPortsButton')?.addEventListener('click', forgetRememberedPorts);
    document.getElementById('serialEraseButton')?.addEventListener('click', eraseAdapterAppForTest);

    // UF2 flow
    document.getElementById('bootModeButton')?.addEventListener('click', () => enterBootloaderOnly(uf2Log, 'uf2BootHint'));
    document.getElementById('downloadButton')?.addEventListener('click', function (event) {
        if (this.classList.contains('disabled')) event.preventDefault();
    });

    // Test-only erase tool reveal
    window.addEventListener('hashchange', maybeRevealEraseTest);
    maybeRevealEraseTest();

    // Quick resume from a previous visit
    const saved = loadSetup();
    if (saved) {
        const card = document.getElementById('resumeCard');
        const summary = document.getElementById('resumeSummary');
        if (card && summary) {
            const label = saved.device === 'summit'
                ? 'Vail Summit'
                : [getModelName(saved.model), saved.model === 'vail_lite' ? null : getBoardName(saved.board)].filter(Boolean).join(' · ');
            summary.textContent = `Jump straight to updating your ${label}.`;
            card.style.display = '';

            document.getElementById('resumeButton')?.addEventListener('click', () => {
                card.style.display = 'none';
                wizardState.device = saved.device;
                wizardState.model = saved.model;
                wizardState.board = saved.board;
                wizardState.flashMethod = saved.flashMethod || 'serial';
                if (saved.device === 'summit') {
                    fetchRecentUpdates('summit');
                    goToStep('summit');
                } else {
                    fetchRecentUpdates('adapter');
                    adapterReleases.fetch();
                    goToStep('update');
                }
            });
            document.getElementById('resumeDismiss')?.addEventListener('click', () => {
                card.style.display = 'none';
                try { localStorage.removeItem(STORAGE_KEY); } catch (_) {}
            });
        }
    }
});
