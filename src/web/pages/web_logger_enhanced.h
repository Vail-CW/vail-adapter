/*
 * Enhanced QSO Logger Web Interface
 * Provides complete CRUD operations, station settings, and map visualization
 */

#ifndef WEB_LOGGER_ENHANCED_H
#define WEB_LOGGER_ENHANCED_H

// This file contains the enhanced HTML/JS for the logger page
// Separated to keep web_server.h manageable

const char LOGGER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>QSO Logger - VAIL SUMMIT</title>
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #fff;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 1600px; margin: 0 auto; }
        header {
            background: rgba(255,255,255,0.1);
            padding: 20px;
            border-radius: 15px;
            margin-bottom: 20px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            backdrop-filter: blur(10px);
            flex-wrap: wrap;
            gap: 15px;
        }
        h1 { font-size: 2rem; }
        .station-info {
            display: flex;
            gap: 15px;
            align-items: center;
            flex-wrap: wrap;
        }
        .station-badge {
            background: rgba(0,212,255,0.2);
            padding: 8px 15px;
            border-radius: 8px;
            font-size: 0.9rem;
            cursor: pointer;
            transition: all 0.2s;
        }
        .station-badge:hover { background: rgba(0,212,255,0.3); }
        .btn {
            padding: 10px 20px;
            background: rgba(255,255,255,0.2);
            color: #fff;
            text-decoration: none;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            cursor: pointer;
            font-size: 1rem;
            transition: all 0.2s;
        }
        .btn:hover { background: rgba(255,255,255,0.3); }
        .btn-primary {
            background: #00d4ff;
            color: #1e3c72;
            border: none;
            font-weight: 600;
        }
        .btn-primary:hover { background: #00b8e6; }
        .btn-success {
            background: #28a745;
            border: none;
        }
        .btn-success:hover { background: #218838; }
        .btn-danger {
            background: #dc3545;
            border: none;
        }
        .btn-danger:hover { background: #c82333; }
        .controls {
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
            flex-wrap: wrap;
        }
        .search-box {
            flex: 1;
            min-width: 200px;
            padding: 10px;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            background: rgba(255,255,255,0.1);
            color: #fff;
            font-size: 1rem;
        }
        .search-box::placeholder { color: rgba(255,255,255,0.6); }
        .stats {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
            margin-bottom: 20px;
        }
        .stat-card {
            background: rgba(255,255,255,0.1);
            padding: 15px 20px;
            border-radius: 10px;
            backdrop-filter: blur(10px);
            text-align: center;
        }
        .stat-value { font-size: 2rem; font-weight: bold; }
        .stat-label { font-size: 0.85rem; opacity: 0.8; margin-top: 5px; }
        #map {
            height: 300px;
            border-radius: 15px;
            margin-bottom: 20px;
            display: none;
        }
        .table-container {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            overflow-x: auto;
            backdrop-filter: blur(10px);
        }
        table {
            width: 100%;
            border-collapse: collapse;
            min-width: 1000px;
        }
        th {
            background: rgba(0,0,0,0.3);
            padding: 15px;
            text-align: left;
            font-weight: 600;
            position: sticky;
            top: 0;
        }
        td {
            padding: 12px 15px;
            border-bottom: 1px solid rgba(255,255,255,0.1);
        }
        tr:hover { background: rgba(255,255,255,0.05); }
        .callsign {
            font-weight: bold;
            color: #00d4ff;
            font-size: 1.1rem;
        }
        .actions {
            display: flex;
            gap: 5px;
        }
        .btn-sm {
            padding: 5px 10px;
            font-size: 0.85rem;
        }
        .modal {
            display: none;
            position: fixed;
            z-index: 1000;
            left: 0;
            top: 0;
            width: 100%;
            height: 100%;
            background: rgba(0,0,0,0.7);
            backdrop-filter: blur(5px);
        }
        .modal-content {
            background: linear-gradient(135deg, #2a5298 0%, #1e3c72 100%);
            margin: 5% auto;
            padding: 30px;
            border-radius: 15px;
            max-width: 600px;
            max-height: 80vh;
            overflow-y: auto;
        }
        .modal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
        }
        .close {
            font-size: 2rem;
            cursor: pointer;
            opacity: 0.7;
        }
        .close:hover { opacity: 1; }
        .form-group {
            margin-bottom: 15px;
        }
        .form-group label {
            display: block;
            margin-bottom: 5px;
            font-weight: 600;
        }
        .form-group input, .form-group select, .form-group textarea {
            width: 100%;
            padding: 10px;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            background: rgba(255,255,255,0.1);
            color: #fff;
            font-size: 1rem;
        }
        .form-group textarea {
            resize: vertical;
            min-height: 80px;
        }
        .form-row {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
        }
        .loading { text-align: center; padding: 40px; font-size: 1.2rem; }
        .empty { text-align: center; padding: 60px 20px; opacity: 0.7; }
        .empty-icon { font-size: 4rem; margin-bottom: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <div>
                <h1>📝 QSO Logger</h1>
            </div>
            <div class="station-info">
                <div class="station-badge" onclick="openStationSettings()">
                    📡 <span id="stationCall">NOCALL</span>
                </div>
                <div class="station-badge" onclick="openStationSettings()">
                    📍 <span id="stationGrid">----</span>
                </div>
                <a href="/" class="btn">← Dashboard</a>
            </div>
        </header>

        <div class="stats">
            <div class="stat-card">
                <div class="stat-value" id="totalQSOs">0</div>
                <div class="stat-label">Total QSOs</div>
            </div>
            <div class="stat-card">
                <div class="stat-value" id="todayQSOs">0</div>
                <div class="stat-label">Today</div>
            </div>
            <div class="stat-card">
                <div class="stat-value" id="uniqueCall">0</div>
                <div class="stat-label">Unique Calls</div>
            </div>
        </div>

        <div id="map"></div>

        <div class="controls">
            <input type="text" class="search-box" id="searchBox" placeholder="🔍 Search by callsign, band, mode...">
            <button class="btn btn-success" onclick="openNewQSOModal()">+ New QSO</button>
            <button class="btn btn-primary" onclick="toggleMap()">🗺️ Map</button>
            <button class="btn btn-primary" onclick="exportADIF()">📥 ADIF</button>
            <button class="btn btn-primary" onclick="exportCSV()">📥 CSV</button>
        </div>

        <div class="table-container">
            <div id="loading" class="loading">Loading logs...</div>
            <div id="empty" class="empty" style="display:none;">
                <div class="empty-icon">📭</div>
                <h2>No QSOs Yet</h2>
                <p>Start logging contacts by clicking "+ New QSO" above.</p>
            </div>
            <table id="qsoTable" style="display:none;">
                <thead>
                    <tr>
                        <th>Date/Time</th>
                        <th>Callsign</th>
                        <th>Freq/Band</th>
                        <th>Mode</th>
                        <th>RST</th>
                        <th>Grid</th>
                        <th>POTA</th>
                        <th>Actions</th>
                    </tr>
                </thead>
                <tbody id="qsoTableBody"></tbody>
            </table>
        </div>
    </div>

    <!-- Station Settings Modal -->
    <div id="stationModal" class="modal">
        <div class="modal-content">
            <div class="modal-header">
                <h2>Station Settings</h2>
                <span class="close" onclick="closeStationModal()">&times;</span>
            </div>
            <form id="stationForm" onsubmit="saveStationSettings(event)">
                <div class="form-group">
                    <label>Callsign</label>
                    <input type="text" id="stationCallsign" required>
                </div>
                <div class="form-group">
                    <label>Grid Square</label>
                    <input type="text" id="stationGridsquare" maxlength="8" placeholder="EM69xu">
                </div>
                <div class="form-group">
                    <label>POTA Reference</label>
                    <input type="text" id="stationPOTA" placeholder="US-2256">
                </div>
                <button type="submit" class="btn btn-primary" style="width:100%">Save Settings</button>
            </form>
        </div>
    </div>

    <!-- QSO Edit/New Modal -->
    <div id="qsoModal" class="modal">
        <div class="modal-content">
            <div class="modal-header">
                <h2 id="qsoModalTitle">New QSO</h2>
                <span class="close" onclick="closeQSOModal()">&times;</span>
            </div>
            <form id="qsoForm" onsubmit="saveQSO(event)">
                <input type="hidden" id="qsoId">
                <input type="hidden" id="qsoDate">

                <div class="form-group">
                    <label>Callsign *</label>
                    <input type="text" id="qsoCallsign" required
                           minlength="3" maxlength="10"
                           pattern="[A-Za-z0-9]+"
                           placeholder="W1ABC"
                           title="3-10 alphanumeric characters with at least one digit">
                </div>

                <div class="form-row">
                    <div class="form-group">
                        <label>Frequency (MHz) *</label>
                        <input type="number" id="qsoFrequency" step="0.001" required
                               min="1.8" max="1300"
                               placeholder="14.025"
                               title="Frequency between 1.8 and 1300 MHz">
                    </div>
                    <div class="form-group">
                        <label>Mode *</label>
                        <select id="qsoMode" required>
                            <option>CW</option>
                            <option>SSB</option>
                            <option>FM</option>
                            <option>AM</option>
                            <option>FT8</option>
                            <option>FT4</option>
                            <option>RTTY</option>
                            <option>PSK31</option>
                        </select>
                    </div>
                </div>

                <div class="form-row">
                    <div class="form-group">
                        <label>RST Sent</label>
                        <input type="text" id="qsoRSTSent" value="599" maxlength="3">
                    </div>
                    <div class="form-group">
                        <label>RST Received</label>
                        <input type="text" id="qsoRSTRcvd" value="599" maxlength="3">
                    </div>
                </div>

                <div class="form-row">
                    <div class="form-group">
                        <label>Their Grid</label>
                        <input type="text" id="qsoGrid" maxlength="8" placeholder="EN61lp">
                    </div>
                    <div class="form-group">
                        <label>Their POTA</label>
                        <input type="text" id="qsoPOTA" placeholder="US-2254">
                    </div>
                </div>

                <div class="form-group">
                    <label>Notes</label>
                    <textarea id="qsoNotes" placeholder="Optional notes..."></textarea>
                </div>

                <div style="display:flex; gap:10px;">
                    <button type="submit" class="btn btn-success" style="flex:1">Save QSO</button>
                    <button type="button" class="btn" onclick="closeQSOModal()" style="flex:1">Cancel</button>
                </div>
            </form>
        </div>
    </div>

    <script>
        let allQSOs = [];
        let stationSettings = { callsign: 'NOCALL', grid: '', pota: '' };
        let map = null;

        // Initialize
        async function init() {
            await loadStationSettings();
            await loadQSOs();
        }

        // Load station settings
        async function loadStationSettings() {
            try {
                const response = await fetch('/api/settings/station');
                const data = await response.json();
                stationSettings = data;
                document.getElementById('stationCall').textContent = data.callsign || 'NOCALL';
                document.getElementById('stationGrid').textContent = data.grid || '----';
            } catch (error) {
                console.error('Failed to load station settings:', error);
            }
        }

        // Save station settings
        async function saveStationSettings(event) {
            event.preventDefault();
            const settings = {
                callsign: document.getElementById('stationCallsign').value,
                grid: document.getElementById('stationGridsquare').value,
                pota: document.getElementById('stationPOTA').value
            };

            try {
                const response = await fetch('/api/settings/station', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(settings)
                });

                if (response.ok) {
                    await loadStationSettings();
                    closeStationModal();
                    alert('Station settings saved!');
                } else {
                    alert('Failed to save settings');
                }
            } catch (error) {
                alert('Error saving settings');
            }
        }

        // Load QSO logs
        async function loadQSOs() {
            try {
                const response = await fetch('/api/qsos');
                const data = await response.json();
                allQSOs = data.logs || [];

                document.getElementById('totalQSOs').textContent = data.total || 0;

                const today = new Date().toISOString().split('T')[0].replace(/-/g, '');
                const todayCount = allQSOs.filter(q => q.date === today).length;
                document.getElementById('todayQSOs').textContent = todayCount;

                const uniqueCalls = new Set(allQSOs.map(q => q.callsign)).size;
                document.getElementById('uniqueCall').textContent = uniqueCalls;

                displayQSOs(allQSOs);

                document.getElementById('loading').style.display = 'none';

                if (allQSOs.length === 0) {
                    document.getElementById('empty').style.display = 'block';
                } else {
                    document.getElementById('qsoTable').style.display = 'table';
                }
            } catch (error) {
                console.error('Failed to load QSOs:', error);
                document.getElementById('loading').textContent = 'Error loading logs';
            }
        }

        // Display QSOs in table
        function displayQSOs(qsos) {
            const tbody = document.getElementById('qsoTableBody');
            tbody.innerHTML = '';

            qsos.forEach(qso => {
                const row = tbody.insertRow();
                const date = qso.date.substring(0,4) + '-' + qso.date.substring(4,6) + '-' + qso.date.substring(6,8);
                const time = qso.time_on.substring(0,2) + ':' + qso.time_on.substring(2,4);

                row.innerHTML = `
                    <td>${date}<br/><small>${time}Z</small></td>
                    <td class="callsign">${qso.callsign}</td>
                    <td>${qso.frequency.toFixed(3)}<br/><small>${qso.band}</small></td>
                    <td>${qso.mode}</td>
                    <td>${qso.rst_sent}/${qso.rst_rcvd}</td>
                    <td>${qso.gridsquare || '-'}</td>
                    <td>${qso.their_pota_ref || '-'}</td>
                    <td class="actions">
                        <button class="btn btn-sm btn-primary" onclick='editQSO(${JSON.stringify(qso)})'>Edit</button>
                        <button class="btn btn-sm btn-danger" onclick="deleteQSO('${qso.date}', ${qso.id})">Del</button>
                    </td>
                `;
            });
        }

        // Search functionality
        document.getElementById('searchBox').addEventListener('input', (e) => {
            const searchTerm = e.target.value.toLowerCase();
            const filtered = allQSOs.filter(qso =>
                qso.callsign.toLowerCase().includes(searchTerm) ||
                qso.band.toLowerCase().includes(searchTerm) ||
                qso.mode.toLowerCase().includes(searchTerm) ||
                (qso.notes && qso.notes.toLowerCase().includes(searchTerm))
            );
            displayQSOs(filtered);
        });

        // Modals
        function openStationSettings() {
            document.getElementById('stationCallsign').value = stationSettings.callsign;
            document.getElementById('stationGridsquare').value = stationSettings.grid;
            document.getElementById('stationPOTA').value = stationSettings.pota;
            document.getElementById('stationModal').style.display = 'block';
        }

        function closeStationModal() {
            document.getElementById('stationModal').style.display = 'none';
        }

        function openNewQSOModal() {
            document.getElementById('qsoModalTitle').textContent = 'New QSO';
            document.getElementById('qsoForm').reset();
            document.getElementById('qsoId').value = '';
            document.getElementById('qsoDate').value = '';
            const now = new Date();
            document.getElementById('qsoFrequency').value = '14.025';
            document.getElementById('qsoModal').style.display = 'block';
        }

        function editQSO(qso) {
            document.getElementById('qsoModalTitle').textContent = 'Edit QSO';
            document.getElementById('qsoId').value = qso.id;
            document.getElementById('qsoDate').value = qso.date;
            document.getElementById('qsoCallsign').value = qso.callsign;
            document.getElementById('qsoFrequency').value = qso.frequency;
            document.getElementById('qsoMode').value = qso.mode;
            document.getElementById('qsoRSTSent').value = qso.rst_sent;
            document.getElementById('qsoRSTRcvd').value = qso.rst_rcvd;
            document.getElementById('qsoGrid').value = qso.gridsquare || '';
            document.getElementById('qsoPOTA').value = qso.their_pota_ref || '';
            document.getElementById('qsoNotes').value = qso.notes || '';
            document.getElementById('qsoModal').style.display = 'block';
        }

        function closeQSOModal() {
            document.getElementById('qsoModal').style.display = 'none';
        }

        // Validate callsign format (must have at least one digit)
        function validateCallsign(callsign) {
            if (!callsign || callsign.length < 3 || callsign.length > 10) {
                return 'Callsign must be 3-10 characters';
            }
            if (!/^[A-Za-z0-9]+$/.test(callsign)) {
                return 'Callsign must be alphanumeric only';
            }
            if (!/\d/.test(callsign)) {
                return 'Callsign must contain at least one digit';
            }
            return null;
        }

        // Validate frequency
        function validateFrequency(freq) {
            if (isNaN(freq) || freq < 1.8 || freq > 1300) {
                return 'Frequency must be between 1.8 and 1300 MHz';
            }
            return null;
        }

        // Save QSO (new or edit)
        async function saveQSO(event) {
            event.preventDefault();

            const callsign = document.getElementById('qsoCallsign').value.trim().toUpperCase();
            const frequency = parseFloat(document.getElementById('qsoFrequency').value);

            // Validate callsign
            const callsignError = validateCallsign(callsign);
            if (callsignError) {
                alert(callsignError);
                return;
            }

            // Validate frequency
            const frequencyError = validateFrequency(frequency);
            if (frequencyError) {
                alert(frequencyError);
                return;
            }

            const qsoData = {
                id: document.getElementById('qsoId').value || Date.now(),
                date: document.getElementById('qsoDate').value || new Date().toISOString().split('T')[0].replace(/-/g, ''),
                time_on: new Date().toISOString().substring(11,16).replace(':', ''),
                callsign: callsign,
                frequency: frequency,
                mode: document.getElementById('qsoMode').value,
                rst_sent: document.getElementById('qsoRSTSent').value || '599',
                rst_rcvd: document.getElementById('qsoRSTRcvd').value || '599',
                gridsquare: document.getElementById('qsoGrid').value.trim().toUpperCase(),
                their_pota_ref: document.getElementById('qsoPOTA').value.trim().toUpperCase(),
                notes: document.getElementById('qsoNotes').value.trim(),
                my_gridsquare: stationSettings.grid,
                my_pota_ref: stationSettings.pota
            };

            const isNew = !document.getElementById('qsoDate').value;
            const url = isNew ? '/api/qsos/create' : '/api/qsos/update';

            try {
                const response = await fetch(url, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(qsoData)
                });

                if (response.ok) {
                    closeQSOModal();
                    await loadQSOs();
                    alert(isNew ? 'QSO created!' : 'QSO updated!');
                } else {
                    const error = await response.json();
                    alert('Failed to save QSO: ' + (error.error || 'Unknown error'));
                }
            } catch (error) {
                alert('Error saving QSO: ' + error.message);
            }
        }

        // Delete QSO
        async function deleteQSO(date, id) {
            if (!confirm('Delete this QSO? This cannot be undone!')) return;

            try {
                const response = await fetch(`/api/qsos/delete?date=${date}&id=${id}`, {
                    method: 'DELETE'
                });

                if (response.ok) {
                    await loadQSOs();
                    alert('QSO deleted');
                } else {
                    alert('Failed to delete QSO');
                }
            } catch (error) {
                alert('Error deleting QSO');
            }
        }

        // Map functions
        function toggleMap() {
            const mapDiv = document.getElementById('map');
            if (mapDiv.style.display === 'none') {
                mapDiv.style.display = 'block';
                initMap();
                updateMapMarkers();
            } else {
                mapDiv.style.display = 'none';
            }
        }

        function initMap() {
            if (map) {
                map.invalidateSize();
                return;
            }

            map = L.map('map').setView([39.8283, -98.5795], 4);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                attribution: '© OpenStreetMap'
            }).addTo(map);

            setTimeout(() => { map.invalidateSize(); }, 100);
        }

        function updateMapMarkers() {
            if (!map) return;

            // Clear existing markers
            map.eachLayer(layer => {
                if (layer instanceof L.Marker) {
                    map.removeLayer(layer);
                }
            });

            // Add markers for today's QSOs with grids
            const today = new Date().toISOString().split('T')[0].replace(/-/g, '');
            const todayQSOs = allQSOs.filter(q => q.date === today && q.gridsquare);

            console.log('Today:', today);
            console.log('QSOs with grids today:', todayQSOs.length);

            todayQSOs.forEach(qso => {
                console.log('QSO:', qso.callsign, 'Grid:', qso.gridsquare);
                const coords = gridToLatLon(qso.gridsquare);
                console.log('Coords:', coords);
                if (coords) {
                    L.marker(coords).addTo(map)
                        .bindPopup(`<strong>${qso.callsign}</strong><br/>${qso.gridsquare}`);
                }
            });
        }

        // Grid square to lat/lon conversion (simplified)
        function gridToLatLon(grid) {
            if (!grid || grid.length < 4) return null;
            const lon = (grid.charCodeAt(0) - 65) * 20 - 180 + (grid.charCodeAt(2) - 48) * 2 + 1;
            const lat = (grid.charCodeAt(1) - 65) * 10 - 90 + (grid.charCodeAt(3) - 48) + 0.5;
            return [lat, lon];
        }

        // Export functions
        function exportADIF() {
            window.location.href = '/api/export/adif';
        }

        function exportCSV() {
            window.location.href = '/api/export/csv';
        }

        // Initialize on load
        init();
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_LOGGER_ENHANCED_H
