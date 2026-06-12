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
    const body = state.lines.map(formatLine).join('\n');
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
async function restartDevice() {
    if (!state.port) return;
    try {
        logNotice('Restarting device…');
        await state.port.setSignals({ dataTerminalReady: false, requestToSend: true });
        await sleep(120);
        await state.port.setSignals({ dataTerminalReady: false, requestToSend: false });
        await sleep(50);
        await state.port.setSignals({ dataTerminalReady: true });
    } catch (err) {
        logNotice(`Restart did not work: ${err.message}. Try the physical reset button.`);
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
    const header = [
        '=== Vail Troubleshooting Recording ===',
        `Device:    ${deviceLabel}`,
        `Baud rate: ${state.baud || '(not set)'}`,
        `Recorded:  ${new Date().toString()}`,
        `Browser:   ${navigator.userAgent}`,
        '======================================',
        '',
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
        await navigator.clipboard.writeText(fullText());
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

// ---- per-device hint copy -------------------------------------------------
function deviceHint() {
    if (state.device === 'adapter') {
        return '💡 To record the startup messages, click <strong>Capture Startup</strong>: ' +
               'pick your adapter, then unplug it and plug it back in — recording starts ' +
               'the instant it reconnects. Or click <strong>Connect</strong> to just watch live.';
    }
    if (state.device === 'summit') {
        return '💡 Click <strong>Connect</strong>, then use <strong>Restart Device</strong> to ' +
               'reboot the Summit so its startup messages get recorded too.';
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
        card.addEventListener('click', () => {
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
                state.keepReading = false;
                setConnected(false);
                setStatus('Device unplugged.', 'err');
                logNotice('Device was unplugged — reconnect to continue recording');
                state.port = null;
            }
        });
    }
});
