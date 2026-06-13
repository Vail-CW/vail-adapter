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

        // Handshake: enter binary mode and confirm an Arduino XYZ bootloader.
        async connect() {
            this.rx = '';
            await this._sendStr('N#');
            try { await this._waitFor('\n\r', 1500); } catch (_) { /* some bootloaders are silent here */ }
            await this._sendStr('V#');
            const version = (await this._waitFor('\n\r', 3000)).trim();
            this.log(`Bootloader: ${version}`);
            if (!/Arduino:XYZ/i.test(version)) {
                throw new Error('This port is not a SAMD21 Arduino-style bootloader (no [Arduino:XYZ]). Make sure you selected the bootloader COM port.');
            }
            return version;
        }

        async eraseApp() {
            this.log('Erasing application flash…');
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
        for (let pos = 0; pos + 512 <= arrayBuffer.byteLength; pos += 512) {
            const magic0 = view.getUint32(pos + 0, true);
            const magic1 = view.getUint32(pos + 4, true);
            const magicEnd = view.getUint32(pos + 508, true);
            if (magic0 !== 0x0A324655 || magic1 !== 0x9E5D5157 || magicEnd !== 0x0AB16F30) continue;
            const flags = view.getUint32(pos + 8, true);
            const addr = view.getUint32(pos + 12, true);
            const payloadSize = view.getUint32(pos + 16, true);
            if (flags & 0x00000001) continue; // "not main flash" block
            const data = new Uint8Array(arrayBuffer, pos + 32, payloadSize);
            blocks.push({ addr, data });
            if (addr < minAddr) minAddr = addr;
            if (addr + payloadSize > maxEnd) maxEnd = addr + payloadSize;
        }
        if (!blocks.length) throw new Error('No valid UF2 blocks found in firmware file.');
        const bin = new Uint8Array(maxEnd - minAddr).fill(0xFF);
        for (const b of blocks) bin.set(b.data, b.addr - minAddr);
        return { bin, baseAddr: minAddr };
    }

    window.SAMBAFlasher = SAMBAFlasher;
    window.uf2ToBin = uf2ToBin;
})();
