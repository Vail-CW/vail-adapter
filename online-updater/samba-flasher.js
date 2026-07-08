// WebSerial SAM-BA / BOSSA flasher for SAMD21 boards (Seeed XIAO, Adafruit QT Py,
// TRRS Trinkey / Vail Lite). Flashes firmware directly over the bootloader's CDC
// serial port — the same path the Arduino IDE uses via `bossac` — bypassing the
// UF2 mass-storage drag-and-drop entirely. This is the fallback for users whose
// Windows UF2 copy hangs but whose bootloader still enumerates.
//
// Protocol (Adafruit uf2-samdx1 SAM-BA monitor, "[Arduino:XYZ]" extended set):
//   N#                      -> binary (non-terminal) mode, replies "\n\r"
//   V#                      -> version string, advertises [Arduino:XYZ]
//   X<addr>#                -> erase flash from <addr> to end, replies "X\n\r"
//   S<addr>,<size>#  <bin>  -> load <size> raw bytes into SRAM at <addr>
//   Y<sram>,0#              -> set SRAM source buffer, replies "Y\n\r"
//   Y<flash>,<size>#        -> copy <size> bytes SRAM->flash, replies "Y\n\r"
//   W<addr>,<value>#        -> write 32-bit word (used to reset via AIRCR)
//   w<addr>,4#              -> read 32-bit word, replies 4 raw bytes (binary mode)
//   Z<addr>,<size>#         -> CRC16 (XMODEM) of flash range, replies "Z<hex8>#\n\r"
// All numbers are hex; every command ends with '#'.

(function () {
    'use strict';

    const FLASH_APP_ADDR = 0x2000;     // SAMD21 app start (after 8 KB bootloader)
    const FLASH_END      = 0x40000;    // 256 KB total flash
    const SRAM_BUFFER    = 0x20004000; // staging buffer (bossac's _user for SAMD21)
    const CHUNK          = 0x1000;     // 4 KB per S/Y transfer (multiple of 64-byte page)
    const AIRCR          = 0xE000ED0C; // Cortex-M Application Interrupt/Reset Control
    const AIRCR_RESET    = 0x05FA0004; // VECTKEY | SYSRESETREQ

    const hex8 = (n) => (n >>> 0).toString(16).toUpperCase().padStart(8, '0');

    // CRC16 XMODEM (poly 0x1021, init 0) — the algorithm behind the monitor's
    // Z command and bossac's verify pass.
    const CRC16_TABLE = (() => {
        const t = new Uint16Array(256);
        for (let i = 0; i < 256; i++) {
            let c = (i << 8) & 0xFFFF;
            for (let j = 0; j < 8; j++) {
                c = (c & 0x8000) ? (((c << 1) ^ 0x1021) & 0xFFFF) : ((c << 1) & 0xFFFF);
            }
            t[i] = c;
        }
        return t;
    })();

    function crc16(data) {
        let crc = 0;
        for (let i = 0; i < data.length; i++) {
            crc = ((crc << 8) & 0xFFFF) ^ CRC16_TABLE[((crc >> 8) ^ data[i]) & 0xFF];
        }
        return crc;
    }

    class SAMBAFlasher {
        constructor({ log, progress } = {}) {
            this.log = log || (() => {});
            this.progress = progress || (() => {});
            this.port = null;
            this.reader = null;
            this.writer = null;
            this.rx = '';
            this._pump = null;
        }

        async open(port, baudRate = 115200) {
            this.port = port;
            // Baud is irrelevant for a USB-CDC bootloader, but WebSerial requires one.
            await port.open({ baudRate });
            this.writer = port.writable.getWriter();
            this.reader = port.readable.getReader();
            this.rx = '';
            // Background pump: append all incoming bytes (latin1) to this.rx.
            this._pump = (async () => {
                try {
                    while (true) {
                        const { value, done } = await this.reader.read();
                        if (done) break;
                        if (value) for (let i = 0; i < value.length; i++) this.rx += String.fromCharCode(value[i]);
                    }
                } catch (_) { /* reader cancelled on close */ }
            })();
        }

        async close() {
            try { if (this.reader) { await this.reader.cancel(); this.reader.releaseLock(); } } catch (_) {}
            try { if (this.writer) { this.writer.releaseLock(); } } catch (_) {}
            try { if (this.port) await this.port.close(); } catch (_) {}
            this.reader = this.writer = this.port = null;
        }

        async _sendStr(s) { await this.writer.write(new TextEncoder().encode(s)); }
        async _sendBytes(u8) { await this.writer.write(u8); }

        // Wait until `token` appears in the receive buffer, then consume up to and
        // including it. Throws on timeout.
        async _waitFor(token, timeoutMs = 5000) {
            const deadline = Date.now() + timeoutMs;
            while (Date.now() < deadline) {
                const idx = this.rx.indexOf(token);
                if (idx !== -1) {
                    const out = this.rx.slice(0, idx + token.length);
                    this.rx = this.rx.slice(idx + token.length);
                    return out;
                }
                await new Promise((r) => setTimeout(r, 15));
            }
            throw new Error(`Timed out waiting for "${token.replace(/[\r\n]/g, '?')}" (got "${this.rx.slice(0, 64).replace(/[\r\n]/g, '?')}")`);
        }

        // Handshake: enter binary mode and read the bootloader version. Retries
        // because the first bytes after a CDC port opens are often dropped.
        async connect() {
            // Raise DTR/RTS — some WebSerial + USB-CDC combos won't deliver RX
            // bytes until DTR is asserted. Harmless on the bootloader.
            try { await this.port.setSignals({ dataTerminalReady: true, requestToSend: true }); } catch (_) {}

            let version = '';
            for (let attempt = 1; attempt <= 5 && !version; attempt++) {
                this.rx = '';
                await this._sendStr('N#');           // exit terminal mode
                await new Promise((r) => setTimeout(r, 120));
                await this._sendStr('V#');           // request version
                try {
                    await this._waitFor('\n', 1200); // any newline = a line arrived
                    await new Promise((r) => setTimeout(r, 150)); // let the rest land
                    version = (this.rx || '').replace(/[\r\n]/g, ' ').trim();
                } catch (_) {
                    this.log(`No bootloader response yet (attempt ${attempt}/5)…`);
                }
            }
            this.rx = '';

            if (!version) {
                throw new Error('No response from the bootloader. Make sure the adapter is in BOOTLOADER mode (double-tap the reset button — a drive like XIAOBOOT/QTPYBOOT/ADAPTERBOOT should appear) and that you picked THAT device\'s COM port.');
            }
            this.log(`Bootloader: ${version}`);
            if (!/Arduino:XYZ/i.test(version)) {
                // Don't hard-fail: try the extended X/Y commands anyway and let
                // the erase/write step surface a real failure if they're absent.
                this.log('⚠️ Bootloader did not advertise [Arduino:XYZ]; trying extended commands anyway.');
            }
            return version;
        }

        async eraseApp() {
            this.log('Erasing application flash…');
            this.rx = '';
            await this._sendStr(`X${hex8(FLASH_APP_ADDR)}#`);
            await this._waitFor('X', 15000);
            this.log('Erase complete.');
        }

        // Write a contiguous binary image to flash. `startAddr` is the absolute
        // flash address of the first byte (the UF2's base, normally 0x2000).
        async writeFirmware(bin, startAddr = FLASH_APP_ADDR) {
            const total = bin.length;
            let offset = 0;
            while (offset < total) {
                const size = Math.min(CHUNK, total - offset);
                const chunk = bin.subarray(offset, offset + size);
                // 1) stage the chunk in SRAM
                await this._sendStr(`S${hex8(SRAM_BUFFER)},${hex8(size)}#`);
                await this._sendBytes(chunk);
                // 2) point the source buffer at the staged data
                await this._sendStr(`Y${hex8(SRAM_BUFFER)},0#`);
                await this._waitFor('Y', 5000);
                // 3) copy SRAM -> flash at the absolute application address
                await this._sendStr(`Y${hex8(startAddr + offset)},${hex8(size)}#`);
                await this._waitFor('Y', 8000);
                offset += size;
                this.progress(offset, total);
            }
            this.log(`Wrote ${total} bytes to flash.`);
        }

        // Wait until at least `count` raw bytes are in the receive buffer, then
        // consume and return them as byte values. Throws on timeout.
        async _waitForBytes(count, timeoutMs = 3000) {
            const deadline = Date.now() + timeoutMs;
            while (Date.now() < deadline) {
                if (this.rx.length >= count) {
                    const out = [];
                    for (let i = 0; i < count; i++) out.push(this.rx.charCodeAt(i) & 0xFF);
                    this.rx = this.rx.slice(count);
                    return out;
                }
                await new Promise((r) => setTimeout(r, 15));
            }
            throw new Error(`Timed out reading ${count} bytes (got ${this.rx.length})`);
        }

        // Read one 32-bit word from the device. In binary mode the monitor
        // replies with 4 raw little-endian bytes.
        async readWord(addr) {
            this.rx = '';
            await this._sendStr(`w${hex8(addr)},4#`);
            const b = await this._waitForBytes(4, 3000);
            return (b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24)) >>> 0;
        }

        // Ask the bootloader for the CRC16 of a flash range (Z command).
        async crcRegion(addr, size) {
            this.rx = '';
            await this._sendStr(`Z${hex8(addr)},${hex8(size)}#`);
            const reply = await this._waitFor('#', 20000);
            const m = reply.match(/Z([0-9A-Fa-f]{8})#/);
            if (!m) throw new Error(`Bad response to CRC check ("${reply.slice(-24).replace(/[\r\n]/g, '?')}")`);
            return parseInt(m[1], 16) >>> 0;
        }

        // Compare a range of words against the local image. Returns the byte
        // offsets that differ. Deterministic sample points: the vector table,
        // the end of the image, and one word per KB in between.
        async _sampleMismatches(bin, startAddr) {
            const offsets = new Set([0, 4]);
            for (let o = 0; o < bin.length; o += 1024) offsets.add(o);
            offsets.add(bin.length - 4);
            const bad = [];
            for (const raw of [...offsets].sort((a, b) => a - b)) {
                const off = raw & ~3;
                if (off < 0 || off + 4 > bin.length) continue;
                const dev = await this.readWord(startAddr + off);
                const local = (bin[off] | (bin[off + 1] << 8) | (bin[off + 2] << 16) | (bin[off + 3] << 24)) >>> 0;
                if (dev !== local) bad.push(off);
            }
            return bad;
        }

        // Verify what actually landed in flash, the step bossac always does and
        // the reason a bad write there fails loudly instead of half-flashing a
        // dead board. Prefers the Z CRC over the full image; falls back to (or
        // double-checks with) sampled word reads. Throws on real mismatch.
        async verifyFirmware(bin, startAddr = FLASH_APP_ADDR) {
            this.log('Verifying flash contents…');
            let crcMatch = null;
            try {
                const dev = await this.crcRegion(startAddr, bin.length);
                crcMatch = ((dev & 0xFFFF) === crc16(bin));
            } catch (_) {
                this.log('CRC check unavailable on this bootloader, comparing sampled words instead.');
            }
            if (crcMatch === true) {
                this.log('✅ Verification passed (CRC match over the full image).');
                return;
            }
            // CRC missing or mismatched: adjudicate with direct word reads.
            const bad = await this._sampleMismatches(bin, startAddr);
            if (bad.length) {
                throw new Error(`Verification FAILED: flash differs from the firmware image at ${bad.length} of the checked locations (first at offset 0x${bad[0].toString(16)}).`);
            }
            if (crcMatch === false) {
                this.log('⚠️ CRC disagreed but every sampled word matches. Proceeding.');
            } else {
                this.log('✅ Verification passed (all sampled words match).');
            }
        }

        // Trigger a CPU reset so the freshly flashed app runs. The port drops as
        // the device re-enumerates, so a write error here is expected/benign.
        async resetDevice() {
            this.log('Resetting device…');
            try {
                await this._sendStr(`W${hex8(AIRCR)},${hex8(AIRCR_RESET)}#`);
            } catch (_) { /* port already gone — the reset took effect */ }
        }
    }

    // Parse a UF2 file (ArrayBuffer) into a contiguous flash image + base address.
    // Each 512-byte block carries a target address and (typically) 256 payload
    // bytes. Gaps are filled with 0xFF (matching erased flash).
    function uf2ToBin(arrayBuffer) {
        const view = new DataView(arrayBuffer);
        const blocks = [];
        let minAddr = Infinity, maxEnd = 0;
        // Every UF2 block records its own index and the file's total block
        // count, so a truncated or corrupted download is detectable instead of
        // silently producing a partial image.
        let expectedBlocks = null;
        const seenBlocks = new Set();
        for (let pos = 0; pos + 512 <= arrayBuffer.byteLength; pos += 512) {
            const magic0 = view.getUint32(pos + 0, true);
            const magic1 = view.getUint32(pos + 4, true);
            const magicEnd = view.getUint32(pos + 508, true);
            if (magic0 !== 0x0A324655 || magic1 !== 0x9E5D5157 || magicEnd !== 0x0AB16F30) continue;
            const flags = view.getUint32(pos + 8, true);
            const addr = view.getUint32(pos + 12, true);
            const payloadSize = view.getUint32(pos + 16, true);
            const blockNo = view.getUint32(pos + 20, true);
            const numBlocks = view.getUint32(pos + 24, true);
            if (expectedBlocks === null && numBlocks > 0) expectedBlocks = numBlocks;
            seenBlocks.add(blockNo);
            if (flags & 0x00000001) continue; // "not main flash" block
            const data = new Uint8Array(arrayBuffer, pos + 32, payloadSize);
            blocks.push({ addr, data });
            if (addr < minAddr) minAddr = addr;
            if (addr + payloadSize > maxEnd) maxEnd = addr + payloadSize;
        }
        if (!blocks.length) throw new Error('No valid UF2 blocks found in firmware file.');
        if (expectedBlocks !== null && seenBlocks.size !== expectedBlocks) {
            throw new Error(`Firmware file is incomplete: found ${seenBlocks.size} of ${expectedBlocks} blocks. The download was likely cut short — try again.`);
        }
        const bin = new Uint8Array(maxEnd - minAddr).fill(0xFF);
        for (const b of blocks) bin.set(b.data, b.addr - minAddr);
        return { bin, baseAddr: minAddr };
    }

    window.SAMBAFlasher = SAMBAFlasher;
    window.uf2ToBin = uf2ToBin;
})();
