// AVR109 / Butterfly bootloader flasher (Web Serial)
//
// Targets Arduino Micro / Leonardo-style boards running the Caterina
// bootloader. Flow:
//   1. Open the running application port at 1200 baud, then close it —
//      this magic tap asks Caterina to jump into the bootloader.
//   2. Caterina re-enumerates on a different USB PID for ~8 seconds. The
//      user selects that new port (web pages cannot enumerate by VID/PID).
//   3. We speak AVR109 to that port to program flash, then tell the
//      bootloader to run the application.
//
// This file has no dependencies and exposes `window.AVR109Flasher`.

(function () {
    'use strict';

    // --- Intel HEX parser ---------------------------------------------------

    function parseIntelHex(text) {
        // Returns a Uint8Array byte image starting at address 0, with gaps
        // filled with 0xFF (Caterina's erase state).
        const bytes = [];
        let baseAddr = 0;
        let maxAddr = 0;

        for (let rawLine of text.split(/\r?\n/)) {
            const line = rawLine.trim();
            if (!line || line[0] !== ':') continue;
            if (line.length < 11) throw new Error('Malformed HEX line: ' + line);

            const byteCount = parseInt(line.substr(1, 2), 16);
            const addr = parseInt(line.substr(3, 4), 16);
            const recType = parseInt(line.substr(7, 2), 16);

            if (recType === 0x00) { // data
                const target = baseAddr + addr;
                while (bytes.length < target) bytes.push(0xFF);
                for (let i = 0; i < byteCount; i++) {
                    bytes[target + i] = parseInt(line.substr(9 + i * 2, 2), 16);
                }
                if (target + byteCount > maxAddr) maxAddr = target + byteCount;
            } else if (recType === 0x01) { // EOF
                break;
            } else if (recType === 0x02) { // extended segment address
                baseAddr = parseInt(line.substr(9, 4), 16) << 4;
            } else if (recType === 0x04) { // extended linear address
                baseAddr = parseInt(line.substr(9, 4), 16) << 16;
            }
            // record types 03, 05 (entry points) are ignored
        }

        // Truncate to maxAddr so padding 0xFF at the tail doesn't waste writes
        while (bytes.length > maxAddr) bytes.pop();
        return new Uint8Array(bytes);
    }

    // --- Reader helper with timeout ----------------------------------------

    class TimedReader {
        constructor(port) {
            this.port = port;
            this.reader = port.readable.getReader();
            this.buffer = [];
        }

        async readBytes(count, timeoutMs = 2000) {
            const deadline = Date.now() + timeoutMs;
            while (this.buffer.length < count) {
                const remaining = deadline - Date.now();
                if (remaining <= 0) {
                    throw new Error(
                        `AVR109 read timeout (got ${this.buffer.length}/${count} bytes)`
                    );
                }
                let timer;
                const timeout = new Promise((_, rej) => {
                    timer = setTimeout(
                        () => rej(new Error('timeout')),
                        remaining
                    );
                });
                try {
                    const { value, done } = await Promise.race([
                        this.reader.read(),
                        timeout,
                    ]);
                    clearTimeout(timer);
                    if (done) throw new Error('Serial port closed');
                    if (value) {
                        for (const b of value) this.buffer.push(b);
                    }
                } catch (err) {
                    clearTimeout(timer);
                    if (err.message !== 'timeout') throw err;
                }
            }
            return this.buffer.splice(0, count);
        }

        async release() {
            try { this.reader.releaseLock(); } catch (e) { /* ignore */ }
        }
    }

    // --- AVR109 protocol ----------------------------------------------------

    class AVR109Flasher {
        constructor({ log, progress } = {}) {
            this.log = log || (() => {});
            this.progress = progress || (() => {});
            this.port = null;
            this.reader = null;
            this.writer = null;
        }

        async openPort(port, baudRate = 57600) {
            this.port = port;
            await port.open({ baudRate });
            // Small settle delay — Caterina USB CDC is fussy about writing
            // immediately on open.
            await new Promise(r => setTimeout(r, 150));
            this.reader = new TimedReader(port);
            this.writer = port.writable.getWriter();
        }

        async close() {
            try { if (this.reader) await this.reader.release(); } catch (e) {}
            try { if (this.writer) this.writer.releaseLock(); } catch (e) {}
            try { if (this.port) await this.port.close(); } catch (e) {}
            this.reader = null;
            this.writer = null;
            this.port = null;
        }

        async writeBytes(bytes) {
            await this.writer.write(new Uint8Array(bytes));
        }

        async expect(byte, what) {
            const [b] = await this.reader.readBytes(1, 2000);
            if (b !== byte) {
                throw new Error(
                    `${what}: expected 0x${byte.toString(16)}, got 0x${b.toString(16)}`
                );
            }
        }

        async expectCR(what) {
            await this.expect(0x0D, what);
        }

        // --- AVR109 commands ------------------------------------------------

        async identify() {
            await this.writeBytes([0x53]); // 'S' — return programmer ID (7 bytes)
            const id = await this.reader.readBytes(7, 2000);
            this.log(`Programmer ID: "${String.fromCharCode(...id)}"`);

            await this.writeBytes([0x56]); // 'V' — software version (2 bytes)
            const ver = await this.reader.readBytes(2, 2000);
            this.log(`Bootloader version: ${String.fromCharCode(...ver)}`);

            await this.writeBytes([0x70]); // 'p' — return programmer type
            const type = await this.reader.readBytes(1, 2000);
            this.log(`Programmer type: "${String.fromCharCode(type[0])}"`);
        }

        async checkBlockSupport() {
            // 'b' → 'Y' + high + low = supported with block size
            //       'N' = block mode not supported
            await this.writeBytes([0x62]);
            const yn = await this.reader.readBytes(1, 2000);
            if (yn[0] !== 0x59) {
                throw new Error('Bootloader does not support block write mode');
            }
            const [hi, lo] = await this.reader.readBytes(2, 2000);
            const blockSize = (hi << 8) | lo;
            this.log(`Block write supported, block size = ${blockSize} bytes`);
            return blockSize;
        }

        async enterProgMode() {
            await this.writeBytes([0x50]); // 'P'
            await this.expectCR('enter programming mode');
        }

        async leaveProgMode() {
            await this.writeBytes([0x4C]); // 'L'
            await this.expectCR('leave programming mode');
        }

        async setAddress(wordAddress) {
            // 'A' + hi + lo — address is in words (not bytes) for flash ops
            await this.writeBytes([
                0x41,
                (wordAddress >> 8) & 0xFF,
                wordAddress & 0xFF,
            ]);
            await this.expectCR('set address');
        }

        async writeBlock(data) {
            // 'B' + size_hi + size_lo + 'F' + data
            const sz = data.length;
            const header = [0x42, (sz >> 8) & 0xFF, sz & 0xFF, 0x46];
            await this.writeBytes(header);
            await this.writeBytes(data);
            await this.expectCR('write block');
        }

        async exitBootloader() {
            await this.writeBytes([0x45]); // 'E' — exit & run app
            await this.expectCR('exit bootloader');
        }

        async flashHex(hexText) {
            this.log('Parsing Intel HEX…');
            const image = parseIntelHex(hexText);
            this.log(`Firmware image: ${image.length} bytes`);

            await this.identify();
            const blockSize = await this.checkBlockSupport();
            await this.enterProgMode();

            const total = Math.ceil(image.length / blockSize);
            this.log(`Writing ${total} blocks of ${blockSize} bytes each…`);

            for (let i = 0; i < total; i++) {
                const offset = i * blockSize;
                const chunk = new Uint8Array(blockSize);
                chunk.fill(0xFF);
                const end = Math.min(offset + blockSize, image.length);
                chunk.set(image.subarray(offset, end), 0);

                // Word address = byte address / 2 on AVR
                await this.setAddress(offset >> 1);
                await this.writeBlock(Array.from(chunk));
                this.progress(i + 1, total);
            }

            await this.leaveProgMode();
            await this.exitBootloader();
            this.log('✅ Flash complete — bootloader will now run the application.');
        }
    }

    // --- 1200-baud touch helper --------------------------------------------
    //
    // Called on the *application* port to force Caterina into bootloader
    // mode. After this the Micro disappears from USB and re-appears on a
    // new port for ~8 seconds. The caller must then ask the user to pick
    // the new bootloader port.

    async function touch1200(port) {
        await port.open({ baudRate: 1200 });
        // Most reference implementations skip an explicit sleep here and
        // rely on open()→close() being enough for Caterina's bootloader
        // entry to trigger. A tiny delay avoids some platform-specific
        // flush quirks.
        await new Promise(r => setTimeout(r, 100));
        await port.close();
    }

    window.AVR109Flasher = AVR109Flasher;
    window.avr109Touch1200 = touch1200;
    window.parseIntelHex = parseIntelHex;
})();
