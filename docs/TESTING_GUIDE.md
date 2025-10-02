# Vail Adapter Firmware Update Website - Manual Testing Guide

This guide outlines the steps to manually test the functionality and user interface of the Vail Adapter Firmware Update website (`index.html`).

**Last Updated:** YYYY-MM-DD (Update this as needed)
**Tester:** ____________________
**Browser(s) Tested:** ____________________ (e.g., Chrome vXX, Firefox vYY, Edge vZZ)
**Operating System:** ____________________

---

## I. Visual Inspection and Basic UI

**Objective:** Verify the initial page load, visual appearance, and responsiveness.

| Step | Action                                                                       | Expected Result                                                                                                                                                                                             | Actual Result (Pass/Fail) & Notes |
|------|------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------|
| 1    | Open `index.html` in a modern web browser (e.g., Chrome, Firefox, Edge).     | Page loads successfully.                                                                                                                                                                                    |                                   |
| 2    | Check the browser tab/window title.                                          | Title is "Vail Adapter Firmware Update".                                                                                                                                                                    |                                   |
| 3    | Look at the main heading on the page.                                        | Main heading "Vail Adapter Firmware Update" is displayed prominently.                                                                                                                                     |                                   |
| 4    | Read the introductory paragraph below the main heading.                      | The paragraph clearly explains the purpose of the page.                                                                                                                                                     |                                   |
| 5    | Verify the presence and titles of all six step sections.                     | All six sections are displayed in order with the following headings: <br>1. "Step 1: Choose Vail Adapter version"<br>2. "Step 2: Plug in Vail Adapter"<br>3. "Step 3: Enter Boot Mode"<br>4. "Step 4: Download UF2 File"<br>5. "Step 5: Drag and Drop UF2 File"<br>6. "Step 6: Finalize Update" |                                   |
| 6    | Evaluate overall styling and readability.                                    | - Font is clear and easy to read.<br>- Color scheme is consistent and visually appealing.<br>- Content is generally centered and well-spaced within a container.<br>- Sections are visually distinct (e.g., borders, background). |                                   |
| 7    | Resize the browser window (from desktop width down to mobile width).         | - Layout adjusts to smaller screen sizes without breaking.<br>- Text remains readable and does not overflow.<br>- Buttons and radio buttons are still usable and appropriately sized.                      |                                   |

---

## II. Step 1: Choose Vail Adapter Version

**Objective:** Verify the functionality of the adapter version selection.

| Step | Action                                                                | Expected Result                                                                                                | Actual Result (Pass/Fail) & Notes |
|------|-----------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------|-----------------------------------|
| 1    | Examine the radio buttons in "Step 1".                                | Four radio button options are present with labels: "Non PCB/Basic", "Basic PCB V1", "Basic PCB V2", "Advanced PCB". |                                   |
| 2    | Click on "Non PCB/Basic".                                             | "Non PCB/Basic" is selected.                                                                                   |                                   |
| 3    | Click on "Basic PCB V1".                                              | "Basic PCB V1" is selected, and "Non PCB/Basic" becomes unselected.                                            |                                   |
| 4    | Click on "Basic PCB V2".                                              | "Basic PCB V2" is selected, and "Basic PCB V1" becomes unselected.                                             |                                   |
| 5    | Click on "Advanced PCB".                                              | "Advanced PCB" is selected, and "Basic PCB V2" becomes unselected.                                             |                                   |
| 6    | Verify label association.                                             | Clicking on a label selects the corresponding radio button.                                                    |                                   |

---

**III. Step 3: Press this button to enter boot mode (WebSerial Functionality)**

*Note: This requires a compatible SAMD21 device (Xiao SAMD21 or Adafruit QT Py SAMD21) that is NOT yet in bootloader mode. Ensure you are using a WebSerial compatible browser (e.g., Chrome, Edge).*

1.  **Prepare for Testing:**
    *   Open `index.html` in your WebSerial-compatible browser.
    *   Connect your SAMD21 device to the computer via USB.
    *   Open the browser's developer console (F12) and select the "Console" tab to monitor any background errors.
    *   Locate the "Activity Log:" area within Step 3 on the webpage. This will show specific messages from the script.

2.  **Initiate Boot Mode:**
    *   Click the "Press to Enter Boot Mode" button.
    *   Your browser should display a prompt asking you to select a serial port.
    *   Choose the serial port corresponding to your connected SAMD21 device. (The name might vary, e.g., "Adafruit QT Py SAMD21", "Arduino Zero", "USB Serial Device", etc.).
    *   Click "Connect" (or the equivalent button in the browser prompt).

3.  **Observe the Process:**
    *   **Activity Log on Webpage:** Watch the messages appearing in the "Activity Log:". You should see:
        *   "Attempting to trigger bootloader via WebSerial..."
        *   "Requesting serial port selection..."
        *   "Port selected."
        *   "Attempting 1200bps touch..."
        *   "Port opened at 1200bps."
        *   "Port closed after 1200bps touch."
        *   "SUCCESS: Bootloader mode command sent! ..."
    *   **Browser Console:** Check for any errors if the process doesn't complete as expected.
    *   **Device Behavior:** Your SAMD21 device should disconnect from the system (the serial port will disappear) and then, after a few seconds, reconnect.
    *   **New Drive:** After successful reconnection, a new storage drive should appear on your computer (e.g., "QTPYBOOT", "XIAOBOOT", "ADAPTERBOOT"). This indicates the device is in bootloader mode.

4.  **Test Error Conditions / Edge Cases:**
    *   **Cancel Port Selection:** Click the "Press to Enter Boot Mode" button, but click "Cancel" (or close) the browser's serial port selection prompt.
        *   Verify: The "Activity Log" should show an error message (e.g., "NotFoundError: User cancelled the port selection dialog." or similar). The operation should stop gracefully. An alert might also appear.
    *   **No Compatible Device Connected:** Disconnect your SAMD21 device. Click the "Press to Enter Boot Mode" button.
        *   Verify: The browser prompt might show no devices, or the script might log an error in the "Activity Log" if no port with the specified VIDs is found (e.g., "NotFoundError: No port selected by the user." or similar). An alert might also appear.
    *   **Device Already in Bootloader Mode:** If the device is already in bootloader mode (i.e., the drive is visible), this process might not work as expected or might try to connect to the bootloader serial port. Note the behavior. (Ideally, the user does this step when the device is running application code). The "Activity Log" might show messages, but the device won't reset.

**Troubleshooting "Error: WebSerial API not supported..."**
*   Ensure you are using a modern browser that supports WebSerial (e.g., Chrome version 78 or later, Edge version 79 or later). Firefox does not support WebSerial by default.

**Troubleshooting General Connection Issues:**
*   **Check Cables/Connections:** Ensure your USB cable is a data cable (not just charging) and is securely connected.
*   **Try Different USB Port:** Sometimes, specific USB ports can be problematic.
*   **Conflicting Software:** While WebSerial is generally better at sharing than WebUSB, ensure no other application (like Arduino IDE's serial monitor) is actively holding the *exact same serial port* open. The 1200bps trick often involves the OS temporarily losing the port, so conflicts are less common *during* the baud rate change itself, but initial connection can be an issue.
*   **Device Drivers:** Ensure your OS has the necessary drivers for the SAMD21's serial port (usually automatic on Windows 10/11, macOS, Linux).

---

## IV. Step 4: Download UF2 File

**Objective:** Verify the download button's state changes and file download functionality.

| Step | Action                                                                 | Expected Result                                                                                                                                                               | Actual Result (Pass/Fail) & Notes |
|------|------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------|
| 1    | Observe the "Download UF2 File" button in "Step 4" on page load.       | The button is initially disabled (e.g., greyed out, not clickable) and its text is "Download UF2 File". Clicking it might show an alert to select a version first.                |                                   |
| 2    | Go to "Step 1" and select the "Non PCB/Basic" radio button.            | "Non PCB/Basic" is selected.                                                                                                                                                  |                                   |
| 3    | Re-observe the button in "Step 4".                                     | The button becomes enabled. Its text updates to "Download non_pcb_basic.uf2".                                                                                                 |                                   |
| 4    | Click the now enabled "Download non_pcb_basic.uf2" button.             | A file download initiates. The suggested filename is `non_pcb_basic.uf2`. (Content will be placeholder text).                                                                 |                                   |
| 5    | Go to "Step 1" and select the "Basic PCB V1" radio button.             | "Basic PCB V1" is selected.                                                                                                                                                   |                                   |
| 6    | Re-observe the button in "Step 4".                                     | The button remains enabled. Its text updates to "Download basic_pcb_v1.uf2".                                                                                                   |                                   |
| 7    | Click the "Download basic_pcb_v1.uf2" button.                          | A file download initiates. The suggested filename is `basic_pcb_v1.uf2`.                                                                                                      |                                   |
| 8    | Repeat steps 5-7 for "Basic PCB V2" and "Advanced PCB".                | - Button text updates correctly for "Basic PCB V2" (to `basic_pcb_v2.uf2`).<br>- Button text updates correctly for "Advanced PCB" (to `advanced_pcb.uf2`).<br>- Downloads initiate with correct filenames. |                                   |
| 9    | (If applicable) Refresh the page or select no radio button.            | The download button should return to its disabled state and default text.                                                                                                     |                                   |

---

## V. Step 5: Drag and Drop UF2 File Instructions

**Objective:** Verify the clarity and accuracy of the UF2 file transfer instructions.

| Step | Action                                                  | Expected Result                                                                                                                                                                                             | Actual Result (Pass/Fail) & Notes |
|------|---------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------|
| 1    | Read through all instructions in "Step 5".              | - Instructions are clear, grammatically correct, and easy for a non-technical user to follow.<br>- Steps correctly describe opening a file explorer, finding the downloaded UF2, identifying the bootloader drive (e.g., "ADAPTERBOOT", "QTPYBOOT"), and the drag-and-drop action.<br>- Mentions LED activity and drive auto-ejecting. |                                   |
| 2    | Check for any ambiguities or missing information.       | All necessary information for this step is present.                                                                                                                                                         |                                   |

---

## VI. Step 6: Finalize Update Instructions

**Objective:** Verify the clarity and accuracy of the finalization steps.

| Step | Action                                                  | Expected Result                                                                                                                                                           | Actual Result (Pass/Fail) & Notes |
|------|---------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------|
| 1    | Read through all instructions in "Step 6".              | - Instructions are clear, grammatically correct, and easy to follow.<br>- Steps correctly describe waiting, unplugging the adapter, and plugging it back in.                |                                   |
| 2    | Check for any ambiguities or missing information.       | All necessary information for concluding the update process is present.                                                                                                   |                                   |

---

## VII. Console and Activity Log Checks

**Objective:** Identify any errors or warnings in the browser's developer console and the on-page Activity Log.

| Step | Action                                                                    | Expected Result                                     | Actual Result (Pass/Fail) & Notes |
|------|---------------------------------------------------------------------------|-----------------------------------------------------|-----------------------------------|
| 1    | Open the browser's developer console (F12).                               | Console opens.                                      |                                   |
| 2    | Simultaneously monitor the "Activity Log:" area within Step 3 on the webpage. | The "Activity Log:" area is visible.                |                                   |
| 3    | During all steps, but *especially during Step 3 (Enter Boot Mode)*, observe both the browser console and the webpage's Activity Log for: | - Any error messages or warnings.<br>- Successful status messages being logged as expected in both locations (though Activity Log will be more specific to WebSerial steps).<br>- Ensure the messages in the Activity Log match the actions being performed. |                                   |

---

**Overall Test Summary & Additional Notes:**

(Space for general comments, issues not covered above, or overall impression)
