# Smart Mirror Prototype

This project is a functional Smart Mirror that combines a real-time clock (RTC), LCD display, and a simple user interface controlled via a single button. The prototype manages time display, LED brightness adjustment, and menu navigation using a state machine.

---

### [Link to demo](https://youtu.be/SZAy3_tnhQE?si=a_sdpUQwHDt8H1BS)

### Source Code Structure

The source code is structured as follows:

- **Core Libraries**:
    - `RTClib.h` – For RTC DS1307 module communication.
    - `LiquidCrystal.h` – For interfacing with the 16x2 LCD display.

- **Main Components**:
    - **Enums**:
        - `ButtonEvent`: Short and long button press handling.
        - `AppState`: Defines the primary states of the system (e.g., `STATE_MAIN`, `STATE_MENU`).
        - `TimeField`: Represents fields during time editing (e.g., hour, minute, second).
    - **Context Structures**:
        - `MenuContext`: Manages menu navigation.
        - `TimeEditingContext`: Tracks active time fields and editing status.
        - `AppContext`: Stores the current state, menu information, and editing contexts.
    - **Functions**:
        - `checkButtonEvent()`: Detects and debounces button presses.
        - `handleEvent()`: Updates the state machine based on button events.
        - `updateDisplay()`: Renders the appropriate screen to the LCD based on the current state.
        - `setup()` and `loop()`: Arduino standard functions to initialize and control the system.

---

### How to Use the Prototype

1. **Hardware Setup**:
   - Procure the following components:
     - Arduino Uno, RTC DS3231 Module, 16x2 LCD, Push Button, LEDs, and a Breadboard.
   - Wire the components using general good wiring practices.
   - Connect the push button and LED pins to digital I/O pins on the Arduino.

2. **Source Code**:
   - Open the Arduino IDE and navigate to:
     ```
     Arduino/
       ├── libraries/         # Required libraries (RTClib, LiquidCrystal)
       └── sketch_dec_11/     # Contains the source code for the Smart Mirror
     ```
   - Modify pin values in the source code if your wiring differs:
     ```cpp
     #define BTN_1_PIN 13    // Pin for the push button
     #define LED_PIN 6       // Pin for the LED
     ```

3. **Usage**:
   - Upload the source code to your Arduino.
   - **Power On**: Long press the button to turn on the system.
   - **Brightness Control**: Short press cycles through LED brightness levels.
   - **Menu Navigation**:
     - Long press opens the menu.
     - Use short presses to navigate options and long presses to select.
   - **Set Time**:
     - Navigate to "Set Time" in the menu.
     - Short press cycles through hours, minutes, and seconds.
     - Long press toggles between editing mode and saving the changes.
   - **Power Off**: Long press when prompted to turn off the system.

---

For more details, refer to the documentation or visual guides in the repository.
