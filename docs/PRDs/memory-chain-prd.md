# Memory Chain - Morse Code Game PRD

## Overview
Memory Chain is a progressive memory game that challenges users to remember and reproduce increasingly long sequences of Morse code characters. The game tests both character recognition and short-term memory skills, similar to the classic "Simon Says" electronic game but using CW.

## Game Objective
Players must listen to sequences of Morse code characters and reproduce them accurately using their paddle/key. Each successful reproduction adds one more character to the sequence. The goal is to achieve the longest possible chain before making a mistake.

## Core Gameplay Loop

### Round Flow
1. Game sends a Morse code character through the piezo buzzer
2. Player must send back that same character using their key
3. If correct, game sends the previous character(s) PLUS one new character
4. Player must reproduce the entire sequence in order
5. Continue until player makes a mistake

### Example Sequence
- Round 1: Game sends "A" → Player sends "A"
- Round 2: Game sends "A T" → Player sends "A T"  
- Round 3: Game sends "A T E" → Player sends "A T E"
- Round 4: Game sends "A T E N" → Player sends "A T E N"
- Player makes mistake → Game Over

## Display Elements

### During Gameplay
- Current chain length (e.g., "Chain: 7")
- Game state indicator ("Listen" / "Your Turn" / "Correct!" / "Wrong!")
- Optional: Visual representation of sequence as it plays (dots/dashes or letters)
- Lives remaining (if using multi-life mode)

### Between Rounds
- Brief pause with visual feedback after player completes sequence
- Clear indication when adding new character to chain

### Game Over Screen
- Final chain length achieved
- High score for this session
- All-time best score (stored in flash memory)
- Options: "Play Again" / "Main Menu"

## Settings & Difficulty Options

### Character Set Selection
- Beginner: Letters only (A-Z)
- Intermediate: Letters + Numbers (0-9)
- Advanced: Letters + Numbers + Prosigns (SK, AR, BT, etc.)
- Custom: User selects specific characters to practice

### Speed Settings
- WPM range: 10-30 WPM (adjustable)
- Farnsworth spacing option for slower effective speeds
- Speed stays constant throughout game (doesn't auto-increase)

### Game Modes

**Standard Mode**
- One mistake = game over
- Pure score chase

**Practice Mode**  
- 3 lives/hearts
- Mistake costs one life but game continues
- Allows players to recover and keep practicing

**Timed Challenge**
- Complete as many rounds as possible in 60 seconds
- No game over on mistakes, just keeps going
- Score = successful chains completed

## Audio Feedback

### Game Sounds (through piezo)
- Character playback at set WPM
- Short "correct" tone (higher pitch beep)
- Short "incorrect" tone (lower buzz)
- "New high score" fanfare

### Player Sounds
- Player's key input echoes through buzzer as they send
- Allows player to hear their own timing

## Input Handling

### Character Recognition
- Must detect complete character sent by player
- Allow reasonable timing tolerance for dit/dah recognition
- Timeout after reasonable pause indicates character is complete (~3-5 dit lengths)

### Validation
- Compare player's sent character to expected character
- Provide immediate feedback (correct/incorrect)
- On mistake, optionally show what was expected vs. what was received

## Data Persistence

### Saved Data
- All-time high score (chain length)
- High score per difficulty setting
- Preferred game mode
- Preferred speed setting
- Preferred character set

### Statistics (Optional Enhancement)
- Total games played
- Average chain length
- Most common mistake characters
- Practice time logged

## User Interface Flow

### Main Menu
```
MEMORY CHAIN
> Start Game
  Settings
  High Scores
  Back
```

### Settings Menu
```
SETTINGS
  Speed: [15 WPM]
  Character Set: [Letters Only]
  Game Mode: [Standard]
  Sound: [On]
  Back
```

### Gameplay Screen
```
Chain: 4
━━━━━━━━━━━━━━
YOUR TURN

♥ ♥ ♥
```

## Edge Cases & Error Handling

### Player Input Issues
- Extra dits/dahs before/after character: Ignore leading/trailing noise
- Very long pause: Treat as character end, evaluate what was sent
- Rapid mistakes: Don't allow "button mashing" to progress

### Game State Issues
- Player disconnects key mid-game: Pause game, show reconnection prompt
- Player sends character during "Listen" phase: Ignore input, don't count as mistake

### Display Issues
- Chain too long for display: Scroll or show "Chain: 47" instead of full sequence
- Very long sequences: Consider maximum chain length cap (e.g., 99)

## Success Metrics

### Player Engagement
- Average session length
- Repeat play rate
- Difficulty progression (do players increase settings over time?)

### Learning Effectiveness
- Improvement in chain length over multiple sessions
- Character accuracy by character (which ones cause most failures?)

### Technical Performance
- Accurate character recognition rate
- Input latency (should feel immediate)

## Future Enhancements (Out of Scope for V1)

- Two-player mode (alternate turns, first to fail loses)
- Online leaderboards
- Unlock new character sets by reaching chain milestones
- Daily challenge with fixed seed for global competition
- Variable speed mode (speeds up as chain gets longer)
- Theme variations (different sounds, visual styles)

## Technical Requirements

### Hardware
- ESP32-S3 microcontroller
- Waveshare LCD display
- CardKB keyboard (for menu navigation)
- Piezo buzzer
- Morse key/paddle input

### Performance
- Responsive input detection (<50ms latency)
- Smooth display updates
- Reliable character recognition
- Stable WPM timing accuracy

### Storage
- Minimal flash storage for high scores and settings
- No internet connectivity required
- Standalone operation

## Open Questions

1. Should incorrect attempts show what the player actually sent vs. what was expected?
2. Should there be a maximum chain length to prevent infinite games?
3. Should speed gradually increase as chain grows (even slightly)?
4. Should practice mode show the expected character on screen during player's turn?
5. Is there value in showing the full sequence visually, or is pure audio better for training?

## Success Criteria for V1

- Game is playable and fun with basic settings
- Character recognition works reliably with player's key
- High score persistence works across power cycles
- Clear visual and audio feedback for all game states
- At least two difficulty levels functional
- Players can complete 10+ character chains with practice
