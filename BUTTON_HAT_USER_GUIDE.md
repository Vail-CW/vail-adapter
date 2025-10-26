# Vail Adapter Button Hat - User Guide

Welcome to your Vail Adapter with Button Hat! This guide will help you get started with the three-button interface that lets you adjust settings without using a computer.

---

## What Can the Buttons Do?

The button hat gives you control over three important settings:
- **Speed** - Adjust your keyer speed (5-40 WPM)
- **Tone** - Change your sidetone frequency (42 different pitches)
- **Key Type** - Switch between Straight Key, Iambic A, and Iambic B modes

All feedback is through Morse code and tones played on your piezo buzzer!

---

## Button Layout

When looking down at your Vail Adapter from above (button side up), with the USB port on the left:

```
       USB  [1]  [2]  [3]

    
```

- **Button 1** (Leftmost, closest to USB) - Speed settings
- **Button 2** (Middle) - Tone settings
- **Button 3** (Rightmost, furthest from USB) - Key type settings

---

## Understanding Button Presses

There are two types of button presses you'll use:

### Quick Press
Press and release quickly (less than 1 second)
- Used to adjust values up or down

### Long Press
Hold for 2 seconds until you hear audio feedback
- Used to enter settings modes or save changes
- You can release the button once you hear the announcement

---

## Quick Reference Card

| Action | What It Does |
|--------|--------------|
| **Hold Button 1** (2 sec) | Enter Speed Mode - hear "SPEED" |
| **Hold Button 2** (2 sec) | Enter Tone Mode - hear "TONE" |
| **Hold Button 3** (2 sec) | Enter Key Type Mode - hear "KEY" |
| | |
| **In any setting mode:** | |
| Quick press Button 1 | Increase value |
| Quick press Button 3 | Decrease value |
| **Hold Button 2** (2 sec) | Save and exit - hear "RR" |
| Wait 30 seconds | Auto-save and exit - hear descending tones |

---

## How to Adjust Speed (WPM)

Speed controls how fast your keyer sends code, from 5 WPM (slow) to 40 WPM (fast).

**Step by step:**

1. **Hold Button 1** for 2 seconds
   - When you hear "SPEED" in Morse, release the button

2. **Adjust the speed:**
   - **Button 1** (quick press) = Increase speed by 1 WPM
   - **Button 3** (quick press) = Decrease speed by 1 WPM
   - Each adjustment plays a beep (higher pitch for increase, lower for decrease)
   - **Try it out!** Use your paddle to test the new speed

3. **When you hit the limits:**
   - Minimum is 5 WPM, maximum is 40 WPM
   - If you try to go beyond, you'll hear a low buzz error tone

4. **Save your new speed:**
   - **Hold Button 2** for 2 seconds
   - When you hear "RR" in Morse, release the button
   - You're back to normal mode with your new speed saved!

**OR** just wait 30 seconds - it will auto-save and you'll hear descending tones.

---

## How to Adjust Tone (Sidetone Frequency)

Tone controls the pitch of your sidetone when keying.

**Step by step:**

1. **Hold Button 2** for 2 seconds
   - When you hear "TONE" in Morse, release the button

2. **Adjust the tone:**
   - **Button 1** (quick press) = Higher pitch
   - **Button 3** (quick press) = Lower pitch
   - Each adjustment plays a 100ms beep at the NEW frequency
   - **Try it out!** Use your paddle to hear the new tone

3. **When you hit the limits:**
   - There are 42 different tones available (middle range of frequencies)
   - If you try to go beyond either end, you'll hear a low buzz error tone

4. **Save your new tone:**
   - **Hold Button 2** for 2 seconds
   - When you hear "RR" in Morse, release the button
   - You're back to normal mode with your new tone saved!

**OR** just wait 30 seconds - it will auto-save and you'll hear descending tones.

---

## How to Change Key Type

Key type controls how your paddle input is processed.

**The three key types:**
- **Straight (S)** - Acts like a straight key, no automatic timing
- **Iambic A (IA)** - Automatic alternating dits and dahs
- **Iambic B (IB)** - Like Iambic A with squeeze timing (most popular!)

**Step by step:**

1. **Hold Button 3** for 2 seconds
   - When you hear "KEY" in Morse, release the button

2. **Cycle through key types:**
   - **Button 1** (quick press) = Next type forward
   - **Button 3** (quick press) = Previous type backward
   - You'll hear the type identifier in Morse:
     - **S** = ... (dit dit dit) for Straight
     - **IA** = .. .- (I then A) for Iambic A
     - **IB** = .. -... (I then B) for Iambic B
   - **Try it out!** Use your paddle to test the keyer behavior

3. **Cycling order:**
   - Forward: Straight → Iambic A → Iambic B → Straight...
   - Backward: Straight → Iambic B → Iambic A → Straight...

4. **Save your key type:**
   - **Hold Button 2** for 2 seconds
   - When you hear "RR" in Morse, release the button
   - You're back to normal mode with your new key type saved!

**OR** just wait 30 seconds - it will auto-save and you'll hear descending tones.

---

## Audio Feedback Guide

All feedback comes through your piezo buzzer in Morse code and tones.

### Morse Announcements

| Sound | Meaning |
|-------|---------|
| "SPEED" | Entered speed setting mode |
| "TONE" | Entered tone setting mode |
| "KEY" | Entered key type mode |
| "RR" | Settings saved, returning to normal mode |
| "S" (dit dit dit) | Straight key mode |
| "IA" (I then A) | Iambic A mode |
| "IB" (I then B) | Iambic B mode |

### Special Tones

| Sound | Meaning |
|-------|---------|
| Short beep (higher pitch) | Increased a setting |
| Short beep (lower pitch) | Decreased a setting |
| Low buzz (200 Hz) | Error - you hit the minimum or maximum limit |
| Descending tones (7 steps) | Auto-save timeout - settings saved |
| 100ms beep at new frequency | New tone frequency preview (in tone mode) |

---

## Tips & Tricks

### Testing While Adjusting
Unlike other CW devices, you can **test while you adjust**:
- In Speed Mode: Key with your paddle to hear the new speed
- In Tone Mode: Key with your paddle to hear the new tone
- In Key Type Mode: Key with your paddle to test the keyer behavior

Changes take effect immediately, so you can find the perfect setting before saving!

### Activity Timeout
The 30-second auto-save timer resets whenever you:
- Press any button
- Use your paddle or straight key

So feel free to take your time testing different settings!

### What Gets Saved
All three settings are saved to EEPROM and survive power cycles:
- Your speed (WPM)
- Your tone (frequency)
- Your key type (Straight/Iambic A/Iambic B)

### Starting From Scratch
If you ever want to know your current settings:
- Enter each mode (the announcement plays at your current setting)
- Speed: Watch the serial monitor for "Current speed: XX WPM"
- Tone: Watch for "Current tone: MIDI note XX (XXX Hz)"
- Key Type: Watch for "Current keyer type: [name]"

### No Computer Needed
Once you're comfortable with the buttons, you can adjust all three settings without ever connecting to a computer. Perfect for field operations or portable setups!

---

## Troubleshooting

**Problem:** Buttons don't seem to respond
- Make sure you're holding long enough (2 seconds for mode entry)
- Wait for the audio announcement before releasing
- Try a single button at a time first

**Problem:** I'm not hearing any audio feedback
- Check that your piezo buzzer is connected
- Verify you haven't disabled the buzzer (hold DIT for 5 seconds re-enables it)

**Problem:** Settings don't save
- Make sure you either hold Button 2 for "RR" confirmation
- Or wait for the descending tones (30-second auto-save)
- If you exit accidentally (power loss), changes are NOT saved

**Problem:** I don't know which mode I'm in
- Just wait - if you don't press anything for 30 seconds, you'll auto-exit
- Or try entering a mode - you can always save without changing anything

**Problem:** Speed or tone is at the wrong limit
- Press the opposite button to move away from the limit
- Speed range: 5-40 WPM
- Tone range: MIDI notes 43-85 (42 different tones)

---

## Default Settings

When you first flash the firmware or reset EEPROM:
- **Speed:** 12 WPM
- **Tone:** MIDI note 69 (440 Hz - concert A)
- **Key Type:** Iambic B

---

## Summary Cheat Sheet

```
MODE ENTRY (hold 2 seconds):
├─ Button 1 → Speed Mode   ("SPEED")
├─ Button 2 → Tone Mode    ("TONE")
└─ Button 3 → Key Type Mode ("KEY")

IN ANY SETTING MODE:
├─ Button 1 (quick) → Increase
├─ Button 3 (quick) → Decrease
├─ Button 2 (hold 2s) → Save & exit ("RR")
└─ Wait 30s → Auto-save (descending tones)

TEST WHILE ADJUSTING:
└─ Use your CW paddle to test immediately!
```

---

## Getting Help

If you have questions or run into issues:
- Check the serial monitor (9600 baud) for detailed feedback
- Review this guide for step-by-step instructions
- Make sure your firmware is up to date

**Happy keying! 73**
