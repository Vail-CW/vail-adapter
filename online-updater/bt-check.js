// Summit Bluetooth keyboard check — guided pairing with a recorded timeline.
//
// The user pairs on the Summit's own External Keyboard screen, exactly as they
// normally would; this page tells them what to do one step at a time and
// records what the Summit reports while they do it. Needs troubleshoot.js
// (loaded first) for the serial session, sendVail/waitForVail and the log.
//
// Summit side (firmware 0.94+): "@vail bt status|mark|keys" plus a stream of
//   @vail {"t":"bt","ms":..,"ph":"<phase>","a":..,"b":..,"s":".."}
// lines while a computer is attached, and "bt_map" lines with the keyboard's
// HID Report Map.

// MODE_BT_KEYBOARD_SETTINGS in the Summit's src/core/modes.h
const BT_KB_SETTINGS_MODE = 55;

const btCheck = {
    running: false,
    status: null,          // first bt_status of the run
    lastStatus: null,      // most recent bt_status
    answers: [],           // { q, a }
    results: [],           // { id, name, st, detail, advice }
    timeline: [],          // bt messages, in arrival order, with page time
    found: [],             // { addr, name, rssi, atype } HID devices from the last scan
    others: [],            // { name, rssi, appearance } non-keyboard devices from the last scan
    keys: [],              // typed characters seen during the key test
    reports: 0,            // raw key reports seen during the key test
    reportMap: null,       // { len, hex }
    mapChunks: {},
    events: [],
    finishedAt: null,
    startedAt: null,
    listeners: [],         // fn(msg) for the step that is watching
};

// ---- phase descriptions ---------------------------------------------------
function btPhaseText(m) {
    const reason = m.s ? ` (${m.s})` : '';
    switch (m.ph) {
        case 'init':        return m.a ? `Bluetooth started, ${m.b} bytes free` : `Bluetooth could not start: ${m.s}`;
        case 'scan':        return 'Scan started';
        case 'autoscan':    return 'Looking for the saved keyboard (auto-reconnect)';
        case 'scan_fail':   return 'Scan failed to start';
        case 'found': {
            const [addr, name] = (m.s || '').split('|');
            return `Found keyboard: ${name || '?'} [${addr || '?'}] ${m.a} dBm`;
        }
        case 'other':       return `Saw non-keyboard device: ${m.s} ${m.a} dBm`;
        case 'scan_end':    return `Scan finished: ${m.a} keyboard(s), ${m.b} other named device(s)`;
        case 'seen':        return `Saved keyboard is advertising [${m.s}], reconnecting`;
        case 'connect': {
            const [addr, name] = (m.s || '').split('|');
            return `Connecting to ${name || '?'} [${addr}]${m.a ? ' (fresh pairing)' : ' (reconnect)'}`;
        }
        case 'bond_clear':  return `Cleared old pairing keys (${m.a} left)`;
        case 'forget':      return m.a ? `Forget Pairing: removed ${m.s}` : 'Forget Pairing (nothing was saved)';
        case 'attempt':     return `Connect attempt ${m.a}`;
        case 'link':        return 'Radio link up';
        case 'link_fail':
        case 'connect_fail':return `Could not connect${reason} [code 0x${(m.a >>> 0).toString(16)}]`;
        case 'pairing':     return 'Pairing / encrypting…';
        case 'passkey':     return `Keyboard asked for a PIN: type ${m.a} then Enter on the keyboard`;
        case 'passkey_in':  return 'Keyboard asked the Summit to enter a PIN';
        case 'confirm':     return `Numeric comparison ${m.a} (accepted)`;
        case 'auth':        return `Security done: encrypted=${m.a ? 'yes' : 'NO'}, bonded=${m.b & 1 ? 'yes' : 'no'}, MITM=${m.b & 2 ? 'yes' : 'no'}, key ${m.s} bytes`;
        case 'pair_ok':     return 'Pairing OK';
        case 'pair_fail':   return `Pairing failed${reason} [code 0x${(m.a >>> 0).toString(16)}]`;
        case 'pair_drop':   return m.a ? 'Gave up waiting for pairing (time limit)' : 'Keyboard dropped the link during pairing';
        case 'params':      return `Keyboard asked for interval ${m.a * 1.25} ms, timeout ${m.b * 10} ms (${m.s}), accepted`;
        case 'hid_service': return m.a ? 'Keyboard service found' : 'Keyboard service NOT found';
        case 'pnp':         return `Vendor 0x${hex4(m.a)}, product 0x${hex4(m.b)} (${m.s})`;
        case 'report_map':  return `Read report map (${m.a} bytes)`;
        case 'proto':       return 'Switched keyboard to report mode';
        case 'report_ref':  return `Input report handle ${m.a}: id ${m.b}, ${m.s}`;
        case 'subscribe':   return `${m.b ? 'Subscribed to' : 'Could NOT subscribe to'} handle ${m.a} (${m.s})`;
        case 'enc_retry':   return 'Link not encrypted, reconnecting once more';
        case 'conn_info':   return `Connected: identity ${(m.s || '').split('|')[0]}, MTU ${m.a}, interval ${m.b * 1.25} ms`;
        case 'done':        return m.a ? `Keyboard ready (took ${(m.b / 1000).toFixed(1)} s)` : `Gave up: ${m.s || 'unknown'} (after ${(m.b / 1000).toFixed(1)} s)`;
        case 'deadline':    return `Connect took longer than ${m.a} s, dropped it`;
        case 'disconnect':  return `Disconnected${reason} [code 0x${(m.a >>> 0).toString(16)}]`;
        case 'report':      return `Key report h${m.a} (${m.b} bytes): ${m.s}`;
        case 'key':         return `Key 0x${m.a.toString(16).toUpperCase()} → ${btKeyName(m.b)}`;
        case 'dropped':     return `(${m.a} trace lines lost, the Summit was busy)`;
        default:            return `${m.ph} ${m.a} ${m.b} ${m.s || ''}`;
    }
}

const BT_BAD_PHASES = new Set(['link_fail', 'connect_fail', 'pair_fail', 'pair_drop', 'deadline', 'scan_fail']);

function hex4(n) { return (n >>> 0).toString(16).toUpperCase().padStart(4, '0'); }

function btKeyName(code) {
    if (code === 13 || code === 10) return 'Enter';
    if (code === 8) return 'Backspace';
    if (code === 27) return 'Esc';
    if (code === 32) return 'Space';
    if (code >= 33 && code < 127) return String.fromCharCode(code);
    return `0x${code.toString(16)}`;
}

// ---- message hook (called by troubleshoot.js for every @vail message) -----
function btCheckOnMessage(msg) {
    if (!btCheck.running) return;
    switch (msg.t) {
        case 'bt':
            btCheck.timeline.push({ at: new Date(), ...msg });
            if (msg.ph === 'scan') { btCheck.found = []; btCheck.others = []; }
            if (msg.ph === 'found') {
                const [addr, name] = (msg.s || '').split('|');
                btCheck.found.push({ addr, name, rssi: msg.a, atype: msg.b });
            }
            if (msg.ph === 'other') btCheck.others.push({ name: msg.s, rssi: msg.a, appearance: msg.b });
            if (msg.ph === 'report') btCheck.reports++;
            if (msg.ph === 'key') btCheck.keys.push(btKeyName(msg.b));
            renderBtTimeline();
            break;
        case 'bt_map':
            btCheck.mapChunks[msg.off] = msg.hex;
            {
                const offs = Object.keys(btCheck.mapChunks).map(Number).sort((a, b) => a - b);
                const hex = offs.map((o) => btCheck.mapChunks[o]).join('');
                if (hex.length / 2 >= msg.len) btCheck.reportMap = { len: msg.len, hex };
            }
            break;
        case 'bt_status':
            btCheck.lastStatus = msg;
            break;
        case 'event':
            btCheck.events.push(msg);
            break;
        default:
            break;
    }
    for (const fn of btCheck.listeners.slice()) fn(msg);
}

// Resolve when a bt message matching pred arrives, or reject after timeoutMs
function btWait(pred, timeoutMs, timeoutMessage) {
    return new Promise((resolve, reject) => {
        const fn = (msg) => {
            if (msg.t === 'bt' && pred(msg)) { cleanup(); resolve(msg); }
        };
        const timer = setTimeout(() => { cleanup(); reject(new Error(timeoutMessage || 'Timed out.')); }, timeoutMs);
        function cleanup() {
            clearTimeout(timer);
            const i = btCheck.listeners.indexOf(fn);
            if (i >= 0) btCheck.listeners.splice(i, 1);
        }
        btCheck.listeners.push(fn);
    });
}

// Ask the Summit for a status snapshot
async function btStatus(timeoutMs = 4000) {
    const got = waitForVail((m) => m.t === 'bt_status', timeoutMs,
        "The Summit didn't answer the Bluetooth check. Update it to the newest firmware from the Home page, then try again.");
    await sendVail('bt status');
    return got;
}

async function btMark(step) {
    try { await sendVail(`bt mark ${step}`); } catch (_) {}
}

// ---- rendering ------------------------------------------------------------
function setBtStatus(text, kind) {
    const el = $('btStatus');
    if (!el) return;
    el.textContent = text;
    el.className = 'conn-status' + (kind ? ' ' + kind : '');
}

function btAsk(promptHtml, buttons, extraHtml = '') {
    const panel = $('btGuided');
    panel.style.display = 'block';
    $('btPrompt').innerHTML = promptHtml;
    $('btGuidedExtra').innerHTML = extraHtml;
    const row = $('btButtons');
    row.innerHTML = '';
    return new Promise((resolve) => {
        buttons.forEach((b) => {
            const btn = document.createElement('button');
            btn.className = b.primary ? 'btn-primary' : 'btn-secondary';
            btn.textContent = b.label;
            btn.addEventListener('click', () => resolve(b.value));
            row.appendChild(btn);
        });
    });
}

function btHideGuided() {
    const panel = $('btGuided');
    if (panel) panel.style.display = 'none';
}

function btResult(id, name, st, detail, advice = '') {
    btCheck.results = btCheck.results.filter((r) => r.id !== id);
    btCheck.results.push({ id, name, st, detail, advice });
    const list = $('btResults');
    list.innerHTML = btCheck.results.map((r) => `
        <li class="sc-item sc-${escapeHtml(r.st)}">
            <span class="sc-icon">${STATUS_ICON[r.st] || '•'}</span>
            <div>
                <div><strong>${escapeHtml(r.name)}</strong> <span class="sc-detail">${escapeHtml(r.detail)}</span></div>
                ${r.advice && (r.st === 'warn' || r.st === 'fail') ? `<div class="sc-advice">${escapeHtml(r.advice)}</div>` : ''}
            </div>
        </li>`).join('');
    list.style.display = 'block';
}

function renderBtTimeline() {
    const box = $('btTimeline');
    if (!box) return;
    const t0 = btCheck.timeline.length ? btCheck.timeline[0].ms : 0;
    box.innerHTML = btCheck.timeline
        .filter((m) => m.ph !== 'report')
        .slice(-40)
        .map((m) => `<div class="bt-line${BT_BAD_PHASES.has(m.ph) || (m.ph === 'done' && !m.a) ? ' bad' : ''}${m.ph === 'done' && m.a ? ' good' : ''}">` +
            `<span class="bt-time">+${((m.ms - t0) / 1000).toFixed(1)}s</span> ${escapeHtml(btPhaseText(m))}</div>`)
        .join('');
    box.style.display = btCheck.timeline.length ? 'block' : 'none';
    box.scrollTop = box.scrollHeight;
}

// ---- the flow -------------------------------------------------------------
async function runBtCheck() {
    if (btCheck.running || selfCheck.running || !state.port) return;
    Object.assign(btCheck, {
        running: true, status: null, lastStatus: null, answers: [], results: [], timeline: [],
        found: [], others: [], keys: [], reports: 0, reportMap: null, mapChunks: {},
        events: [], finishedAt: null, startedAt: new Date(), listeners: [],
    });
    $('btResults').innerHTML = '';
    $('btResults').style.display = 'none';
    $('btDone').style.display = 'none';
    renderBtTimeline();
    applyDeviceButtons();

    try {
        setBtStatus('Talking to the Summit…', 'wait');
        const hello = waitForVail((m) => m.t === 'hello', 4000,
            "The Summit didn't answer. Update it to the newest firmware from the Home page, then try again.");
        await sendVail('hello');
        await hello;
        btCheck.status = await btStatus();
        await btMark('start');

        if (!(await btStepKeyboardType())) return finishBtCheck();
        btStepHealth(btCheck.status);
        await btStepOpenScreen();
        await btStepFresh();
        const picked = await btStepScan();
        if (picked) {
            const ok = await btStepConnect();
            if (ok) {
                await btStepKeys();
                await btStepReconnect();
                await btStepRestart();
            }
        }
        await finishBtCheck();
    } catch (err) {
        btHideGuided();
        setBtStatus(err.message, 'err');
        btResult('stopped', 'Check stopped', 'warn', err.message);
        await finishBtCheck(true);
    } finally {
        btCheck.running = false;
        btCheck.listeners = [];
        try { await sendVail('bt keys off'); } catch (_) {}
        applyDeviceButtons();
    }
}

async function btStepKeyboardType() {
    setBtStatus('Step 1: your keyboard', 'wait');
    await btMark('keyboard_type');
    const tips = '<ul class="bt-tips">' +
        '<li>Look on the box, manual or product page for <strong>Bluetooth LE</strong>, <strong>BLE</strong>, <strong>Bluetooth 4.0</strong> or <strong>5.x</strong>.</li>' +
        '<li>Keyboards that work with an iPhone or iPad are almost always BLE.</li>' +
        '<li><strong>Bluetooth 3.0</strong> or <strong>2.1</strong> on the label means it is Classic only.</li>' +
        '</ul>';
    let a = await btAsk('The Summit can only pair with <strong>Bluetooth Low Energy (BLE)</strong> keyboards. ' +
        'Older "Bluetooth Classic" keyboards never show up on it. Does your keyboard support BLE?',
        [{ label: 'Yes', value: 'yes', primary: true }, { label: 'Not sure', value: 'unsure' },
         { label: 'No, Classic only', value: 'no' }], tips);
    btCheck.answers.push({ q: 'Keyboard supports BLE?', a });
    if (a === 'no') {
        btResult('kb_type', 'Keyboard type', 'fail', 'Bluetooth Classic only',
            'The Summit has no Bluetooth Classic radio, so this keyboard cannot pair with it. Use a keyboard that ' +
            'supports Bluetooth LE (most multi-device keyboards do), or the keyboard that came with the Summit.');
        return false;
    }
    btResult('kb_type', 'Keyboard type', a === 'yes' ? 'pass' : 'info', a === 'yes' ? 'BLE' : 'not sure, checking anyway');
    return true;
}

function btStepHealth(st) {
    if (!st) return;
    if (st.heap < st.heapNeed) {
        btResult('health', 'Summit memory', 'fail', `${st.heap} bytes free, Bluetooth needs ${st.heapNeed}`,
            'Restart the Summit and run this check before opening anything else.');
    } else if (st.tripped) {
        btResult('health', 'Summit Bluetooth', 'warn', `auto-reconnect was switched off after ${st.crashes} crash(es) while ${st.crashPhase}`,
            'This check will try pairing anyway. The report will show where it goes wrong.');
    } else {
        btResult('health', 'Summit Bluetooth', 'pass', `${st.state}, ${st.heap} bytes free` +
            (st.paired ? `, saved keyboard: ${st.name}` : ', no saved keyboard'));
    }
    if (st.wifi !== 'off') {
        btResult('wifi', 'WiFi', 'info', `${st.wifi}${st.wifiUp ? ', connected' : ''} (shares the radio with Bluetooth)`);
    }
}

async function btStepOpenScreen() {
    setBtStatus('Step 2: open the keyboard screen', 'wait');
    await btMark('open_screen');
    let st = await btStatus();
    if (st.mode === BT_KB_SETTINGS_MODE) return;
    // Watch for the user getting there; they can also click past it
    let stop = false;
    const poll = (async () => {
        while (!stop) {
            await sleep(1500);
            if (stop) return;
            try { st = await btStatus(3000); } catch (_) { continue; }
            if (st.mode === BT_KB_SETTINGS_MODE) return 'there';
        }
    })();
    const click = btAsk('On the Summit, go to <strong>Settings → Device Settings → Bluetooth → External Keyboard</strong>. ' +
        'This page moves on by itself when you get there.',
        [{ label: "I'm there", value: 'there', primary: true }]);
    await Promise.race([poll, click]);
    stop = true;
}

async function btStepFresh() {
    setBtStatus('Step 3: start fresh', 'wait');
    await btMark('fresh_start');
    const st = await btStatus();
    if (!st.paired) return;
    const a = await btAsk(`The Summit remembers <strong>${escapeHtml(st.name)}</strong>. To start clean, ` +
        'select <strong>Forget Pairing</strong> on the Summit. If your keyboard keeps a list of devices it is paired with, ' +
        'remove <strong>VAIL-SUMMIT-KB</strong> from it too.',
        [{ label: 'Done', value: 'done', primary: true }, { label: 'Keep it, skip this', value: 'skip' }]);
    btCheck.answers.push({ q: 'Forget old pairing?', a });
}

async function btStepScan() {
    for (let round = 1; ; round++) {
        setBtStatus('Step 4: find the keyboard', 'wait');
        await btMark(`scan_${round}`);
        // Only a scan the user started counts: auto-reconnect scans for a saved
        // keyboard log "scan" followed straight away by "autoscan"
        let userScan = false;
        const ended = btWait((m) => {
            if (m.ph === 'scan') userScan = true;
            else if (m.ph === 'autoscan') userScan = false;
            return m.ph === 'scan_end' && userScan;
        }, 90000, "The Summit didn't finish a scan. Make sure you selected Scan for Keyboards on the Summit.");
        const go = btAsk('Put the keyboard in <strong>pairing mode</strong>: usually hold its Bluetooth or connect key ' +
            'until a light blinks quickly. Then select <strong>Scan for Keyboards</strong> on the Summit.<br>' +
            '<span class="muted">The scan takes 10 seconds.</span>',
            [{ label: 'Skip', value: 'skip' }]);
        const r = await Promise.race([ended.then(() => 'ended'), go]);
        if (r === 'skip') {
            ended.catch(() => {});
            btResult('scan', 'Scan', 'info', 'skipped');
            return false;
        }

        const list = btCheck.found.length
            ? '<ul class="bt-tips">' + btCheck.found.map((d) => `<li>${escapeHtml(d.name)} <span class="muted">${d.rssi} dBm</span></li>`).join('') + '</ul>'
            : '<p class="muted">No keyboards were found.</p>';
        const a = await btAsk('Is your keyboard in the list the Summit found?', [
            { label: 'Yes', value: 'yes', primary: true },
            { label: 'No, scan again', value: 'again' },
            { label: 'No, give up', value: 'no' }], list);
        btCheck.answers.push({ q: `Scan ${round}: keyboard listed?`, a });
        if (a === 'yes') {
            const weak = btCheck.found.every((d) => d.rssi < -80);
            btResult('scan', 'Scan', weak ? 'warn' : 'pass', `${btCheck.found.length} keyboard(s) found`,
                weak ? 'The signal is weak. Move the keyboard closer to the Summit.' : '');
            return true;
        }
        if (a === 'no') {
            const others = btCheck.others.map((o) => o.name).join(', ');
            btResult('scan', 'Scan', 'fail', btCheck.found.length ? `${btCheck.found.length} other keyboard(s), not this one` : 'no keyboards found',
                (others ? `The Summit did see these other devices: ${others}. If your keyboard is one of them, it isn't ` +
                    'saying it is a keyboard while it advertises. ' : '') +
                'Check the keyboard is in pairing mode (fast blinking), close to the Summit, has fresh batteries, and supports Bluetooth LE. ' +
                'Turn off Bluetooth on nearby phones or computers it was paired with, so it does not connect to them instead.');
            return false;
        }
    }
}

async function btStepConnect() {
    setBtStatus('Step 5: pair', 'wait');
    await btMark('connect');
    let pinShown = false;
    const onPin = (msg) => {
        if (msg.t === 'bt' && msg.ph === 'passkey' && !pinShown) {
            pinShown = true;
            $('btGuidedExtra').innerHTML = `<div class="bt-pin">Type <strong>${msg.a}</strong> then press <strong>Enter</strong> on the keyboard</div>`;
        }
    };
    btCheck.listeners.push(onPin);
    const done = btWait((m) => m.ph === 'done', 150000, 'Pairing did not finish within 2½ minutes.');
    const skip = btAsk('On the Summit, select <strong>your keyboard</strong> in the list. ' +
        'If the keyboard wants a PIN, the number to type appears here.',
        [{ label: 'Skip', value: 'skip' }]);
    let res;
    try {
        res = await Promise.race([done, skip]);
    } finally {
        btCheck.listeners = btCheck.listeners.filter((f) => f !== onPin);
    }
    if (res === 'skip') {
        done.catch(() => {});
        btResult('pair', 'Pairing', 'info', 'skipped');
        return false;
    }
    if (res.a) {
        btResult('pair', 'Pairing', 'pass', `connected in ${(res.b / 1000).toFixed(1)} s`);
        return true;
    }
    btResult('pair', 'Pairing', 'fail', res.s || 'failed', btPairAdvice());
    return false;
}

// Advice from what happened during the last connect
function btPairAdvice() {
    const tl = btCheck.timeline;
    let i = tl.length - 1;
    while (i >= 0 && tl[i].ph !== 'connect') i--;
    const run = tl.slice(Math.max(i, 0));
    const has = (ph, pred = () => true) => run.some((m) => m.ph === ph && pred(m));
    if (has('connect_fail') || has('link_fail')) {
        return 'The keyboard did not accept a connection. Put it back into pairing mode right before selecting it, and keep it close.';
    }
    if (has('pair_drop', (m) => m.a)) {
        return 'Pairing hung until the time limit. If the keyboard asked for a PIN, type it and press Enter. Otherwise turn the keyboard off and on, put it in pairing mode again and retry.';
    }
    if (has('pair_fail') || has('pair_drop') || has('disconnect', (m) => (m.a & 0xFF) === 0x3D || m.a === 0x206)) {
        return 'The keyboard refused to pair. It may still remember the Summit from an earlier try: remove VAIL-SUMMIT-KB from the keyboard (or reset its pairings), select Forget Pairing on the Summit, then try again.';
    }
    if (has('hid_service', (m) => !m.a)) {
        return "The device didn't offer a keyboard service. It may not be a keyboard, or it only works over Bluetooth Classic.";
    }
    if (has('subscribe') && !has('subscribe', (m) => m.b)) {
        return 'The keyboard would not let the Summit listen for key presses. Please send this report so we can look at it.';
    }
    return 'Please send this report so we can see where it stopped.';
}

async function btStepKeys() {
    setBtStatus('Step 6: type on it', 'wait');
    await btMark('keys');
    btCheck.keys = [];
    btCheck.reports = 0;
    const got = waitForVail((m) => m.t === 'bt_keys' && m.on === true, 3000);
    await sendVail('bt keys 30');
    await got;
    const ended = waitForVail((m) => m.t === 'bt_keys' && m.on === false, 40000).catch(() => {});
    const typed = () => `<div class="muted">Summit received: <span class="bt-typed">${escapeHtml(btCheck.keys.join(' ')) || '(nothing yet)'}</span></div>`;
    const refresh = (msg) => { if (msg.t === 'bt' && (msg.ph === 'key' || msg.ph === 'report')) $('btGuidedExtra').innerHTML = typed(); };
    btCheck.listeners.push(refresh);
    const a = await btAsk('Type <strong>a s d 1</strong> and press <strong>Enter</strong> on the Bluetooth keyboard. ' +
        '<span class="muted">Only what you type in the next 30 seconds is recorded.</span>',
        [{ label: 'Done', value: 'done', primary: true }, { label: 'Nothing happens', value: 'nothing' }], typed());
    btCheck.listeners = btCheck.listeners.filter((f) => f !== refresh);
    await sendVail('bt keys off');
    await ended;
    btCheck.answers.push({ q: 'Typing test', a });
    if (btCheck.keys.length) {
        btResult('keys', 'Typing', 'pass', `received: ${btCheck.keys.join(' ')}`);
    } else if (btCheck.reports) {
        btResult('keys', 'Typing', 'fail', `${btCheck.reports} key report(s) arrived but none turned into letters`,
            'The keyboard sends data in a format the Summit does not understand yet. Please send this report: it includes what the keyboard sent.');
    } else {
        btResult('keys', 'Typing', 'fail', 'nothing arrived from the keyboard',
            'Connected, but the keyboard sent no key presses. Turn the keyboard off and on (it should reconnect), then try typing again. If it still sends nothing, please send this report.');
    }
}

async function btStepReconnect() {
    setBtStatus('Step 7: reconnect', 'wait');
    const go = await btAsk('Now check it comes back on its own. Click <strong>Start</strong>, then turn the keyboard ' +
        '<strong>off</strong>, wait 5 seconds, and turn it back <strong>on</strong>.',
        [{ label: 'Start', value: 'go', primary: true }, { label: 'Skip', value: 'skip' }]);
    if (go === 'skip') { btResult('reconnect', 'Reconnect', 'info', 'skipped'); return; }
    await btMark('reconnect');
    const t0 = Date.now();
    const back = btWait((m) => m.ph === 'done' && m.a, 120000, 'timeout');
    const skip = btAsk('Waiting for the keyboard to reconnect (up to 2 minutes). Press a key on it if it is asleep.',
        [{ label: 'Skip', value: 'skip' }]);
    let r;
    try { r = await Promise.race([back, skip]); } catch (_) { r = null; }
    if (r === 'skip') { back.catch(() => {}); btResult('reconnect', 'Reconnect', 'info', 'skipped'); return; }
    if (r) {
        btResult('reconnect', 'Reconnect', 'pass', `back in ${((Date.now() - t0) / 1000).toFixed(0)} s`);
    } else {
        const st = await btStatus().catch(() => null);
        btResult('reconnect', 'Reconnect', 'fail', st ? `Summit is ${st.state}${st.auto ? '' : ', auto-reconnect is OFF'}` : 'did not reconnect',
            st && !st.auto ? 'Turn Auto-Reconnect ON on the External Keyboard screen.' :
                'The Summit looks for the keyboard every 30 seconds. If it never comes back, please send this report.');
    }
}

async function btStepRestart() {
    setBtStatus('Step 8: restart', 'wait');
    const go = await btAsk('Last one: does the Summit remember the keyboard after a restart? Click <strong>Restart Summit</strong> ' +
        'and keep the keyboard on.',
        [{ label: '🔄 Restart Summit', value: 'go', primary: true }, { label: 'Skip', value: 'skip' }]);
    if (go === 'skip') { btResult('restart', 'After restart', 'info', 'skipped'); return; }
    await btMark('restart');
    await restartDevice();
    btAsk('Waiting for the Summit to start up and find the keyboard (up to 90 seconds)…', []);
    const t0 = Date.now();
    // After a restart nothing is traced until the page talks to the Summit again,
    // so keep asking for status until it answers, then watch it.
    let st = null;
    while (Date.now() - t0 < 90000) {
        await sleep(2000);
        try { st = await btStatus(1500); } catch (_) { continue; }
        if (st.state === 'connected') break;
    }
    if (st && st.state === 'connected') {
        btResult('restart', 'After restart', 'pass', `reconnected ${((Date.now() - t0) / 1000).toFixed(0)} s after restart`);
    } else {
        btResult('restart', 'After restart', 'fail', st ? `Summit is ${st.state}${st.paired ? '' : ', pairing NOT saved'}` : 'Summit did not answer after restart',
            st && !st.paired ? 'The pairing was not saved. Please send this report.' :
                'The Summit remembers the keyboard but did not reconnect. Press a key on the keyboard to wake it, and check Auto-Reconnect is ON.');
    }
}

async function finishBtCheck(quiet = false) {
    btHideGuided();
    try {
        btCheck.events = [];
        const evDone = waitForVail((m) => m.t === 'events_end', 5000);
        await sendVail('events');
        await evDone;
        await btStatus().catch(() => null);
        await btMark('end');
    } catch (_) {}
    btCheck.finishedAt = new Date();
    if (!quiet) {
        const fails = btCheck.results.filter((r) => r.st === 'fail').length;
        setBtStatus(fails ? `Finished: ${fails} problem(s). See the advice above, and send the report.` : 'Finished: the keyboard works!',
            fails ? 'err' : 'ok');
    }
    showBtDone();
}

// ---- report ---------------------------------------------------------------
function btReportText() {
    if (!btCheck.finishedAt) return '';
    const h = selfCheck.hello || {};
    const s = btCheck.status || {};
    const e = btCheck.lastStatus || {};
    const tag = { pass: 'PASS', warn: 'WARN', fail: 'FAIL', info: 'INFO' };
    const lines = [
        '=== Vail Summit Bluetooth Keyboard Check ===',
        `Date:      ${btCheck.finishedAt.toString()}`,
        `Firmware:  v${h.fw || s.fw || '?'} (${h.date || '?'})`,
        `At start:  state ${s.state}, saved keyboard ${s.paired ? `${s.name} [${s.addr} type ${s.atype}]` : 'none'}, ` +
            `auto-reconnect ${s.auto ? 'on' : 'off'}${s.tripped ? ' (tripped by crash guard)' : ''}, crashes ${s.crashes} (${s.crashPhase}), ` +
            `bonds ${s.bonds}, heap ${s.heap}/${s.heapNeed}, WiFi ${s.wifi}${s.wifiUp ? ' connected' : ''}, last error "${s.err || ''}"`,
        `At end:    state ${e.state}, saved keyboard ${e.paired ? `${e.name} [${e.addr} type ${e.atype}]` : 'none'}, ` +
            `bonds ${e.bonds}, heap ${e.heap}, connected ${e.connSecs ?? 0} s, last error "${e.err || ''}"`,
        '',
        'Results:',
    ];
    for (const r of btCheck.results) {
        lines.push(`  [${tag[r.st] || r.st}] ${r.name}: ${r.detail}`);
        if (r.advice && (r.st === 'warn' || r.st === 'fail')) lines.push(`         -> ${r.advice}`);
    }
    lines.push('', 'Answers:');
    for (const a of btCheck.answers) lines.push(`  ${a.q} ${a.a}`);
    lines.push('', 'Keyboards found in last scan:');
    if (!btCheck.found.length) lines.push('  (none)');
    for (const d of btCheck.found) lines.push(`  ${d.name} [${d.addr} type ${d.atype}] ${d.rssi} dBm`);
    if (btCheck.others.length) {
        lines.push('Other named devices seen:');
        for (const o of btCheck.others) lines.push(`  ${o.name} ${o.rssi} dBm appearance ${o.appearance}`);
    }
    lines.push('', 'Timeline (Summit time):');
    const t0 = btCheck.timeline.length ? btCheck.timeline[0].ms : 0;
    for (const m of btCheck.timeline) {
        lines.push(`  +${((m.ms - t0) / 1000).toFixed(2).padStart(7)}s  ${m.ph.padEnd(12)} ${btPhaseText(m)}`);
    }
    if (btCheck.reportMap) {
        lines.push('', `HID report map (${btCheck.reportMap.len} bytes):`);
        for (let i = 0; i < btCheck.reportMap.hex.length; i += 64) {
            lines.push('  ' + btCheck.reportMap.hex.slice(i, i + 64).replace(/(..)/g, '$1 ').trim());
        }
    }
    lines.push('', 'Event history (since power-on):');
    if (!btCheck.events.length) lines.push('  (none)');
    for (const ev of btCheck.events) lines.push(`  boot ${ev.boot} +${(ev.ms / 1000).toFixed(1)}s  ${ev.tag}: ${ev.msg}`);
    lines.push('============================================');
    return lines.join('\n');
}

function showBtDone() {
    $('btDone').style.display = 'block';
    const h = selfCheck.hello || {};
    const problems = btCheck.results.filter((r) => r.st === 'fail' || r.st === 'warn').map((r) => `${r.name}: ${r.detail}`);
    const subject = `Summit Bluetooth keyboard check v${h.fw || '?'}${h.call ? ' ' + h.call : ''}`;
    const body = 'Hi, here is my Summit Bluetooth keyboard check.\n\nKeyboard make and model: \n\n' +
        (problems.length ? 'Problems found:\n- ' + problems.join('\n- ') + '\n\n' : 'Everything passed.\n\n') +
        'Full report: (click "Copy report" on the page, then paste it here with Ctrl+V, or attach the saved file)\n';
    $('btEmailLink').href = `${SELF_CHECK_EMAIL_URL}?subject=${encodeURIComponent(subject)}&body=${encodeURIComponent(body)}`;
}

async function copyBtReport() {
    try {
        await navigator.clipboard.writeText(btReportText() + '\n\n=== Full device log ===\n' + fullText());
        const btn = $('btCopyButton');
        const original = btn.textContent;
        btn.textContent = '✅ Copied!';
        setTimeout(() => { btn.textContent = original; }, 1500);
    } catch (err) {
        alert(`Could not copy: ${err.message}`);
    }
}

document.addEventListener('DOMContentLoaded', () => {
    $('btRunButton')?.addEventListener('click', runBtCheck);
    $('btCopyButton')?.addEventListener('click', copyBtReport);
    $('btSaveButton')?.addEventListener('click', saveLog);
});
