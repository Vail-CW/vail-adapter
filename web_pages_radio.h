/*
 * Web Page - Radio Control
 * HTML/CSS/JavaScript for radio control page
 * Includes CW memory management interface
 */

#ifndef WEB_PAGES_RADIO_H
#define WEB_PAGES_RADIO_H

const char RADIO_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Radio Control - VAIL SUMMIT</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #fff;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 800px; margin: 0 auto; }
        header {
            text-align: center;
            margin-bottom: 30px;
            padding: 20px;
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            backdrop-filter: blur(10px);
        }
        h1 { font-size: 2rem; margin-bottom: 10px; }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 25px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
            margin-bottom: 20px;
        }
        .card h2 { font-size: 1.5rem; margin-bottom: 15px; }
        .status-badge {
            display: inline-block;
            padding: 8px 16px;
            border-radius: 8px;
            font-size: 0.9rem;
            font-weight: 600;
            margin-bottom: 15px;
        }
        .status-inactive { background: rgba(255,255,255,0.2); }
        .status-active { background: #00d4ff; color: #1e3c72; }
        .form-group { margin-bottom: 20px; }
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            font-size: 0.9rem;
            text-transform: uppercase;
            letter-spacing: 1px;
            opacity: 0.9;
        }
        textarea {
            width: 100%;
            padding: 12px;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            background: rgba(255,255,255,0.1);
            color: #fff;
            font-size: 1rem;
            font-family: 'Courier New', monospace;
            resize: vertical;
            min-height: 100px;
        }
        textarea::placeholder { color: rgba(255,255,255,0.5); }
        .btn {
            display: inline-block;
            padding: 12px 24px;
            background: rgba(255,255,255,0.2);
            color: #fff;
            text-decoration: none;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            transition: all 0.2s;
            cursor: pointer;
            font-size: 1rem;
            font-weight: 600;
            margin-right: 10px;
        }
        .btn:hover {
            background: rgba(255,255,255,0.3);
            border-color: rgba(255,255,255,0.5);
        }
        .btn-primary {
            background: #00d4ff;
            color: #1e3c72;
            border: none;
        }
        .btn-primary:hover { background: #00b8e6; }
        .btn-danger {
            background: #ff4444;
            color: #fff;
            border: none;
        }
        .btn-danger:hover { background: #cc0000; }
        .message {
            padding: 12px;
            border-radius: 8px;
            margin-bottom: 15px;
            display: none;
        }
        .message.success { background: rgba(0,255,0,0.2); border: 1px solid rgba(0,255,0,0.5); }
        .message.error { background: rgba(255,0,0,0.2); border: 1px solid rgba(255,0,0,0.5); }
        .char-count {
            text-align: right;
            font-size: 0.85rem;
            opacity: 0.7;
            margin-top: 5px;
        }
        .info-box {
            background: rgba(0,212,255,0.1);
            border-left: 4px solid #00d4ff;
            padding: 15px;
            border-radius: 8px;
            margin-top: 20px;
        }
        .info-box p { opacity: 0.9; line-height: 1.6; margin-bottom: 10px; }
        .info-box p:last-child { margin-bottom: 0; }
        select {
            width: 100%;
            padding: 12px;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            background: rgba(255,255,255,0.15);
            color: #fff;
            font-size: 1rem;
            cursor: pointer;
        }
        select option { background: #1e3c72; color: #fff; }
        input[type="text"] {
            width: 100%;
            padding: 12px;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            background: rgba(255,255,255,0.1);
            color: #fff;
            font-size: 1rem;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>📻 Radio Control</h1>
            <p>Send morse code messages to your connected radio</p>
        </header>

        <div class="card">
            <h2>Radio Mode Status</h2>
            <span class="status-badge" id="radioStatus">Checking...</span>
            <div id="messageBox" class="message"></div>

            <div style="margin-top: 20px;">
                <button class="btn btn-primary" id="enterRadioBtn" onclick="enterRadioMode()">
                    Enter Radio Mode
                </button>
                <button class="btn" onclick="window.location.href='/'">
                    Back to Dashboard
                </button>
            </div>
        </div>

        <div class="card">
            <h2>Transmission Settings</h2>
            <div class="form-group">
                <label for="wpmSlider">Speed (WPM)</label>
                <div style="display: flex; align-items: center; gap: 15px;">
                    <input type="range" id="wpmSlider" min="5" max="40" value="20"
                           style="flex: 1; height: 8px; border-radius: 4px; background: rgba(255,255,255,0.2); cursor: pointer;">
                    <span id="wpmDisplay" style="font-size: 1.5rem; font-weight: 600; min-width: 60px; text-align: right;">20 WPM</span>
                </div>
            </div>
        </div>

        <div class="card">
            <h2>Send Morse Code Message</h2>
            <div class="form-group">
                <label for="messageInput">Message to Send</label>
                <textarea id="messageInput" placeholder="Enter your message here (A-Z, 0-9, basic punctuation)"></textarea>
                <div class="char-count">
                    <span id="charCount">0</span> characters
                </div>
            </div>

            <button class="btn btn-primary" onclick="sendMessage()">
                Send Message
            </button>

            <div class="info-box">
                <p><strong>📡 Radio Output:</strong> Messages will be sent as morse code via the 3.5mm jack output.</p>
                <p><strong>⚙️ Speed & Settings:</strong> Uses your configured WPM speed and radio mode settings.</p>
                <p><strong>🔊 No Sidetone:</strong> Your radio provides the audio sidetone, not the Summit device.</p>
            </div>
        </div>

        <!-- CW Memory Presets Card (Collapsible) -->
        <div class="card">
            <h2 onclick="toggleSection('memoryPresetsCard')" style="cursor: pointer; user-select: none;">
                <span id="memoryPresetsToggle">▶</span> CW Memory Presets
            </h2>
            <div id="memoryPresetsCard" style="display: none; margin-top: 15px;">
                <div class="form-group">
                    <label for="memorySelect">Select Memory</label>
                    <select id="memorySelect">
                        <option value="">Loading...</option>
                    </select>
                </div>

                <button class="btn btn-primary" onclick="sendMemory()">
                    Send Memory
                </button>

                <div class="info-box" style="margin-top: 15px;">
                    <p><strong>📝 Quick Send:</strong> Select a preset from the dropdown and click "Send Memory" to transmit it via radio output.</p>
                </div>
            </div>
        </div>

        <!-- Manage CW Memories Card (Collapsible) -->
        <div class="card">
            <h2 onclick="toggleSection('manageMemoriesCard')" style="cursor: pointer; user-select: none;">
                <span id="manageMemoriesToggle">▶</span> Manage CW Memories
            </h2>
            <div id="manageMemoriesCard" style="display: none; margin-top: 15px;">
                <button class="btn btn-primary" onclick="showCreateModal()" style="margin-bottom: 15px;">
                    + Create New Preset
                </button>

                <div style="overflow-x: auto;">
                    <table id="memoriesTable" style="width: 100%; border-collapse: collapse;">
                        <thead>
                            <tr style="background: rgba(255,255,255,0.1);">
                                <th style="padding: 10px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.2);">Slot</th>
                                <th style="padding: 10px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.2);">Label</th>
                                <th style="padding: 10px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.2);">Message</th>
                                <th style="padding: 10px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.2);">Actions</th>
                            </tr>
                        </thead>
                        <tbody id="memoriesTableBody">
                            <tr><td colspan="4" style="padding: 20px; text-align: center;">Loading...</td></tr>
                        </tbody>
                    </table>
                </div>
            </div>
        </div>
    </div>

    <!-- Create/Edit Modal -->
    <div id="memoryModal" style="display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.7); z-index: 1000; padding: 20px;">
        <div style="max-width: 500px; margin: 50px auto; background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%); border-radius: 15px; padding: 25px; border: 1px solid rgba(255,255,255,0.2);">
            <h2 id="modalTitle" style="margin-bottom: 20px;">Create New Preset</h2>

            <div class="form-group">
                <label for="modalSlot">Slot</label>
                <select id="modalSlot" style="width: 100%; padding: 12px; border-radius: 8px; border: 1px solid rgba(255,255,255,0.3); background: rgba(255,255,255,0.15); color: #fff; font-size: 1rem;">
                    <option value="1">1</option>
                    <option value="2">2</option>
                    <option value="3">3</option>
                    <option value="4">4</option>
                    <option value="5">5</option>
                    <option value="6">6</option>
                    <option value="7">7</option>
                    <option value="8">8</option>
                    <option value="9">9</option>
                    <option value="10">10</option>
                </select>
            </div>

            <div class="form-group">
                <label for="modalLabel">Label (max 15 chars)</label>
                <input type="text" id="modalLabel" maxlength="15" style="width: 100%; padding: 12px; border-radius: 8px; border: 1px solid rgba(255,255,255,0.3); background: rgba(255,255,255,0.1); color: #fff; font-size: 1rem;">
                <div class="char-count"><span id="labelCount">0</span> / 15</div>
            </div>

            <div class="form-group">
                <label for="modalMessage">Message (max 100 chars)</label>
                <textarea id="modalMessage" maxlength="100" style="width: 100%; padding: 12px; border-radius: 8px; border: 1px solid rgba(255,255,255,0.3); background: rgba(255,255,255,0.1); color: #fff; font-size: 1rem; min-height: 80px;"></textarea>
                <div class="char-count"><span id="messageCount">0</span> / 100</div>
            </div>

            <div id="modalError" class="message error" style="margin-bottom: 15px;"></div>

            <button class="btn btn-primary" onclick="saveMemory()">
                Save Preset
            </button>
            <button class="btn" onclick="closeModal()">
                Cancel
            </button>
        </div>
    </div>

    <script>
        // Update character count
        document.getElementById('messageInput').addEventListener('input', function(e) {
            document.getElementById('charCount').textContent = e.target.value.length;
        });

        // WPM slider handling
        document.getElementById('wpmSlider').addEventListener('input', function(e) {
            const wpm = e.target.value;
            document.getElementById('wpmDisplay').textContent = wpm + ' WPM';
        });

        document.getElementById('wpmSlider').addEventListener('change', function(e) {
            const wpm = parseInt(e.target.value);
            setWPM(wpm);
        });

        // Load current WPM speed
        async function loadWPM() {
            try {
                const response = await fetch('/api/radio/wpm');
                const data = await response.json();

                if (data.wpm) {
                    document.getElementById('wpmSlider').value = data.wpm;
                    document.getElementById('wpmDisplay').textContent = data.wpm + ' WPM';
                }
            } catch (error) {
                console.error('Failed to load WPM:', error);
            }
        }

        // Set WPM speed
        async function setWPM(wpm) {
            try {
                const response = await fetch('/api/radio/wpm', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ wpm: wpm })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('Speed updated to ' + wpm + ' WPM', 'success');
                } else {
                    showMessage('Failed to update speed: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to set WPM:', error);
                showMessage('Failed to update speed', 'error');
            }
        }

        // Check radio mode status
        async function checkRadioStatus() {
            try {
                const response = await fetch('/api/radio/status');
                const data = await response.json();

                const statusBadge = document.getElementById('radioStatus');
                const enterBtn = document.getElementById('enterRadioBtn');

                if (data.active) {
                    statusBadge.textContent = '✓ Radio Mode Active';
                    statusBadge.className = 'status-badge status-active';
                    enterBtn.textContent = 'Radio Mode Active';
                    enterBtn.disabled = true;
                    enterBtn.style.opacity = '0.6';
                    enterBtn.style.cursor = 'not-allowed';
                } else {
                    statusBadge.textContent = '○ Radio Mode Inactive';
                    statusBadge.className = 'status-badge status-inactive';
                    enterBtn.textContent = 'Enter Radio Mode';
                    enterBtn.disabled = false;
                    enterBtn.style.opacity = '1';
                    enterBtn.style.cursor = 'pointer';
                }
            } catch (error) {
                console.error('Failed to check radio status:', error);
                showMessage('Failed to check radio status', 'error');
            }
        }

        // Enter radio mode
        async function enterRadioMode() {
            try {
                const response = await fetch('/api/radio/enter', { method: 'POST' });
                const data = await response.json();

                if (data.success) {
                    showMessage('Device switched to Radio Mode', 'success');
                    checkRadioStatus();
                } else {
                    showMessage('Failed to enter radio mode: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to enter radio mode:', error);
                showMessage('Failed to enter radio mode', 'error');
            }
        }

        // Send message
        async function sendMessage() {
            const message = document.getElementById('messageInput').value.trim();

            if (message.length === 0) {
                showMessage('Please enter a message to send', 'error');
                return;
            }

            try {
                const response = await fetch('/api/radio/send', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ message: message })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('Message queued for transmission (' + message.length + ' characters)', 'success');
                    document.getElementById('messageInput').value = '';
                    document.getElementById('charCount').textContent = '0';
                } else {
                    showMessage('Failed to send message: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to send message:', error);
                showMessage('Failed to send message', 'error');
            }
        }

        // Show message
        function showMessage(text, type) {
            const messageBox = document.getElementById('messageBox');
            messageBox.textContent = text;
            messageBox.className = 'message ' + type;
            messageBox.style.display = 'block';

            setTimeout(() => {
                messageBox.style.display = 'none';
            }, 5000);
        }

        // Load initial state on page load
        loadWPM();
        checkRadioStatus();
        loadMemories();

        // Refresh status every 5 seconds
        setInterval(checkRadioStatus, 5000);

        // ============================================
        // CW Memories Functions
        // ============================================

        let memoriesData = [];
        let editingSlot = null;

        // Toggle collapsible sections
        function toggleSection(sectionId) {
            const section = document.getElementById(sectionId);
            const toggle = document.getElementById(sectionId.replace('Card', 'Toggle'));

            if (section.style.display === 'none') {
                section.style.display = 'block';
                toggle.textContent = '▼';
            } else {
                section.style.display = 'none';
                toggle.textContent = '▶';
            }
        }

        // Load all memories from API
        async function loadMemories() {
            try {
                const response = await fetch('/api/memories/list');
                const data = await response.json();
                memoriesData = data.presets || [];

                updateMemoriesDropdown();
                updateMemoriesTable();
            } catch (error) {
                console.error('Failed to load memories:', error);
                showMessage('Failed to load memories', 'error');
            }
        }

        // Update dropdown for quick send
        function updateMemoriesDropdown() {
            const select = document.getElementById('memorySelect');
            select.innerHTML = '<option value="">-- Select a memory --</option>';

            memoriesData.forEach(preset => {
                if (!preset.isEmpty && preset.label) {
                    const option = document.createElement('option');
                    option.value = preset.slot;
                    option.textContent = preset.slot + ': ' + preset.label;
                    select.appendChild(option);
                }
            });
        }

        // Update memories table
        function updateMemoriesTable() {
            const tbody = document.getElementById('memoriesTableBody');
            tbody.innerHTML = '';

            memoriesData.forEach(preset => {
                const row = document.createElement('tr');
                row.style.borderBottom = '1px solid rgba(255,255,255,0.1)';

                if (preset.isEmpty) {
                    row.innerHTML = `
                        <td style="padding: 10px;">${preset.slot}</td>
                        <td style="padding: 10px; color: rgba(255,255,255,0.5); font-style: italic;" colspan="2">(empty)</td>
                        <td style="padding: 10px;">
                            <button class="btn" style="padding: 6px 12px; font-size: 0.9rem; margin: 0;" onclick="showCreateModal(${preset.slot})">Create</button>
                        </td>
                    `;
                } else {
                    const truncatedMessage = preset.message.length > 40 ? preset.message.substring(0, 40) + '...' : preset.message;
                    row.innerHTML = `
                        <td style="padding: 10px;">${preset.slot}</td>
                        <td style="padding: 10px;">${preset.label}</td>
                        <td style="padding: 10px; font-family: monospace; font-size: 0.9rem;">${truncatedMessage}</td>
                        <td style="padding: 10px;">
                            <button class="btn" style="padding: 6px 12px; font-size: 0.9rem; margin-right: 5px; background: #4A90E2;" onclick="previewMemory(${preset.slot})">Preview</button>
                            <button class="btn" style="padding: 6px 12px; font-size: 0.9rem; margin-right: 5px;" onclick="showEditModal(${preset.slot})">Edit</button>
                            <button class="btn btn-danger" style="padding: 6px 12px; font-size: 0.9rem;" onclick="deleteMemory(${preset.slot})">Delete</button>
                        </td>
                    `;
                }

                tbody.appendChild(row);
            });
        }

        // Send memory via radio
        async function sendMemory() {
            const select = document.getElementById('memorySelect');
            const slot = parseInt(select.value);

            if (!slot) {
                showMessage('Please select a memory to send', 'error');
                return;
            }

            try {
                const response = await fetch('/api/memories/send', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ slot: slot })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('Memory queued for transmission', 'success');
                } else {
                    showMessage('Failed to send memory: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to send memory:', error);
                showMessage('Failed to send memory', 'error');
            }
        }

        // Preview memory on device speaker
        async function previewMemory(slot) {
            try {
                const response = await fetch('/api/memories/preview', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ slot: slot })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('Playing preview on device speaker', 'success');
                } else {
                    showMessage('Failed to preview memory: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to preview memory:', error);
                showMessage('Failed to preview memory', 'error');
            }
        }

        // Show create modal
        function showCreateModal(slot = null) {
            editingSlot = null;
            document.getElementById('modalTitle').textContent = 'Create New Preset';
            document.getElementById('modalSlot').value = slot || '1';
            document.getElementById('modalSlot').disabled = false;
            document.getElementById('modalLabel').value = '';
            document.getElementById('modalMessage').value = '';
            document.getElementById('labelCount').textContent = '0';
            document.getElementById('messageCount').textContent = '0';
            document.getElementById('modalError').style.display = 'none';
            document.getElementById('memoryModal').style.display = 'block';
        }

        // Show edit modal
        function showEditModal(slot) {
            const preset = memoriesData.find(p => p.slot === slot);
            if (!preset) return;

            editingSlot = slot;
            document.getElementById('modalTitle').textContent = 'Edit Preset';
            document.getElementById('modalSlot').value = slot;
            document.getElementById('modalSlot').disabled = true;
            document.getElementById('modalLabel').value = preset.label;
            document.getElementById('modalMessage').value = preset.message;
            document.getElementById('labelCount').textContent = preset.label.length;
            document.getElementById('messageCount').textContent = preset.message.length;
            document.getElementById('modalError').style.display = 'none';
            document.getElementById('memoryModal').style.display = 'block';
        }

        // Close modal
        function closeModal() {
            document.getElementById('memoryModal').style.display = 'none';
            editingSlot = null;
        }

        // Save memory (create or update)
        async function saveMemory() {
            const slot = parseInt(document.getElementById('modalSlot').value);
            const label = document.getElementById('modalLabel').value.trim().toUpperCase();
            const message = document.getElementById('modalMessage').value.trim().toUpperCase();
            const errorBox = document.getElementById('modalError');

            // Validation
            if (!label) {
                errorBox.textContent = 'Label cannot be empty';
                errorBox.style.display = 'block';
                return;
            }
            if (!message) {
                errorBox.textContent = 'Message cannot be empty';
                errorBox.style.display = 'block';
                return;
            }

            const endpoint = editingSlot ? '/api/memories/update' : '/api/memories/create';

            try {
                const response = await fetch(endpoint, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ slot, label, message })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage(editingSlot ? 'Memory updated successfully' : 'Memory created successfully', 'success');
                    closeModal();
                    loadMemories();
                } else {
                    errorBox.textContent = data.error || 'Failed to save memory';
                    errorBox.style.display = 'block';
                }
            } catch (error) {
                console.error('Failed to save memory:', error);
                errorBox.textContent = 'Failed to save memory';
                errorBox.style.display = 'block';
            }
        }

        // Delete memory
        async function deleteMemory(slot) {
            const preset = memoriesData.find(p => p.slot === slot);
            if (!preset) return;

            if (!confirm(`Delete preset "${preset.label}"?`)) {
                return;
            }

            try {
                const response = await fetch('/api/memories/delete', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ slot: slot })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('Memory deleted successfully', 'success');
                    loadMemories();
                } else {
                    showMessage('Failed to delete memory: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to delete memory:', error);
                showMessage('Failed to delete memory', 'error');
            }
        }

        // Character counters for modal
        document.getElementById('modalLabel').addEventListener('input', function(e) {
            document.getElementById('labelCount').textContent = e.target.value.length;
        });

        document.getElementById('modalMessage').addEventListener('input', function(e) {
            document.getElementById('messageCount').textContent = e.target.value.length;
        });

        // Refresh memories every 15 seconds
        setInterval(loadMemories, 15000);
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGES_RADIO_H
