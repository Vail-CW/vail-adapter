// Troubleshooting Recorder — records a Vail device's serial output so users can
// save it to a file and send it in for help. Device-agnostic: the user picks
// their device, which selects the correct serial speed for them.
//
// Baud is mapped from the chosen device so the user never has to guess:
//   - Vail Adapter (SAMD21/32u4 native USB) talks at 9600
//   - Vail Summit  (ESP32) talks at 115200
// For native-USB CDC devices the baud value is effectively ignored by the
// hardware, but we set the correct one anyway for the UART-bridge cases.
//
// Native-USB boards (the Adapter) can't be rebooted over the wire, so to catch
// their startup messages we use "Capture Startup": grant permission to the port
// while it's plugged in, ask the user to unplug & replug, then aggressively
// re-open the same port the instant it re-enumerates and start recording.

const DEVICES = {
    adapter: { label: 'Vail Adapter', baud: 9600,   esp: false },
    summit:  { label: 'Vail Summit',  baud: 115200, esp: true  },
};

const state = {
    device: null,        // 'adapter' | 'summit'
    baud: null,
    port: null,
    reader: null,
    keepReading: false,
    connected: false,
    watchPort: null,     // port we have permission to, awaiting replug
    watching: false,     // aggressively trying to re-open watchPort
    lines: [],           // [{ t: Date, text: string }] finalized lines
    pending: '',         // partial line not yet terminated by \n
    timestamps: false,
    autoscroll: true,
};

// ---- small DOM helpers ----------------------------------------------------
const $ = (id) => document.getElementById(id);

function setStatus(text, kind) {
    const el = $('connStatus');
    if (!el) return;
    el.textContent = text;
    el.className = 'conn-status' + (kind ? ' ' + kind : '');
}

// Show the right set of buttons for the current device + connection state.
function applyDeviceButtons() {
    const isAdapter = state.device === 'adapter';
    const isSummit = state.device === 'summit';
    const connected = state.connected;
    const watching = state.watching;

    $('connectButton').style.display = (!connected && !watching) ? 'inline-block' : 'none';
    $('armStartupButton').style.display = (!connected && !watching && isAdapter) ? 'inline-block' : 'none';

    const disconnectBtn = $('disconnectButton');
    disconnectBtn.style.display = (connected || watching) ? 'inline-block' : 'none';
    disconnectBtn.textContent = watching ? 'Cancel' : 'Disconnect';

    const restartBtn = $('restartButton');
    restartBtn.style.display = isSummit ? 'inline-block' : 'none';
    restartBtn.disabled = !connected;

    // Self check is Summit-only; it shifts the later step numbers down by one
    const sc = $('selfCheckStep');
    if (sc) sc.style.display = isSummit ? 'block' : 'none';
    if ($('badgeRecord')) $('badgeRecord').textContent = isSummit ? '3' : '2';
    if ($('badgeSave')) $('badgeSave').textContent = isSummit ? '4' : '3';
    const busy = selfCheck.running || (typeof btCheck !== 'undefined' && btCheck.running);
    const runBtn = $('scRunButton');
    if (runBtn) runBtn.disabled = !connected || busy;
    const btBtn = $('btRunButton');
    if (btBtn) btBtn.disabled = !connected || busy;
}

function setConnected(connected) {
    state.connected = connected;
    applyDeviceButtons();
}

// ---- log rendering --------------------------------------------------------
function formatLine(line) {
    if (!state.timestamps) return line.text;
    const t = line.t;
    const hh = String(t.getHours()).padStart(2, '0');
    const mm = String(t.getMinutes()).padStart(2, '0');
    const ss = String(t.getSeconds()).padStart(2, '0');
    const ms = String(t.getMilliseconds()).padStart(3, '0');
    return `[${hh}:${mm}:${ss}.${ms}] ${line.text}`;
}

function renderAll() {
    const pre = $('debugLog');
    if (!pre) return;
    // "@vail {json}" lines are self-check protocol traffic: kept in the saved
    // recording, hidden from the on-screen view so the log stays readable.
    const body = state.lines.filter((l) => !isVailLine(l.text)).map(formatLine).join('\n');
    pre.textContent = body + (state.pending ? (state.lines.length ? '\n' : '') + state.pending : '');
    if (state.autoscroll) pre.scrollTop = pre.scrollHeight;
}

function addChunk(text) {
    state.pending += text;
    let idx;
    let added = false;
    while ((idx = state.pending.indexOf('\n')) >= 0) {
        const line = state.pending.slice(0, idx).replace(/\r$/, '');
        state.pending = state.pending.slice(idx + 1);
        state.lines.push({ t: new Date(), text: line });
        added = true;
        if (isVailLine(line)) handleVailLine(line);
    }
    // Trim the on-screen buffer for very long sessions to keep the DOM light.
    if (state.lines.length > 5000) {
        state.lines.splice(0, state.lines.length - 5000);
        if (!state._trimmed) {
            state._trimmed = true;
            console.log('[troubleshoot] display trimmed to last 5000 lines.');
        }
    }
    renderAll();
    return added;
}

function logNotice(text) {
    state.lines.push({ t: new Date(), text: `--- ${text} ---` });
    renderAll();
}

// ---- open / read ----------------------------------------------------------
async function beginSession(port) {
    state.port = port;
    // Assert DTR/RTS so devices that gate logging on an open terminal start talking.
    try { await port.setSignals({ dataTerminalReady: true, requestToSend: true }); } catch (_) {}
    setConnected(true);
    setStatus(`Connected — ${DEVICES[state.device].label} @ ${state.baud} baud`, 'ok');
    state.keepReading = true;
    readLoop(); // fire and forget
}

async function connect() {
    if (!('serial' in navigator)) {
        alert('Web Serial is not supported in this browser. Please use Chrome, Edge, or Opera.');
        return;
    }
    if (!state.device) {
        alert('Please choose your device first.');
        return;
    }
    try {
        const port = await navigator.serial.requestPort();
        state.watchPort = port; // remember it for a later startup capture
        await port.open({ baudRate: state.baud, bufferSize: 4096 });
        logNotice(`Connected to ${DEVICES[state.device].label} at ${state.baud} baud`);
        await beginSession(port);
    } catch (err) {
        if (err && err.name === 'NotFoundError') {
            setStatus('No device selected.', '');
        } else {
            setStatus(`Error: ${err.message}`, 'err');
            alert(`Could not open the connection: ${err.message}`);
        }
    }
}

async function readLoop() {
    const decoder = new TextDecoder();
    try {
        while (state.keepReading && state.port && state.port.readable) {
            state.reader = state.port.readable.getReader();
            try {
                while (true) {
                    const { value, done } = await state.reader.read();
                    if (done) break;
                    if (value) addChunk(decoder.decode(value, { stream: true }));
                }
            } catch (err) {
                logNotice(`Read error: ${err.message}`);
            } finally {
                try { state.reader.releaseLock(); } catch (_) {}
                state.reader = null;
            }
        }
    } finally {
        const tail = decoder.decode();
        if (tail) addChunk(tail);
    }
}

// Close the active port without forgetting which port we're watching.
async function softClose() {
    state.keepReading = false;
    try { if (state.reader) await state.reader.cancel(); } catch (_) {}
    try { if (state.port) await state.port.close(); } catch (_) {}
    state.port = null;
}

async function disconnect() {
    abortPending('Disconnected.');
    state.watching = false; // doubles as "Cancel" while waiting for a replug
    await softClose();
    setConnected(false);
    setStatus('Disconnected.', '');
}

// ---- startup capture (unplug & replug, then latch on) ---------------------
// Grab permission to the port while it's plugged in, then poll-open it as fast
// as the OS will allow so we catch the boot output the moment it reconnects.
async function armStartupCapture() {
    if (!('serial' in navigator)) {
        alert('Web Serial is not supported in this browser. Please use Chrome, Edge, or Opera.');
        return;
    }
    try {
        if (!state.watchPort) {
            setStatus('Select your adapter (it must be plugged in right now)…', '');
            state.watchPort = await navigator.serial.requestPort();
        }
    } catch (err) {
        if (err && err.name === 'NotFoundError') {
            setStatus('No device selected.', '');
            return;
        }
        setStatus(`Error: ${err.message}`, 'err');
        return;
    }

    await softClose(); // release the port so the OS can drop it on unplug

    state.watching = true;
    applyDeviceButtons();
    setStatus('Now UNPLUG the adapter, wait ~2 seconds, then plug it back in…', 'wait');
    logNotice('Waiting for you to unplug and replug the adapter — recording will start automatically');
    watchLoop();
}

async function watchLoop() {
    // Repeatedly try to open the watched port. While the device is unplugged the
    // open() call rejects quickly; the first attempt that succeeds is the instant
    // the device came back, so we latch on and start recording immediately.
    while (state.watching && state.watchPort) {
        try {
            await state.watchPort.open({ baudRate: state.baud, bufferSize: 4096 });
        } catch (_) {
            await sleep(80); // not back yet — keep hammering
            continue;
        }
        // Opened! We're latched on.
        if (!state.watching) {
            // Cancelled in the tiny window between open and here
            try { await state.watchPort.close(); } catch (_) {}
            return;
        }
        state.watching = false;
        logNotice('Adapter reconnected — recording started');
        await beginSession(state.watchPort);
        return;
    }
}

function sleep(ms) { return new Promise((r) => setTimeout(r, ms)); }

// ---- restart device (Summit) ----------------------------------------------
// ESP32 auto-reset: pulse RTS (wired to EN) while DTR (GPIO0) stays high so it
// boots the application, not the bootloader.
// Returns true if the reset signals went out. A check that is running gets
// cancelled unless it asked for the restart itself (opts.expected).
async function restartDevice(opts) {
    if (!state.port) return false;
    if (!(opts && opts.expected === true)) abortPending('The Summit was restarted.');
    try {
        logNotice('Restarting device…');
        await state.port.setSignals({ dataTerminalReady: false, requestToSend: true });
        await sleep(120);
        await state.port.setSignals({ dataTerminalReady: false, requestToSend: false });
        await sleep(50);
        await state.port.setSignals({ dataTerminalReady: true });
        return true;
    } catch (err) {
        logNotice(`Restart did not work: ${err.message}. Try the physical reset button.`);
        return false;
    }
}

// ---- save / copy / clear --------------------------------------------------
function fullText() {
    const lines = state.lines.map(formatLine);
    if (state.pending) lines.push(state.pending);
    return lines.join('\n');
}

function timestampSlug() {
    const d = new Date();
    const p = (n, w = 2) => String(n).padStart(w, '0');
    return `${d.getFullYear()}${p(d.getMonth() + 1)}${p(d.getDate())}-${p(d.getHours())}${p(d.getMinutes())}${p(d.getSeconds())}`;
}

function saveLog() {
    const deviceLabel = state.device ? DEVICES[state.device].label : 'Unknown device';
    const report = allReportsText();
    const header = [
        '=== Vail Troubleshooting Recording ===',
        `Device:    ${deviceLabel}`,
        `Baud rate: ${state.baud || '(not set)'}`,
        `Recorded:  ${new Date().toString()}`,
        `Browser:   ${navigator.userAgent}`,
        '======================================',
        '',
        report ? report + '\n\n=== Full device log ===' : '',
    ].join('\n');

    const blob = new Blob([header + fullText() + '\n'], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    const slug = (state.device || 'device');
    a.href = url;
    a.download = `vail-${slug}-troubleshooting-${timestampSlug()}.txt`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    setTimeout(() => URL.revokeObjectURL(url), 1000);
}

async function copyLog() {
    try {
        const report = allReportsText();
        await navigator.clipboard.writeText(report ? report + '\n\n=== Full device log ===\n' + fullText() : fullText());
        const btn = $('copyButton');
        const original = btn.textContent;
        btn.textContent = '✅ Copied!';
        setTimeout(() => { btn.textContent = original; }, 1500);
    } catch (err) {
        alert(`Could not copy: ${err.message}`);
    }
}

function clearLog() {
    state.lines = [];
    state.pending = '';
    state._trimmed = false;
    renderAll();
}

// ---- Summit self check ----------------------------------------------------
// The Summit (firmware with the self check) answers "@vail <command>" lines with
// "@vail {json}" lines. This drives its built-in checks, asks the yes/no
// questions here instead of on the tiny screen, and builds a plain-text report
// that goes at the top of Copy / Save.

const SELF_CHECK_EMAIL_URL = 'mailto:ke9bos@pigletradio.org';

const selfCheck = {
    running: false,
    diagActive: false,     // between sending "diag" and its diag_end
    collectingEvents: false,
    error: '',             // why the run stopped early, if it did
    skipped: [],           // names of guided tests the user skipped
    hello: null,           // device info from "hello"
    results: [],           // check lines, in order (guided answers included)
    events: [],            // event history entries
    eventsInfo: null,      // { count, boot, modes }
    finishedAt: null,
};

const vailWaiters = [];    // { match(msg) -> bool, resolve, reject, timer }

// Plain-English versions of the Summit's refusals
const VAIL_ERRORS = {
    not_on_menu: "The Summit is busy with something else. On the Summit, press ESC until you're on a menu " +
                 '(and close any pop-up), then click Run Self Check again.',
    busy: 'The Summit is still finishing a test. Wait a moment, then click Run Self Check again.',
    'unknown command': "The Summit's firmware is too old for this check. Update it from the Home page, then try again.",
    not_on_bt_screen: 'The typing test only runs on the External Keyboard screen. On the Summit, go to Settings → Device Settings → Bluetooth → External Keyboard, then try again.',
    bt_not_connected: "The keyboard isn't connected right now. Press a key on it to wake it, wait for the Summit to show Connected, then try again.",
};

// Usually at the start of a line, but an ordinary log print the Summit had to
// cut short can leave a fragment in front of it, so look anywhere in the line.
function isVailLine(text) {
    return text.includes('@vail {');
}

function handleVailLine(text) {
    let msg;
    try { msg = JSON.parse(text.slice(text.indexOf('@vail {') + 6)); } catch (_) { return; }
    if (msg.t === 'err') {
        failVailWaiters(VAIL_ERRORS[msg.msg] || `The Summit said: ${msg.msg}`);
        return;
    }
    onVailMessage(msg);
    if (typeof btCheckOnMessage === 'function') btCheckOnMessage(msg);
    for (let i = vailWaiters.length - 1; i >= 0; i--) {
        const w = vailWaiters[i];
        if (w.match(msg)) {
            vailWaiters.splice(i, 1);
            clearTimeout(w.timer);
            w.resolve(msg);
        }
    }
}

function waitForVail(match, timeoutMs, timeoutMessage) {
    return new Promise((resolve, reject) => {
        const w = { match, resolve, reject, timer: null };
        w.timer = setTimeout(() => {
            const i = vailWaiters.indexOf(w);
            if (i >= 0) vailWaiters.splice(i, 1);
            reject(new Error(timeoutMessage || 'The Summit stopped answering.'));
        }, timeoutMs);
        vailWaiters.push(w);
    });
}

function failVailWaiters(reason) {
    while (vailWaiters.length) {
        const w = vailWaiters.pop();
        clearTimeout(w.timer);
        w.reject(new Error(reason));
    }
}

// Anything else a running check is waiting on (an on-page question, a trace
// watcher) registers here, so losing the device ends the check instead of
// leaving it stuck on a prompt nobody can answer.
const pendingAborts = new Set();

function onAbort(fn) {
    pendingAborts.add(fn);
    return () => pendingAborts.delete(fn);
}

function abortPending(reason) {
    failVailWaiters(reason);
    const fns = [...pendingAborts];
    pendingAborts.clear();
    for (const fn of fns) fn(reason);
}

async function sendVail(command) {
    if (!state.port || !state.port.writable) throw new Error('Not connected.');
    const writer = state.port.writable.getWriter();
    try {
        await writer.write(new TextEncoder().encode(`@vail ${command}\n`));
    } finally {
        writer.releaseLock();
    }
}

// Messages that update the page whether or not a step is waiting on them
function onVailMessage(msg) {
    switch (msg.t) {
        case 'hello':
            selfCheck.hello = msg;
            break;
        case 'diag_start':
            // Only the run we started; a late reply from a timed-out run
            // must not wipe the current one
            if (!selfCheck.diagActive) break;
            selfCheck.results = [];
            renderSelfCheckResults();
            break;
        case 'diag_end':
            selfCheck.diagActive = false;
            break;
        case 'check':
            if (!selfCheck.running) break;
            // A re-run of a guided test replaces its earlier answer
            selfCheck.results = selfCheck.results.filter((r) => r.id !== msg.id);
            selfCheck.results.push(msg);
            renderSelfCheckResults();
            break;
        case 'event':
            if (selfCheck.collectingEvents) selfCheck.events.push(msg);
            break;
        case 'events_end':
            if (selfCheck.collectingEvents) selfCheck.eventsInfo = msg;
            break;
        case 'key_target': {
            const box = document.querySelector(`#scKeyBoxes [data-index="${msg.index}"]`);
            if (box) box.classList.add('hit');
            break;
        }
        case 'key': {
            const typed = $('scKeysTyped');
            if (typed) typed.textContent = (typed.textContent ? typed.textContent + ' ' : '') + msg.key;
            break;
        }
        case 'paddle_step':
            setGuidedPrompt(msg.want === 'dit'
                ? 'Press the <strong>DIT</strong> side of your paddle (or your straight key).'
                : 'Now press the <strong>DAH</strong> side.');
            break;
        case 'closed':
            if (selfCheck.running) abortPending('The self check screen was closed on the Summit.');
            break;
        default:
            break;
    }
}

// ---- rendering ------------------------------------------------------------
const STATUS_ICON = { pass: '✅', warn: '⚠️', fail: '❌', info: 'ℹ️' };

function escapeHtml(s) {
    return String(s ?? '').replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
}

function renderSelfCheckResults() {
    const list = $('scResults');
    if (!list) return;
    list.innerHTML = selfCheck.results.map((r) => `
        <li class="sc-item sc-${escapeHtml(r.st)}">
            <span class="sc-icon">${STATUS_ICON[r.st] || '•'}</span>
            <div>
                <div><strong>${escapeHtml(r.name)}</strong> <span class="sc-detail">${escapeHtml(r.detail)}</span></div>
                ${r.advice && (r.st === 'warn' || r.st === 'fail') ? `<div class="sc-advice">${escapeHtml(r.advice)}</div>` : ''}
            </div>
        </li>`).join('');
    list.style.display = selfCheck.results.length ? 'block' : 'none';
}

function setSelfCheckStatus(text, kind) {
    const el = $('scStatus');
    if (!el) return;
    el.textContent = text;
    el.className = 'conn-status' + (kind ? ' ' + kind : '');
}

function setGuidedPrompt(html) {
    const el = $('scPrompt');
    if (el) el.innerHTML = html;
}

// Show a guided panel (ids <prefix>Guided, <prefix>Prompt, <prefix>GuidedExtra,
// <prefix>Buttons) with a prompt and a row of buttons; resolves with the
// clicked button's value, rejects if the check is aborted.
//
// promptHtml and extraHtml are page-authored markup: callers escape anything
// that came from the device with escapeHtml() before putting it in.
const promptCleanup = {};

function askIn(prefix, promptHtml, buttons, extraHtml = '') {
    if (promptCleanup[prefix]) promptCleanup[prefix]();   // drop a prompt nobody waits on any more
    $(prefix + 'Guided').style.display = 'block';
    $(prefix + 'Prompt').innerHTML = promptHtml;
    $(prefix + 'GuidedExtra').innerHTML = extraHtml;
    const row = $(prefix + 'Buttons');
    row.innerHTML = '';
    return new Promise((resolve, reject) => {
        const off = onAbort((reason) => reject(new Error(reason)));
        promptCleanup[prefix] = off;
        buttons.forEach((b) => {
            const btn = document.createElement('button');
            btn.className = b.primary ? 'btn-primary' : 'btn-secondary';
            btn.textContent = b.label;
            btn.addEventListener('click', () => {
                off();
                if (promptCleanup[prefix] === off) delete promptCleanup[prefix];
                resolve(b.value);
            });
            row.appendChild(btn);
        });
    });
}

function askGuided(promptHtml, buttons, extraHtml = '') {
    return askIn('sc', promptHtml, buttons, extraHtml);
}

// A guided test the user skipped: shown and reported, never counted as a pass
function noteSkipped(id, name) {
    selfCheck.skipped.push(name);
    selfCheck.results = selfCheck.results.filter((r) => r.id !== id);
    selfCheck.results.push({ id, name, st: 'info', detail: 'skipped' });
    renderSelfCheckResults();
}

function hideGuided() {
    if (promptCleanup.sc) { promptCleanup.sc(); delete promptCleanup.sc; }
    const panel = $('scGuided');
    if (panel) panel.style.display = 'none';
}

// ---- the flow -------------------------------------------------------------
async function runSelfCheck() {
    if (selfCheck.running || (typeof btCheck !== 'undefined' && btCheck.running) || !state.port) return;
    selfCheck.running = true;
    selfCheck.results = [];
    selfCheck.events = [];
    selfCheck.eventsInfo = null;
    selfCheck.finishedAt = null;
    selfCheck.error = '';
    selfCheck.skipped = [];
    renderSelfCheckResults();
    $('scDone').style.display = 'none';
    applyDeviceButtons();

    try {
        setSelfCheckStatus('Talking to the Summit…', 'wait');
        const hello = waitForVail((m) => m.t === 'hello', 4000,
            "The Summit didn't answer. Its firmware may be too old for the self check: update it to the newest version from the Home page, then try again.");
        await sendVail('hello');
        await hello;

        setSelfCheckStatus('Running automatic checks (about 10 seconds)…', 'wait');
        const diagDone = waitForVail((m) => m.t === 'diag_end', 60000);
        selfCheck.diagActive = true;
        await sendVail('diag');
        await diagDone;

        await guidedScreen();
        await guidedSound();
        await guidedKeys();
        await guidedPaddle();
        hideGuided();

        setSelfCheckStatus('Collecting the event history…', 'wait');
        selfCheck.events = [];
        selfCheck.collectingEvents = true;
        const evDone = waitForVail((m) => m.t === 'events_end', 5000);
        await sendVail('events');
        await evDone;
        selfCheck.collectingEvents = false;

        selfCheck.finishedAt = new Date();
        const fails = selfCheck.results.filter((r) => r.st === 'fail').length;
        const warns = selfCheck.results.filter((r) => r.st === 'warn').length;
        const skipped = selfCheck.skipped.length;
        const skipNote = skipped ? ` ${skipped} test(s) skipped.` : '';
        setSelfCheckStatus(fails + warns === 0
            ? (skipped ? `All done: everything that ran passed.${skipNote}` : 'All done: everything passed!')
            : `All done: ${fails} problem(s), ${warns} warning(s). See the advice above.${skipNote}`, fails ? 'err' : 'ok');
        showSelfCheckDone();
    } catch (err) {
        hideGuided();
        selfCheck.error = err.message;
        setSelfCheckStatus(err.message, 'err');
        try { await sendVail('stop'); } catch (_) {}
        if (selfCheck.results.length) {
            selfCheck.finishedAt = new Date();
            showSelfCheckDone();
        }
    } finally {
        selfCheck.running = false;
        selfCheck.diagActive = false;
        selfCheck.collectingEvents = false;
        applyDeviceButtons();
    }
}

async function guidedScreen() {
    setSelfCheckStatus('Screen test', 'wait');
    const on = waitForVail((m) => m.t === 'screen' && m.on === true, 5000);
    await sendVail('screen on');
    await on;
    const answer = await askGuided(
        'Look at the Summit\'s screen. Do you see <strong>8 colored bars</strong> (white, yellow, cyan, green, ' +
        'magenta, red, blue, black) and a <strong>smooth black-to-white strip</strong> below them?',
        [{ label: 'Yes, looks right', value: 'yes', primary: true },
         { label: 'No, something is off', value: 'no' },
         { label: 'Skip', value: 'skip' }]);
    if (answer !== 'skip') await sendVail(`answer screen ${answer}`);
    else noteSkipped('screen', 'Screen');
    await sendVail('screen off');
}

async function playTone(loud) {
    const done = waitForVail((m) => m.t === 'tone_done', 8000);
    await sendVail(loud ? 'tone loud' : 'tone');
    await done;
}

async function guidedSound() {
    setSelfCheckStatus('Sound test', 'wait');
    let answer = await askGuided('Listen to the Summit, then click <strong>Play tone</strong>.',
        [{ label: '🔊 Play tone', value: 'play', primary: true }, { label: 'Skip', value: 'skip' }]);
    if (answer === 'skip') { noteSkipped('sound', 'Sound'); return; }
    for (;;) {
        await playTone(false);
        answer = await askGuided('Did you hear a short beep from the Summit?',
            [{ label: 'Yes', value: 'yes', primary: true }, { label: 'No', value: 'no' },
             { label: 'Play again', value: 'again' }]);
        if (answer !== 'again') break;
    }
    await sendVail(`answer sound ${answer}`);
    if (answer === 'yes') return;

    // Not heard: try once louder to tell "volume too low" from "no sound at all"
    await askGuided("Let's try once more, louder. Unplug any headphones first.",
        [{ label: '🔊 Play loud tone', value: 'play', primary: true }]);
    await playTone(true);
    answer = await askGuided('Did you hear it this time?',
        [{ label: 'Yes', value: 'yes', primary: true }, { label: 'No', value: 'no' }]);
    await sendVail(`answer sound_loud ${answer}`);
}

async function guidedKeys() {
    setSelfCheckStatus('Keyboard test', 'wait');
    const start = await askGuided('Next, the keyboard. Click <strong>Start</strong>, then press the keys shown on the Summit keyboard.',
        [{ label: 'Start', value: 'go', primary: true }, { label: 'Skip', value: 'skip' }]);
    if (start === 'skip') { noteSkipped('keys', 'Keyboard'); return; }
    const boxes = ['A', '1', 'an arrow', 'ENTER']
        .map((k, i) => `<span class="sc-key" data-index="${i}">${k}</span>`).join('');
    const ended = waitForVail((m) => m.t === 'keys_end', 30000);
    await sendVail('keys');
    const skip = askGuided('Press each of these keys on the Summit (you have 20 seconds):',
        [{ label: 'Skip', value: 'skip' }],
        `<div id="scKeyBoxes" class="sc-keys">${boxes}</div><div class="muted">Typed: <span id="scKeysTyped"></span></div>`);
    const which = await Promise.race([ended.then(() => 'done'), skip]);
    if (which === 'skip') {
        await sendVail('stop');
        await ended.catch(() => {});
        noteSkipped('keys', 'Keyboard');
    }
}

async function guidedPaddle() {
    setSelfCheckStatus('Paddle test', 'wait');
    const start = await askGuided('Last one: plug your paddle (or straight key) into the Summit, then click <strong>Start</strong>.',
        [{ label: 'Start', value: 'go', primary: true }, { label: 'Skip (no paddle)', value: 'skip' }]);
    if (start === 'skip') { noteSkipped('paddle', 'Paddle'); return; }
    const ended = waitForVail((m) => m.t === 'paddle_end', 45000);
    await sendVail('paddle');
    const skip = askGuided('Press the <strong>DIT</strong> side of your paddle (or your straight key).',
        [{ label: 'Skip', value: 'skip' }]);
    const which = await Promise.race([ended.then(() => 'done'), skip]);
    if (which === 'skip') {
        await sendVail('stop');
        await ended.catch(() => {});
        noteSkipped('paddle', 'Paddle');
    }
}

// ---- report ---------------------------------------------------------------
// Every finished check's report (self check, Bluetooth keyboard check), for
// the top of Copy / Save
function allReportsText() {
    const bt = typeof btReportText === 'function' ? btReportText() : '';
    return [selfCheckReportText(), bt].filter(Boolean).join('\n\n');
}

function selfCheckReportText() {
    if (!selfCheck.finishedAt || !selfCheck.results.length) return '';
    const h = selfCheck.hello || {};
    const lines = [
        '=== Vail Summit Self Check ===',
        `Date:      ${selfCheck.finishedAt.toString()}`,
        `Firmware:  v${h.fw || '?'} (${h.date || '?'}), web files expected v${h.web || '?'}`,
        `Callsign:  ${h.call || '(not set)'}`,
        `PSRAM:     ${h.psram ? 'yes' : 'no'}, uptime ${h.uptime ?? '?'} s`,
        '',
        'Results:',
    ];
    const tag = { pass: 'PASS', warn: 'WARN', fail: 'FAIL', info: 'INFO' };
    for (const r of selfCheck.results) {
        lines.push(`  [${tag[r.st] || r.st}] ${r.name}: ${r.detail}`);
        if (r.advice && (r.st === 'warn' || r.st === 'fail')) lines.push(`         -> ${r.advice}`);
    }
    if (selfCheck.skipped.length) lines.push(`  Skipped: ${selfCheck.skipped.join(', ')}`);
    if (selfCheck.error) lines.push(`  STOPPED EARLY: ${selfCheck.error}`);
    lines.push('', 'Event history (since power-on):');
    if (!selfCheck.events.length) lines.push('  (none)');
    for (const e of selfCheck.events) {
        lines.push(`  boot ${e.boot} +${(e.ms / 1000).toFixed(1)}s  ${e.tag}: ${e.msg}`);
    }
    if (selfCheck.eventsInfo && selfCheck.eventsInfo.modes) {
        lines.push(`  recent screens: ${selfCheck.eventsInfo.modes}`);
    }
    lines.push('==============================');
    return lines.join('\n');
}

function showSelfCheckDone() {
    $('scDone').style.display = 'block';
    const h = selfCheck.hello || {};
    const problems = selfCheck.results.filter((r) => r.st === 'fail' || r.st === 'warn')
        .map((r) => `${r.name}: ${r.detail}`);
    const subject = `Summit self check v${h.fw || '?'}${h.call ? ' ' + h.call : ''}`;
    let summary = problems.length ? 'Problems found:\n- ' + problems.join('\n- ') + '\n\n'
        : (selfCheck.error || selfCheck.skipped.length ? 'No problems in the tests that ran.\n\n' : 'Everything passed.\n\n');
    if (selfCheck.skipped.length) summary += `Skipped: ${selfCheck.skipped.join(', ')}\n\n`;
    if (selfCheck.error) summary += `The check stopped early: ${selfCheck.error}\n\n`;
    const body = 'Hi, here is my Summit self check report.\n\n' + summary +
        'Full report: (click "Copy report" on the page, then paste it here with Ctrl+V, or attach the saved file)\n';
    $('scEmailLink').href = `${SELF_CHECK_EMAIL_URL}?subject=${encodeURIComponent(subject)}&body=${encodeURIComponent(body)}`;
}

async function copySelfCheckReport() {
    try {
        await navigator.clipboard.writeText(selfCheckReportText() + '\n\n=== Full device log ===\n' + fullText());
        const btn = $('scCopyButton');
        const original = btn.textContent;
        btn.textContent = '✅ Copied!';
        setTimeout(() => { btn.textContent = original; }, 1500);
    } catch (err) {
        alert(`Could not copy: ${err.message}`);
    }
}

// ---- per-device hint copy -------------------------------------------------
function deviceHint() {
    if (state.device === 'adapter') {
        return '💡 To record the startup messages, click <strong>Capture Startup</strong>: ' +
               'pick your adapter, then unplug it and plug it back in — recording starts ' +
               'the instant it reconnects. Or click <strong>Connect</strong> to just watch live.';
    }
    if (state.device === 'summit') {
        return '💡 Turn the Summit on, click <strong>Connect</strong>, then run the self check below. ' +
               'Use <strong>Restart Device</strong> if you also want its startup messages recorded.';
    }
    return '';
}

// ---- init -----------------------------------------------------------------
document.addEventListener('DOMContentLoaded', () => {
    if (!('serial' in navigator)) {
        const warn = $('noWebSerial');
        if (warn) warn.style.display = 'block';
    }

    document.querySelectorAll('#deviceCards .selection-card').forEach((card) => {
        card.addEventListener('click', async () => {
            // A different device: the open connection belongs to the old one,
            // so close it rather than send Summit commands to an Adapter
            if (card.dataset.device !== state.device && (state.connected || state.watching)) {
                await disconnect();
                logNotice('Switched device, disconnected. Click Connect to pick the new one');
            }
            document.querySelectorAll('#deviceCards .selection-card').forEach((c) => c.classList.remove('selected'));
            card.classList.add('selected');

            // Switching devices invalidates any port we had permission to.
            state.device = card.dataset.device;
            state.baud = DEVICES[state.device].baud;
            state.watchPort = null;

            $('chosenDevice').textContent = DEVICES[state.device].label;
            $('chosenBaud').textContent = state.baud;
            const hint = $('connectHint');
            if (hint) hint.innerHTML = deviceHint();
            $('monitorSection').style.display = 'block';
            applyDeviceButtons();
            setStatus('Ready to connect.', '');
            $('monitorSection').scrollIntoView({ behavior: 'smooth', block: 'start' });
        });
    });

    $('connectButton')?.addEventListener('click', connect);
    $('armStartupButton')?.addEventListener('click', armStartupCapture);
    $('disconnectButton')?.addEventListener('click', disconnect);
    $('restartButton')?.addEventListener('click', restartDevice);
    $('saveButton')?.addEventListener('click', saveLog);
    $('copyButton')?.addEventListener('click', copyLog);
    $('clearButton')?.addEventListener('click', clearLog);
    $('scRunButton')?.addEventListener('click', runSelfCheck);
    $('scCopyButton')?.addEventListener('click', copySelfCheckReport);
    $('scSaveButton')?.addEventListener('click', saveLog);

    $('timestampToggle')?.addEventListener('change', (e) => {
        state.timestamps = e.target.checked;
        renderAll();
    });
    $('autoscrollToggle')?.addEventListener('change', (e) => {
        state.autoscroll = e.target.checked;
        if (state.autoscroll) renderAll();
    });

    // Physical unplug during a normal session ends it cleanly. (During a startup
    // capture state.port is null, so the watch loop owns the reconnect instead.)
    if ('serial' in navigator) {
        navigator.serial.addEventListener('disconnect', (e) => {
            if (state.port && e.target === state.port) {
                abortPending('The Summit was unplugged.');
                state.keepReading = false;
                setConnected(false);
                setStatus('Device unplugged.', 'err');
                logNotice('Device was unplugged — reconnect to continue recording');
                state.port = null;
            }
        });
    }
});
