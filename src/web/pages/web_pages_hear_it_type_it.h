#ifndef WEB_PAGES_HEAR_IT_TYPE_IT_H
#define WEB_PAGES_HEAR_IT_TYPE_IT_H

const char HEAR_IT_TYPE_IT_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hear It Type It - VAIL SUMMIT</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            color: #333;
        }

        header {
            background: rgba(255, 255, 255, 0.95);
            padding: 1rem 2rem;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }

        .header-content {
            max-width: 1200px;
            margin: 0 auto;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        h1 {
            font-size: 1.5rem;
            color: #667eea;
        }

        .nav-link {
            color: #667eea;
            text-decoration: none;
            padding: 0.5rem 1rem;
            border-radius: 5px;
            transition: background 0.3s;
        }

        .nav-link:hover {
            background: rgba(102, 126, 234, 0.1);
        }

        main {
            flex: 1;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 2rem;
        }

        .training-container {
            background: white;
            border-radius: 15px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            padding: 2rem;
            max-width: 800px;
            width: 100%;
        }

        .title-section {
            text-align: center;
            margin-bottom: 2rem;
        }

        .mode-title {
            font-size: 2rem;
            font-weight: bold;
            color: #667eea;
            margin-bottom: 0.5rem;
        }

        .wpm-display {
            font-size: 1.3rem;
            font-weight: bold;
            color: #f59e0b;
            margin-top: 0.5rem;
        }

        .status-bar {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1.5rem;
            padding: 1rem;
            background: #f7f7f7;
            border-radius: 8px;
        }

        .status-indicator {
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        .status-dot {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            background: #ccc;
            transition: background 0.3s;
        }

        .status-dot.connected {
            background: #10b981;
            animation: pulse 2s infinite;
        }

        .status-dot.playing {
            background: #f59e0b;
            animation: pulse 1s infinite;
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }

        .attempt-counter {
            font-size: 0.9rem;
            color: #f59e0b;
            font-weight: 600;
        }

        .content-area {
            min-height: 200px;
            margin-bottom: 1.5rem;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
        }

        .prompt-text {
            color: #888;
            font-size: 1.1rem;
            margin-bottom: 1rem;
            text-align: center;
        }

        .input-box {
            background: linear-gradient(145deg, #1e3a5f, #2a5885);
            border: 2px solid #4a9eff;
            border-radius: 12px;
            padding: 1.5rem 2rem;
            min-width: 400px;
            max-width: 600px;
            width: 100%;
            text-align: center;
            position: relative;
        }

        .input-text {
            font-size: 2rem;
            font-weight: bold;
            color: white;
            font-family: 'Courier New', monospace;
            letter-spacing: 0.1rem;
            min-height: 2.5rem;
            display: flex;
            align-items: center;
            justify-content: center;
        }

        .cursor {
            display: inline-block;
            width: 3px;
            height: 1.8rem;
            background: #f59e0b;
            margin-left: 0.3rem;
            animation: blink 1s infinite;
            vertical-align: middle;
        }

        @keyframes blink {
            0%, 50% { opacity: 1; }
            51%, 100% { opacity: 0; }
        }

        .playing-message {
            color: #888;
            font-size: 1.2rem;
            text-align: center;
            padding: 2rem;
        }

        .feedback {
            padding: 2rem;
            text-align: center;
            border-radius: 8px;
            margin: 1rem 0;
        }

        .feedback.correct {
            background: rgba(16, 185, 129, 0.1);
        }

        .feedback.incorrect {
            background: rgba(239, 68, 68, 0.1);
        }

        .feedback-title {
            font-size: 1.8rem;
            font-weight: bold;
            margin-bottom: 0.5rem;
        }

        .feedback-title.correct {
            color: #10b981;
        }

        .feedback-title.incorrect {
            color: #ef4444;
        }

        .feedback-text {
            color: #666;
            font-size: 1rem;
        }

        .controls {
            display: flex;
            gap: 1rem;
            justify-content: center;
            flex-wrap: wrap;
        }

        button {
            padding: 0.75rem 2rem;
            font-size: 1rem;
            font-weight: 600;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.3s;
        }

        .btn-primary {
            background: #667eea;
            color: white;
        }

        .btn-primary:hover:not(:disabled) {
            background: #5568d3;
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
        }

        .btn-secondary {
            background: #6b7280;
            color: white;
        }

        .btn-secondary:hover:not(:disabled) {
            background: #4b5563;
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(107, 114, 128, 0.4);
        }

        .btn-success {
            background: #10b981;
            color: white;
        }

        .btn-success:hover:not(:disabled) {
            background: #059669;
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(16, 185, 129, 0.4);
        }

        .btn-warning {
            background: #f59e0b;
            color: white;
        }

        .btn-warning:hover:not(:disabled) {
            background: #d97706;
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(245, 158, 11, 0.4);
        }

        .btn-danger {
            background: #ef4444;
            color: white;
        }

        .btn-danger:hover:not(:disabled) {
            background: #dc2626;
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(239, 68, 68, 0.4);
        }

        button:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }

        .help-text {
            margin-top: 1.5rem;
            padding: 1rem;
            background: #f0f9ff;
            border-left: 4px solid #667eea;
            border-radius: 4px;
            font-size: 0.9rem;
            color: #666;
            text-align: center;
        }

        .instructions {
            margin-top: 1.5rem;
            padding: 1rem;
            background: #f0f9ff;
            border-left: 4px solid #667eea;
            border-radius: 4px;
        }

        .instructions h3 {
            color: #667eea;
            margin-bottom: 0.5rem;
        }

        .instructions ul {
            margin-left: 1.5rem;
        }

        .instructions li {
            margin: 0.5rem 0;
            color: #666;
        }

        .settings-panel {
            background: #f9fafb;
            border: 2px solid #667eea;
            border-radius: 10px;
            padding: 1.5rem;
            margin-bottom: 1.5rem;
        }

        .setting-group {
            margin-bottom: 1rem;
        }

        .setting-group label {
            display: block;
            font-weight: 600;
            color: #374151;
            margin-bottom: 0.5rem;
        }

        .setting-group select,
        .setting-group input {
            width: 100%;
            padding: 0.5rem;
            border: 1px solid #d1d5db;
            border-radius: 5px;
            font-size: 1rem;
        }

        .setting-group input[type="number"] {
            width: 100px;
        }
    </style>
</head>
<body>
    <header>
        <div class="header-content">
            <h1>VAIL SUMMIT - Hear It Type It</h1>
            <a href="/" class="nav-link">← Back to Dashboard</a>
        </div>
    </header>

    <main>
        <div class="training-container">
            <div class="title-section">
                <div class="mode-title">HEAR IT TYPE IT</div>
                <div class="wpm-display" id="wpmDisplay">-- WPM</div>
            </div>

            <div class="status-bar">
                <div class="status-indicator">
                    <div class="status-dot" id="statusDot"></div>
                    <span id="statusText">Disconnected</span>
                </div>
                <div class="attempt-counter" id="attemptCounter"></div>
            </div>

            <div class="content-area" id="contentArea">
                <!-- Content changes based on state -->
            </div>

            <div class="settings-panel" id="settingsPanel">
                <h3 style="color: #667eea; margin-bottom: 1rem;">Training Settings</h3>
                <div class="setting-group">
                    <label for="modeSelect">Mode:</label>
                    <select id="modeSelect" onchange="updateLengthVisibility()">
                        <option value="0">Callsigns</option>
                        <option value="1">Random Letters</option>
                        <option value="2">Random Numbers</option>
                        <option value="3">Letters + Numbers</option>
                        <option value="4">Custom Characters</option>
                    </select>
                </div>
                <div class="setting-group" id="lengthGroup">
                    <label for="lengthInput">Group Length (3-10):</label>
                    <input type="number" id="lengthInput" min="3" max="10" value="5">
                </div>
                <div class="setting-group" id="customGroup" style="display: none;">
                    <label for="customInput">Custom Characters:</label>
                    <input type="text" id="customInput" placeholder="e.g., ABCDE12345" value="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789">
                </div>
            </div>

            <div class="controls" id="controls">
                <button class="btn-primary" id="startBtn" onclick="startTraining()">Start Training</button>
            </div>

            <div class="help-text" id="helpText">
                Configure settings above, then click "Start Training" to begin!
            </div>

            <div class="instructions">
                <h3>How to Play</h3>
                <ul>
                    <li>Choose your training mode and settings above</li>
                    <li>Click "Start Training" to connect and begin</li>
                    <li>Listen to the morse code that plays</li>
                    <li>Type what you heard using your keyboard</li>
                    <li>Press Enter to submit your answer</li>
                    <li>Use Backspace to correct mistakes</li>
                    <li>Click "Replay" to hear it again</li>
                    <li>Click "Skip" to move to the next one</li>
                </ul>
            </div>
        </div>
    </main>

    <script>
        let ws = null;
        let audioContext = null;
        let sessionState = {
            connected: false,
            playing: false,
            waitingForInput: false,
            currentCallsign: '',
            currentMorse: '',
            currentWPM: 0,
            userInput: '',
            attempts: 0
        };

        // Initialize audio context (lazy initialization)
        function initAudio() {
            if (!audioContext) {
                audioContext = new (window.AudioContext || window.webkitAudioContext)();
            }
        }

        // Play morse code tone
        function playTone(frequency, duration) {
            return new Promise((resolve) => {
                const osc = audioContext.createOscillator();
                const gainNode = audioContext.createGain();

                osc.type = 'sine';
                osc.frequency.value = frequency;

                gainNode.gain.value = 0.3;  // Volume

                osc.connect(gainNode);
                gainNode.connect(audioContext.destination);

                osc.start(audioContext.currentTime);
                osc.stop(audioContext.currentTime + duration / 1000);

                setTimeout(resolve, duration);
            });
        }

        // Play morse code pattern
        async function playMorsePattern(patterns, wpm) {
            initAudio();

            // Calculate timing (PARIS standard)
            const ditDuration = 1200 / wpm;  // milliseconds
            const dahDuration = ditDuration * 3;
            const frequency = 700;  // Hz

            const chars = patterns.split(' ');  // Split by spaces (character boundaries)

            for (let i = 0; i < chars.length; i++) {
                const pattern = chars[i];

                // Play each dit/dah in the pattern
                for (let j = 0; j < pattern.length; j++) {
                    const symbol = pattern[j];
                    if (symbol === '.') {
                        await playTone(frequency, ditDuration);
                    } else if (symbol === '-') {
                        await playTone(frequency, dahDuration);
                    }

                    // Inter-element gap (between dits/dahs within a character)
                    if (j < pattern.length - 1) {
                        await new Promise(resolve => setTimeout(resolve, ditDuration));
                    }
                }

                // Inter-character gap (between letters)
                if (i < chars.length - 1) {
                    await new Promise(resolve => setTimeout(resolve, ditDuration * 3));
                }
            }

            console.log('Morse playback complete');
        }

        // Main state update function
        function updateUI() {
            const contentArea = document.getElementById('contentArea');
            const controls = document.getElementById('controls');
            const helpText = document.getElementById('helpText');

            if (!sessionState.connected) {
                // Not connected
                contentArea.innerHTML = '<div class="prompt-text">Not connected. Click "Start Training" to begin.</div>';
                controls.innerHTML = '<button class="btn-primary" onclick="startTraining()">Start Training</button>';
                helpText.textContent = 'Click "Start Training" to begin. You\'ll hear a callsign - type what you hear!';
            } else if (sessionState.playing) {
                // Playing callsign
                contentArea.innerHTML = '<div class="playing-message">🔊 Playing callsign...</div>';
                controls.innerHTML = '<button class="btn-danger" onclick="stopTraining()">Stop Training</button>';
                helpText.textContent = 'Listen carefully to the morse code callsign...';
            } else if (sessionState.waitingForInput) {
                // Waiting for user input
                contentArea.innerHTML = `
                    <div class="prompt-text">Type what you heard:</div>
                    <div class="input-box">
                        <div class="input-text">
                            <span id="inputText">${sessionState.userInput || ''}</span><span class="cursor"></span>
                        </div>
                    </div>
                `;
                controls.innerHTML = `
                    <button class="btn-success" onclick="submitAnswer()">Submit (Enter)</button>
                    <button class="btn-warning" onclick="replayCallsign()">Replay</button>
                    <button class="btn-secondary" onclick="skipCallsign()">Skip</button>
                    <button class="btn-danger" onclick="stopTraining()">Exit</button>
                `;
                helpText.textContent = 'Type the callsign you heard, then press Enter or click Submit.';
            }

            // Update attempt counter
            const attemptCounter = document.getElementById('attemptCounter');
            if (sessionState.attempts > 0) {
                attemptCounter.textContent = `Attempt ${sessionState.attempts + 1}`;
            } else {
                attemptCounter.textContent = '';
            }
        }

        // Keyboard input handler
        document.addEventListener('keydown', (e) => {
            if (!sessionState.waitingForInput) return;

            if (e.key === 'Enter') {
                e.preventDefault();
                submitAnswer();
            } else if (e.key === 'Backspace') {
                e.preventDefault();
                if (sessionState.userInput.length > 0) {
                    sessionState.userInput = sessionState.userInput.slice(0, -1);
                    updateUI();
                }
            } else if (e.key.length === 1 && /[a-zA-Z0-9]/.test(e.key)) {
                e.preventDefault();
                if (sessionState.userInput.length < 10) {
                    sessionState.userInput += e.key.toUpperCase();
                    updateUI();
                }
            }
        });

        // Update visibility of length and custom fields based on mode
        function updateLengthVisibility() {
            const mode = parseInt(document.getElementById('modeSelect').value);
            const lengthGroup = document.getElementById('lengthGroup');
            const customGroup = document.getElementById('customGroup');

            // Hide length for callsigns mode (mode 0)
            lengthGroup.style.display = (mode === 0) ? 'none' : 'block';

            // Show custom chars only for custom mode (mode 4)
            customGroup.style.display = (mode === 4) ? 'block' : 'none';
        }

        function startTraining() {
            // Get settings from UI
            const mode = parseInt(document.getElementById('modeSelect').value);
            const length = parseInt(document.getElementById('lengthInput').value);
            const customChars = document.getElementById('customInput').value;

            // Hide settings panel
            document.getElementById('settingsPanel').style.display = 'none';

            // Send settings to device via POST
            fetch('/api/hear-it/start', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    mode: mode,
                    length: length,
                    customChars: customChars
                })
            })
            .then(response => response.json())
            .then(data => {
                if (data.status === 'active') {
                    connectWebSocket();
                }
            })
            .catch(error => {
                console.error('Failed to start training:', error);
                alert('Failed to start training mode. Please try again.');
                // Show settings panel again on error
                document.getElementById('settingsPanel').style.display = 'block';
            });
        }

        function connectWebSocket() {
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = `${protocol}//${window.location.host}/ws/hear-it`;

            ws = new WebSocket(wsUrl);

            ws.onopen = () => {
                console.log('WebSocket connected');
                sessionState.connected = true;
                updateStatus('connected', 'Connected');
                updateUI();
            };

            ws.onmessage = (event) => {
                const data = JSON.parse(event.data);
                console.log('Received:', data);
                handleWebSocketMessage(data);
            };

            ws.onerror = (error) => {
                console.error('WebSocket error:', error);
            };

            ws.onclose = () => {
                console.log('WebSocket disconnected');
                sessionState.connected = false;
                sessionState.playing = false;
                sessionState.waitingForInput = false;
                updateStatus('disconnected', 'Disconnected');
                updateUI();
                ws = null;
            };
        }

        async function handleWebSocketMessage(data) {
            switch(data.type) {
                case 'new_callsign':
                    sessionState.currentCallsign = data.callsign;
                    sessionState.currentWPM = data.wpm;
                    sessionState.currentMorse = data.morse;  // Store morse pattern
                    sessionState.userInput = '';
                    sessionState.attempts = 0;
                    document.getElementById('wpmDisplay').textContent = `${data.wpm} WPM`;
                    console.log('New callsign:', data.callsign, 'Morse:', data.morse, 'WPM:', data.wpm);
                    break;

                case 'playing':
                    // Device is telling us to play audio in browser
                    sessionState.playing = true;
                    sessionState.waitingForInput = false;
                    updateStatus('playing', 'Playing...');
                    updateUI();

                    // Play morse code audio in browser
                    if (sessionState.currentMorse) {
                        console.log('Playing morse pattern:', sessionState.currentMorse);
                        try {
                            await playMorsePattern(sessionState.currentMorse, sessionState.currentWPM);
                            console.log('Playback finished');

                            // After playback is done, automatically transition to ready for input
                            sessionState.playing = false;
                            sessionState.waitingForInput = true;
                            updateStatus('connected', 'Waiting for input');
                            updateUI();
                        } catch (error) {
                            console.error('Error playing morse:', error);
                        }
                    }
                    break;

                case 'ready_for_input':
                    // Fallback in case device sends this (but browser handles it automatically now)
                    sessionState.playing = false;
                    sessionState.waitingForInput = true;
                    updateStatus('connected', 'Waiting for input');
                    updateUI();
                    break;

                case 'result':
                    handleResult(data.correct, data.answer);
                    break;
            }
        }

        function handleResult(correct, answer) {
            const contentArea = document.getElementById('contentArea');

            if (correct) {
                contentArea.innerHTML = `
                    <div class="feedback correct">
                        <div class="feedback-title correct">✓ CORRECT!</div>
                        <div class="feedback-text">The answer was: ${answer}</div>
                    </div>
                `;
            } else {
                sessionState.attempts++;
                contentArea.innerHTML = `
                    <div class="feedback incorrect">
                        <div class="feedback-title incorrect">✗ INCORRECT</div>
                        <div class="feedback-text">Try again...</div>
                    </div>
                `;
            }

            const controls = document.getElementById('controls');
            controls.innerHTML = '<button class="btn-danger" onclick="stopTraining()">Exit</button>';

            // Device will handle replay (incorrect) or next callsign (correct)
            // Browser just waits for the next "playing" message
        }

        function submitAnswer() {
            if (!ws || !sessionState.waitingForInput || sessionState.userInput.length === 0) return;

            ws.send(JSON.stringify({
                type: 'submit',
                answer: sessionState.userInput
            }));
        }

        function replayCallsign() {
            if (!ws || !sessionState.connected) return;

            ws.send(JSON.stringify({
                type: 'replay'
            }));
        }

        function skipCallsign() {
            if (!ws || !sessionState.connected) return;

            ws.send(JSON.stringify({
                type: 'skip'
            }));
        }

        function stopTraining() {
            if (ws) {
                ws.close();
            }
            sessionState = {
                connected: false,
                playing: false,
                waitingForInput: false,
                currentCallsign: '',
                currentMorse: '',
                currentWPM: 0,
                userInput: '',
                attempts: 0
            };
            updateStatus('disconnected', 'Disconnected');
            updateUI();

            // Show settings panel again
            document.getElementById('settingsPanel').style.display = 'block';
        }

        // Initialize settings visibility on page load
        window.addEventListener('DOMContentLoaded', () => {
            updateLengthVisibility();
        });

        function updateStatus(state, text) {
            const statusDot = document.getElementById('statusDot');
            const statusText = document.getElementById('statusText');

            statusDot.className = 'status-dot';
            if (state === 'connected') {
                statusDot.classList.add('connected');
            } else if (state === 'playing') {
                statusDot.classList.add('playing');
            }

            statusText.textContent = text;
        }

        // Initialize UI
        updateUI();
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGES_HEAR_IT_TYPE_IT_H
