/*
 * Web Server Module (Modular Version)
 * Provides a comprehensive web interface for device management
 * Features: QSO logging, settings management, device status
 *
 * Access via: http://vail-summit.local or device IP address
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

// Include modular web components
#include "web_pages_dashboard.h"
#include "web_pages_wifi.h"
#include "web_api_wifi.h"
#include "web_api_qso.h"
#include "web_api_settings.h"
#include "web_api_memories.h"
#include "web_logger_enhanced.h"

// ============================================
// PROGMEM HTML Page Constants
// ============================================

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

const char SETTINGS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Device Settings - VAIL SUMMIT</title>
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
        .card h2 { font-size: 1.3rem; margin-bottom: 20px; }
        .form-group { margin-bottom: 25px; }
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            font-size: 0.9rem;
            text-transform: uppercase;
            letter-spacing: 1px;
            opacity: 0.9;
        }
        input[type="range"] {
            width: 100%;
            height: 8px;
            border-radius: 4px;
            background: rgba(255,255,255,0.2);
            outline: none;
            cursor: pointer;
        }
        input[type="text"] {
            width: 100%;
            padding: 12px;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            background: rgba(255,255,255,0.1);
            color: #fff;
            font-size: 1rem;
        }
        input[type="text"]::placeholder { color: rgba(255,255,255,0.5); }
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
        select option {
            background: #1e3c72;
            color: #fff;
        }
        .slider-display {
            display: flex;
            align-items: center;
            gap: 15px;
            margin-top: 10px;
        }
        .slider-display span {
            font-size: 1.5rem;
            font-weight: 600;
            min-width: 80px;
            text-align: right;
        }
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
        .message {
            padding: 12px;
            border-radius: 8px;
            margin-bottom: 15px;
            display: none;
        }
        .message.success { background: rgba(0,255,0,0.2); border: 1px solid rgba(0,255,0,0.5); }
        .message.error { background: rgba(255,0,0,0.2); border: 1px solid rgba(255,0,0,0.5); }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>⚙️ Device Settings</h1>
            <p>Configure your VAIL SUMMIT device</p>
        </header>

        <div id="messageBox" class="message"></div>

        <div class="card">
            <h2>CW Settings</h2>

            <div class="form-group">
                <label for="wpmSlider">Speed (WPM)</label>
                <div class="slider-display">
                    <input type="range" id="wpmSlider" min="5" max="40" value="20">
                    <span id="wpmDisplay">20 WPM</span>
                </div>
            </div>

            <div class="form-group">
                <label for="toneSlider">Tone Frequency (Hz)</label>
                <div class="slider-display">
                    <input type="range" id="toneSlider" min="400" max="1200" step="50" value="700">
                    <span id="toneDisplay">700 Hz</span>
                </div>
            </div>

            <div class="form-group">
                <label for="keyTypeSelect">Key Type</label>
                <select id="keyTypeSelect">
                    <option value="0">Straight Key</option>
                    <option value="1">Iambic A</option>
                    <option value="2">Iambic B</option>
                </select>
            </div>

            <button class="btn btn-primary" onclick="saveCWSettings()">
                Save CW Settings
            </button>
        </div>

        <div class="card">
            <h2>Audio Settings</h2>

            <div class="form-group">
                <label for="volumeSlider">Volume (%)</label>
                <div class="slider-display">
                    <input type="range" id="volumeSlider" min="0" max="100" value="50">
                    <span id="volumeDisplay">50%</span>
                </div>
            </div>

            <button class="btn btn-primary" onclick="saveVolume()">
                Save Volume
            </button>
        </div>

        <div class="card">
            <h2>Station Settings</h2>

            <div class="form-group">
                <label for="callsignInput">Callsign</label>
                <input type="text" id="callsignInput" placeholder="W1ABC" maxlength="10">
            </div>

            <button class="btn btn-primary" onclick="saveCallsign()">
                Save Callsign
            </button>
        </div>

        <div style="text-align: center; margin-top: 30px;">
            <button class="btn" onclick="window.location.href='/'">
                Back to Dashboard
            </button>
        </div>
    </div>

    <script>
        // Update slider displays
        document.getElementById('wpmSlider').addEventListener('input', function(e) {
            document.getElementById('wpmDisplay').textContent = e.target.value + ' WPM';
        });

        document.getElementById('toneSlider').addEventListener('input', function(e) {
            document.getElementById('toneDisplay').textContent = e.target.value + ' Hz';
        });

        document.getElementById('volumeSlider').addEventListener('input', function(e) {
            document.getElementById('volumeDisplay').textContent = e.target.value + '%';
        });

        // Load all settings on page load
        async function loadSettings() {
            try {
                // Load CW settings
                const cwResponse = await fetch('/api/settings/cw');
                const cwData = await cwResponse.json();

                if (cwData.wpm !== undefined) {
                    document.getElementById('wpmSlider').value = cwData.wpm;
                    document.getElementById('wpmDisplay').textContent = cwData.wpm + ' WPM';
                }
                if (cwData.tone !== undefined) {
                    document.getElementById('toneSlider').value = cwData.tone;
                    document.getElementById('toneDisplay').textContent = cwData.tone + ' Hz';
                }
                if (cwData.keyType !== undefined) {
                    document.getElementById('keyTypeSelect').value = cwData.keyType;
                }

                // Load volume
                const volResponse = await fetch('/api/settings/volume');
                const volData = await volResponse.json();

                if (volData.volume !== undefined) {
                    document.getElementById('volumeSlider').value = volData.volume;
                    document.getElementById('volumeDisplay').textContent = volData.volume + '%';
                }

                // Load callsign
                const callResponse = await fetch('/api/settings/callsign');
                const callData = await callResponse.json();

                if (callData.callsign !== undefined) {
                    document.getElementById('callsignInput').value = callData.callsign;
                }
            } catch (error) {
                console.error('Failed to load settings:', error);
                showMessage('Failed to load settings', 'error');
            }
        }

        // Save CW settings
        async function saveCWSettings() {
            const wpm = parseInt(document.getElementById('wpmSlider').value);
            const tone = parseInt(document.getElementById('toneSlider').value);
            const keyType = parseInt(document.getElementById('keyTypeSelect').value);

            try {
                const response = await fetch('/api/settings/cw', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ wpm: wpm, tone: tone, keyType: keyType })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('CW settings saved successfully', 'success');
                } else {
                    showMessage('Failed to save CW settings: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to save CW settings:', error);
                showMessage('Failed to save CW settings', 'error');
            }
        }

        // Save volume
        async function saveVolume() {
            const volume = parseInt(document.getElementById('volumeSlider').value);

            try {
                const response = await fetch('/api/settings/volume', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ volume: volume })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('Volume saved successfully', 'success');
                } else {
                    showMessage('Failed to save volume: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to save volume:', error);
                showMessage('Failed to save volume', 'error');
            }
        }

        // Save callsign
        async function saveCallsign() {
            const callsign = document.getElementById('callsignInput').value.trim().toUpperCase();

            if (callsign.length === 0) {
                showMessage('Please enter a callsign', 'error');
                return;
            }

            try {
                const response = await fetch('/api/settings/callsign', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ callsign: callsign })
                });

                const data = await response.json();

                if (data.success) {
                    showMessage('Callsign saved successfully', 'success');
                    document.getElementById('callsignInput').value = callsign;
                } else {
                    showMessage('Failed to save callsign: ' + (data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                console.error('Failed to save callsign:', error);
                showMessage('Failed to save callsign', 'error');
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

        // Load settings on page load
        loadSettings();
    </script>
</body>
</html>
)rawliteral";

const char SYSTEM_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>System Info - VAIL SUMMIT</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #fff;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 1000px; margin: 0 auto; }
        header {
            text-align: center;
            margin-bottom: 30px;
            padding: 20px;
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            backdrop-filter: blur(10px);
        }
        h1 { font-size: 2rem; margin-bottom: 10px; }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 20px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
        }
        .card h2 { font-size: 1.2rem; margin-bottom: 15px; opacity: 0.9; }
        .info-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 10px 0;
            border-bottom: 1px solid rgba(255,255,255,0.1);
        }
        .info-row:last-child { border-bottom: none; }
        .info-label {
            font-size: 0.9rem;
            opacity: 0.7;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        .info-value {
            font-size: 1.1rem;
            font-weight: 600;
            text-align: right;
        }
        .signal-good { color: #00ff00; }
        .signal-ok { color: #ffff00; }
        .signal-poor { color: #ff4444; }
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
        }
        .btn:hover {
            background: rgba(255,255,255,0.3);
            border-color: rgba(255,255,255,0.5);
        }
        .last-update {
            text-align: center;
            opacity: 0.6;
            font-size: 0.85rem;
            margin-top: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>ℹ️ System Info</h1>
            <p>VAIL SUMMIT Diagnostics & Status</p>
        </header>

        <div class="grid">
            <div class="card">
                <h2>📱 Firmware</h2>
                <div class="info-row">
                    <span class="info-label">Version</span>
                    <span class="info-value" id="firmware">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Build Date</span>
                    <span class="info-value" id="firmwareDate">Loading...</span>
                </div>
            </div>

            <div class="card">
                <h2>⏱️ System</h2>
                <div class="info-row">
                    <span class="info-label">Uptime</span>
                    <span class="info-value" id="uptime">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">CPU Speed</span>
                    <span class="info-value" id="cpuSpeed">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Flash Size</span>
                    <span class="info-value" id="flashSize">Loading...</span>
                </div>
            </div>

            <div class="card">
                <h2>💾 Memory</h2>
                <div class="info-row">
                    <span class="info-label">Free RAM</span>
                    <span class="info-value" id="freeHeap">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Min Free RAM</span>
                    <span class="info-value" id="minFreeHeap">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Free PSRAM</span>
                    <span class="info-value" id="freePsram">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Min Free PSRAM</span>
                    <span class="info-value" id="minFreePsram">Loading...</span>
                </div>
            </div>

            <div class="card">
                <h2>💿 Storage</h2>
                <div class="info-row">
                    <span class="info-label">SPIFFS Used</span>
                    <span class="info-value" id="spiffsUsed">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">SPIFFS Total</span>
                    <span class="info-value" id="spiffsTotal">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">QSO Logs</span>
                    <span class="info-value" id="qsoCount">Loading...</span>
                </div>
            </div>

            <div class="card">
                <h2>📶 WiFi</h2>
                <div class="info-row">
                    <span class="info-label">Status</span>
                    <span class="info-value" id="wifiStatus">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">SSID</span>
                    <span class="info-value" id="wifiSSID">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">IP Address</span>
                    <span class="info-value" id="wifiIP">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Signal</span>
                    <span class="info-value" id="wifiSignal">Loading...</span>
                </div>
            </div>

            <div class="card">
                <h2>🔋 Battery</h2>
                <div class="info-row">
                    <span class="info-label">Voltage</span>
                    <span class="info-value" id="battVoltage">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Charge</span>
                    <span class="info-value" id="battPercent">Loading...</span>
                </div>
                <div class="info-row">
                    <span class="info-label">Monitor</span>
                    <span class="info-value" id="battMonitor">Loading...</span>
                </div>
            </div>
        </div>

        <div class="last-update" id="lastUpdate">Last updated: Never</div>

        <div style="text-align: center; margin-top: 20px;">
            <button class="btn" onclick="window.location.href='/'">
                Back to Dashboard
            </button>
        </div>
    </div>

    <script>
        // Format bytes to human readable
        function formatBytes(bytes) {
            if (bytes < 1024) return bytes + ' B';
            if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
            return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
        }

        // Format uptime
        function formatUptime(ms) {
            const seconds = Math.floor(ms / 1000);
            const minutes = Math.floor(seconds / 60);
            const hours = Math.floor(minutes / 60);
            const days = Math.floor(hours / 24);

            if (days > 0) return days + 'd ' + (hours % 24) + 'h';
            if (hours > 0) return hours + 'h ' + (minutes % 60) + 'm';
            if (minutes > 0) return minutes + 'm ' + (seconds % 60) + 's';
            return seconds + 's';
        }

        // Load system info
        async function loadSystemInfo() {
            try {
                const response = await fetch('/api/system/info');
                const data = await response.json();

                // Firmware
                document.getElementById('firmware').textContent = data.firmware || 'Unknown';
                document.getElementById('firmwareDate').textContent = data.firmwareDate || 'Unknown';

                // System
                document.getElementById('uptime').textContent = formatUptime(data.uptime || 0);
                document.getElementById('cpuSpeed').textContent = (data.cpuFreq || '?') + ' MHz';
                document.getElementById('flashSize').textContent = formatBytes(data.flashSize || 0);

                // Memory
                document.getElementById('freeHeap').textContent = formatBytes(data.freeHeap || 0);
                document.getElementById('minFreeHeap').textContent = formatBytes(data.minFreeHeap || 0);
                document.getElementById('freePsram').textContent = data.psramFound ? formatBytes(data.freePsram || 0) : 'N/A';
                document.getElementById('minFreePsram').textContent = data.psramFound ? formatBytes(data.minFreePsram || 0) : 'N/A';

                // Storage
                document.getElementById('spiffsUsed').textContent = formatBytes(data.spiffsUsed || 0);
                document.getElementById('spiffsTotal').textContent = formatBytes(data.spiffsTotal || 0);
                document.getElementById('qsoCount').textContent = (data.qsoCount || 0) + ' logs';

                // WiFi
                document.getElementById('wifiStatus').textContent = data.wifiConnected ? 'Connected' : 'Disconnected';
                document.getElementById('wifiSSID').textContent = data.wifiSSID || 'N/A';
                document.getElementById('wifiIP').textContent = data.wifiIP || 'N/A';

                const rssi = data.wifiRSSI || -100;
                let signalClass = 'signal-poor';
                if (rssi > -60) signalClass = 'signal-good';
                else if (rssi > -70) signalClass = 'signal-ok';
                document.getElementById('wifiSignal').innerHTML = '<span class="' + signalClass + '">' + rssi + ' dBm</span>';

                // Battery
                document.getElementById('battVoltage').textContent = (data.batteryVoltage || 0).toFixed(2) + ' V';
                document.getElementById('battPercent').textContent = (data.batteryPercent || 0).toFixed(0) + '%';
                document.getElementById('battMonitor').textContent = data.batteryMonitor || 'None';

                // Update timestamp
                const now = new Date();
                document.getElementById('lastUpdate').textContent = 'Last updated: ' + now.toLocaleTimeString();

            } catch (error) {
                console.error('Failed to load system info:', error);
            }
        }

        // Load on page load and refresh every 10 seconds
        loadSystemInfo();
        setInterval(loadSystemInfo, 10000);
    </script>
</body>
</html>
)rawliteral";

// Global web server instance
AsyncWebServer webServer(80);

// mDNS hostname
String mdnsHostname = "vail-summit";

// Server state
bool webServerRunning = false;

// Forward declarations
void setupWebServer();
void stopWebServer();
String getDeviceStatusJSON();
String getQSOLogsJSON();
String generateADIF();
String generateCSV();

/*
 * Initialize and start the web server
 * Called automatically when WiFi connects
 */
void setupWebServer() {
  if (webServerRunning) {
    Serial.println("Web server already running");
    return;
  }

  Serial.println("Starting web server...");

  // Check if we're in AP mode (from settings_wifi.h)
  extern bool isAPMode;

  // Set up mDNS responder (only works in Station mode, not AP mode)
  if (!isAPMode) {
    if (MDNS.begin(mdnsHostname.c_str())) {
      Serial.print("mDNS responder started: http://");
      Serial.print(mdnsHostname);
      Serial.println(".local");
      MDNS.addService("http", "tcp", 80);
    } else {
      Serial.println("Error setting up mDNS responder!");
    }
  } else {
    Serial.println("Skipping mDNS setup (not supported in AP mode)");
  }

  // ============================================
  // Main Dashboard Page
  // ============================================
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveDashboard(request, FIRMWARE_VERSION, FIRMWARE_DATE, mdnsHostname);
  });

  // ============================================
  // QSO Logger Page (Enhanced)
  // ============================================
  webServer.on("/logger", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", LOGGER_HTML);
  });

  // ============================================
  // WiFi Setup Page
  // ============================================
  webServer.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveWiFiSetup(request);
  });

  // ============================================
  // Radio Control Page
  // ============================================
  webServer.on("/radio", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", RADIO_HTML);
  });

  // ============================================
  // Device Settings Page
  // ============================================
  webServer.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", SETTINGS_HTML);
  });

  // ============================================
  // System Info Page
  // ============================================
  webServer.on("/system", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", SYSTEM_HTML);
  });

  // ============================================
  // API Endpoints
  // ============================================

  // Device status endpoint
  webServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getDeviceStatusJSON());
  });

  // QSO logs list endpoint
  webServer.on("/api/qsos", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getQSOLogsJSON());
  });

  // ADIF export endpoint
  webServer.on("/api/export/adif", HTTP_GET, [](AsyncWebServerRequest *request) {
    String adif = generateADIF();
    AsyncWebServerResponse *response = request->beginResponse(200, "application/x-adif", adif);
    response->addHeader("Content-Disposition", "attachment; filename=vail-summit-logs.adi");
    request->send(response);
  });

  // CSV export endpoint
  webServer.on("/api/export/csv", HTTP_GET, [](AsyncWebServerRequest *request) {
    String csv = generateCSV();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csv);
    response->addHeader("Content-Disposition", "attachment; filename=vail-summit-logs.csv");
    request->send(response);
  });

  // Setup modular API endpoints
  setupQSOAPI(webServer);
  setupWiFiAPI(webServer);
  setupSettingsAPI(webServer);
  setupMemoriesAPI(webServer);

  // Start server
  webServer.begin();
  webServerRunning = true;

  Serial.println("Web server started successfully");

  // Show appropriate access method based on mode
  if (isAPMode) {
    Serial.print("Access at: http://");
    Serial.print(WiFi.softAPIP());
    Serial.println("/");
    Serial.println("(mDNS not available in AP mode - use IP address only)");
  } else {
    Serial.print("Access at: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
    Serial.print("Or via mDNS: http://");
    Serial.print(mdnsHostname);
    Serial.println(".local/");
  }
}

/*
 * Stop the web server
 */
void stopWebServer() {
  if (!webServerRunning) {
    return;
  }

  webServer.end();
  MDNS.end();
  webServerRunning = false;
  Serial.println("Web server stopped");
}

/*
 * Get device status as JSON
 */
String getDeviceStatusJSON() {
  extern bool isAPMode;  // From settings_wifi.h

  JsonDocument doc;

  // Battery status (using external variables)
  extern bool hasLC709203;
  extern bool hasMAX17048;
  extern Adafruit_LC709203F lc;
  extern Adafruit_MAX17048 maxlipo;

  float batteryVoltage = 0;
  float batteryPercent = 0;

  if (hasMAX17048) {
    batteryVoltage = maxlipo.cellVoltage();
    batteryPercent = maxlipo.cellPercent();
  } else if (hasLC709203) {
    batteryVoltage = lc.cellVoltage();
    batteryPercent = lc.cellPercent();
  }

  char batteryStr[32];
  snprintf(batteryStr, sizeof(batteryStr), "%.2fV (%.0f%%)", batteryVoltage, batteryPercent);
  doc["battery"] = batteryStr;

  // WiFi status with mode information
  if (isAPMode) {
    doc["wifi"] = "AP Mode";
    doc["ip"] = WiFi.softAPIP().toString();
    doc["rssi"] = 0;  // Not applicable in AP mode
    doc["wifiMode"] = "AP";
  } else {
    doc["wifi"] = WiFi.isConnected() ? "Connected" : "Disconnected";
    doc["ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : "N/A";
    doc["rssi"] = WiFi.isConnected() ? WiFi.RSSI() : 0;
    doc["wifiMode"] = "STA";
  }

  // QSO count
  extern StorageStats storageStats;
  doc["qsoCount"] = storageStats.totalLogs;

  // Firmware info
  doc["firmware"] = FIRMWARE_VERSION;

  String output;
  serializeJson(doc, output);
  return output;
}

/*
 * Get all QSO logs as JSON
 */
String getQSOLogsJSON() {
  extern fs::SPIFFSFS FileSystem;

  JsonDocument doc;
  JsonArray logsArray = doc["logs"].to<JsonArray>();

  int totalCount = 0;

  // Iterate through all log files
  File root = FileSystem.open("/logs");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String filename = String(file.name());

        // Extract basename
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) {
          filename = filename.substring(lastSlash + 1);
        }

        // Process QSO log files
        if (filename.startsWith("qso_") && filename.endsWith(".json")) {
          File logFile = FileSystem.open(file.path(), "r");
          if (logFile) {
            String content = logFile.readString();
            logFile.close();

            JsonDocument logDoc;
            DeserializationError error = deserializeJson(logDoc, content);

            if (!error && logDoc.containsKey("logs")) {
              JsonArray logs = logDoc["logs"];
              for (JsonVariant v : logs) {
                logsArray.add(v);
                totalCount++;
              }
            }
          }
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }

  doc["count"] = totalCount;

  String output;
  serializeJson(doc, output);
  return output;
}

/*
 * Generate ADIF export
 */
String generateADIF() {
  extern fs::SPIFFSFS FileSystem;

  String adif = "ADIF Export from VAIL SUMMIT\n";
  adif += "<PROGRAMID:11>VAIL SUMMIT\n";
  adif += "<PROGRAMVERSION:" + String(strlen(FIRMWARE_VERSION)) + ">" + String(FIRMWARE_VERSION) + "\n";
  adif += "<ADIF_VER:5>3.1.4\n";
  adif += "<EOH>\n\n";

  // Iterate through all log files
  File root = FileSystem.open("/logs");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String filename = String(file.name());
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) {
          filename = filename.substring(lastSlash + 1);
        }

        if (filename.startsWith("qso_") && filename.endsWith(".json")) {
          File logFile = FileSystem.open(file.path(), "r");
          if (logFile) {
            String content = logFile.readString();
            logFile.close();

            JsonDocument logDoc;
            DeserializationError error = deserializeJson(logDoc, content);

            if (!error && logDoc.containsKey("logs")) {
              JsonArray logs = logDoc["logs"];
              for (JsonVariant v : logs) {
                JsonObject qso = v.as<JsonObject>();

                // Required fields
                if (qso.containsKey("callsign") && qso["callsign"].as<String>().length() > 0) {
                  String call = qso["callsign"].as<String>();
                  adif += "<CALL:" + String(call.length()) + ">" + call + " ";
                }

                if (qso.containsKey("frequency")) {
                  String freq = String(qso["frequency"].as<float>(), 3);
                  adif += "<FREQ:" + String(freq.length()) + ">" + freq + " ";
                }

                if (qso.containsKey("mode") && qso["mode"].as<String>().length() > 0) {
                  String mode = qso["mode"].as<String>();
                  adif += "<MODE:" + String(mode.length()) + ">" + mode + " ";
                }

                if (qso.containsKey("date") && qso["date"].as<String>().length() > 0) {
                  String date = qso["date"].as<String>();
                  adif += "<QSO_DATE:" + String(date.length()) + ">" + date + " ";
                }

                if (qso.containsKey("time_on") && qso["time_on"].as<String>().length() > 0) {
                  String time = qso["time_on"].as<String>();
                  // Convert HHMM to HHMMSS
                  if (time.length() == 4) {
                    time += "00";
                  }
                  adif += "<TIME_ON:" + String(time.length()) + ">" + time + " ";
                }

                // Optional fields
                if (qso.containsKey("rst_sent") && qso["rst_sent"].as<String>().length() > 0) {
                  String rst = qso["rst_sent"].as<String>();
                  adif += "<RST_SENT:" + String(rst.length()) + ">" + rst + " ";
                }

                if (qso.containsKey("rst_rcvd") && qso["rst_rcvd"].as<String>().length() > 0) {
                  String rst = qso["rst_rcvd"].as<String>();
                  adif += "<RST_RCVD:" + String(rst.length()) + ">" + rst + " ";
                }

                if (qso.containsKey("gridsquare") && qso["gridsquare"].as<String>().length() > 0) {
                  String grid = qso["gridsquare"].as<String>();
                  adif += "<GRIDSQUARE:" + String(grid.length()) + ">" + grid + " ";
                }

                if (qso.containsKey("my_gridsquare") && qso["my_gridsquare"].as<String>().length() > 0) {
                  String grid = qso["my_gridsquare"].as<String>();
                  adif += "<MY_GRIDSQUARE:" + String(grid.length()) + ">" + grid + " ";
                }

                // POTA support
                if (qso.containsKey("my_pota_ref") && qso["my_pota_ref"].as<String>().length() > 0) {
                  String pota = qso["my_pota_ref"].as<String>();
                  adif += "<MY_SIG:4>POTA ";
                  adif += "<MY_SIG_INFO:" + String(pota.length()) + ">" + pota + " ";
                }

                if (qso.containsKey("their_pota_ref") && qso["their_pota_ref"].as<String>().length() > 0) {
                  String pota = qso["their_pota_ref"].as<String>();
                  adif += "<SIG:4>POTA ";
                  adif += "<SIG_INFO:" + String(pota.length()) + ">" + pota + " ";
                }

                adif += "<EOR>\n";
              }
            }
          }
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }

  return adif;
}

/*
 * Generate CSV export
 */
String generateCSV() {
  extern fs::SPIFFSFS FileSystem;

  String csv = "Callsign,Frequency,Mode,Band,Date,Time,RST Sent,RST Rcvd,Grid,My Grid,My POTA,Their POTA,Notes\n";

  // Iterate through all log files
  File root = FileSystem.open("/logs");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String filename = String(file.name());
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) {
          filename = filename.substring(lastSlash + 1);
        }

        if (filename.startsWith("qso_") && filename.endsWith(".json")) {
          File logFile = FileSystem.open(file.path(), "r");
          if (logFile) {
            String content = logFile.readString();
            logFile.close();

            JsonDocument logDoc;
            DeserializationError error = deserializeJson(logDoc, content);

            if (!error && logDoc.containsKey("logs")) {
              JsonArray logs = logDoc["logs"];
              for (JsonVariant v : logs) {
                JsonObject qso = v.as<JsonObject>();

                csv += qso["callsign"].as<String>() + ",";
                csv += String(qso["frequency"].as<float>(), 3) + ",";
                csv += qso["mode"].as<String>() + ",";
                csv += qso["band"].as<String>() + ",";
                csv += qso["date"].as<String>() + ",";
                csv += qso["time_on"].as<String>() + ",";
                csv += qso["rst_sent"].as<String>() + ",";
                csv += qso["rst_rcvd"].as<String>() + ",";
                csv += qso["gridsquare"].as<String>() + ",";
                csv += qso["my_gridsquare"].as<String>() + ",";
                csv += qso["my_pota_ref"].as<String>() + ",";
                csv += qso["their_pota_ref"].as<String>() + ",";

                // Escape quotes in notes
                String notes = qso["notes"].as<String>();
                notes.replace("\"", "\"\"");
                csv += "\"" + notes + "\"";

                csv += "\n";
              }
            }
          }
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }

  return csv;
}

#endif // WEB_SERVER_H
