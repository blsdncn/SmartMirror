#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>

enum ButtonEvent {
  EVENT_NONE,
  EVENT_SHORT_PRESS,
  EVENT_LONG_PRESS
};

enum AppState {
  STATE_OFF,
  STATE_MAIN,
  STATE_MENU,
  STATE_EDIT_TIME,
  STATE_OFF_CONFIRM
};

enum TimeField {
  FIELD_HOUR,
  FIELD_MINUTE,
  FIELD_SECOND,
  FIELD_BACK
};

struct MenuOption {
  const char* label;               // Menu text
  AppState nextStateOnShortPress;  // State on short press (unused)
  AppState nextStateOnLongPress;   // State on long press
};

struct MenuContext {
  const MenuOption* options;       // Pointer to menu options
  uint8_t numOptions;              // Number of menu items
  uint8_t currentIndex;            // Currently selected option
};

struct TimeEditingContext {
  TimeField currentField;          // Which time field is active
  bool editingActive;              // Are we editing the field?
  DateTime frozenTime;             // Captured time during editing
};

struct AppContext {
  AppState currentState;           // Current state of the application
  MenuContext menu;                // Menu state
  TimeEditingContext timeEdit;     // Time editing state
  int brightnessIndex;
};


#define BTN_1_PIN 13
#define LED_PIN 6
RTC_DS1307 rtc;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); // Updated pin configuration

const uint8_t brightnessList[] = {0, 50, 100, 254};
const uint8_t numBrightnessSettings = 4;
char line[16];


static const MenuOption mainMenuOptions[] = {
  {"Set Time",    STATE_EDIT_TIME, STATE_EDIT_TIME},
  {"Back",        STATE_MAIN,      STATE_MAIN},
  {"Turn Off",    STATE_OFF_CONFIRM, STATE_OFF_CONFIRM}
};

AppContext context = {
  STATE_MAIN,                       // Initial state
  {mainMenuOptions, 3, 0},          // Menu with 3 options
  {FIELD_HOUR, false},              // Default time editing context
  0
};

ButtonEvent checkButtonEvent() {
  static bool buttonReady = true;
  static unsigned long pressStartTime = 0;
  static bool longPressTriggered = false;
  static unsigned long lastDebounceTime = 0;  // For debouncing
  const unsigned long longPressThreshold = 1000;
  const unsigned long debounceDelay = 25;     // Debounce delay in milliseconds
  
  static int lastButtonState = LOW;
  int buttonState = !digitalRead(BTN_1_PIN);   // Current button state
  
  // Debouncing: Check if button state changed and stabilize it
  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Stable button state
    if (buttonState == HIGH && buttonReady) {
      // Button just pressed
      buttonReady = false;
      pressStartTime = millis();
      longPressTriggered = false;
    }

    if (buttonState == HIGH && !longPressTriggered) {
      // Check long press
      if (millis() - pressStartTime >= longPressThreshold) {
        longPressTriggered = true;
        lastButtonState = buttonState;  // Update last state
        return EVENT_LONG_PRESS;
      }
    }

    if (buttonState == LOW && !buttonReady) {
      // Button just released
      buttonReady = true;
      if (!longPressTriggered) {
        lastButtonState = buttonState;  // Update last state
        return EVENT_SHORT_PRESS;
      }
    }
  }

  lastButtonState = buttonState;  // Save the current state
  return EVENT_NONE;
}

void handleEvent(ButtonEvent evnt){
  switch (context.currentState){
    case STATE_OFF:
      if(evnt == EVENT_LONG_PRESS){
        context.currentState = STATE_MAIN;
        lcd.display();
        context.menu.currentIndex = 0;
      } 
      // nothing for other events--it's off.
      break;

    case STATE_MAIN:
      if(evnt == EVENT_LONG_PRESS){
        context.currentState = STATE_MENU;
        context.menu.currentIndex = 0;
      } else if (evnt == EVENT_SHORT_PRESS){
        context.brightnessIndex = (context.brightnessIndex+1)%4;
        analogWrite(LED_PIN,brightnessList[context.brightnessIndex]);
      }
      break;

    case STATE_MENU:
      if(evnt == EVENT_LONG_PRESS){
        context.currentState = context.menu.options[context.menu.currentIndex].nextStateOnLongPress;
      } else if (evnt == EVENT_SHORT_PRESS){
        context.menu.currentIndex = (context.menu.currentIndex + 1) % context.menu.numOptions;
      }
      break;

    case STATE_EDIT_TIME:
      if(evnt == EVENT_LONG_PRESS){
        if(context.timeEdit.currentField == FIELD_BACK){
          context.timeEdit.currentField = FIELD_HOUR;
          context.currentState = STATE_MENU;
        } else {
          context.timeEdit.editingActive = !context.timeEdit.editingActive;
          if(context.timeEdit.editingActive){
            context.timeEdit.frozenTime = rtc.now();
          } else {
            rtc.adjust(context.timeEdit.frozenTime);
          }
        }
      } else if (evnt == EVENT_SHORT_PRESS){
        if (context.timeEdit.editingActive) {
          // Increment the currently selected field
          switch (context.timeEdit.currentField) {
            case FIELD_HOUR:
              context.timeEdit.frozenTime = DateTime(
              context.timeEdit.frozenTime.year(),
              context.timeEdit.frozenTime.month(),
              context.timeEdit.frozenTime.day(),
              (context.timeEdit.frozenTime.hour() + 1) % 24, // Wrap around 24 hours
              context.timeEdit.frozenTime.minute(),
              context.timeEdit.frozenTime.second()
              );
              break;

            case FIELD_MINUTE:
              context.timeEdit.frozenTime = DateTime(
              context.timeEdit.frozenTime.year(),
              context.timeEdit.frozenTime.month(),
              context.timeEdit.frozenTime.day(),
              context.timeEdit.frozenTime.hour(),
              (context.timeEdit.frozenTime.minute() + 1) % 60, // Wrap around 60 minutes
              context.timeEdit.frozenTime.second()
              );
              break;

            case FIELD_SECOND:
              context.timeEdit.frozenTime = DateTime(
              context.timeEdit.frozenTime.year(),
              context.timeEdit.frozenTime.month(),
              context.timeEdit.frozenTime.day(),
              context.timeEdit.frozenTime.hour(),
              context.timeEdit.frozenTime.minute(),
              (context.timeEdit.frozenTime.second() + 1) % 60 // Wrap around 60 seconds
              );
              break;

            default:
              break;
          }      
        } else {
          // Cycle through fields if not editing
          context.timeEdit.currentField = static_cast<TimeField>((context.timeEdit.currentField + 1) % 4);
        } 
      }
      break;

    case STATE_OFF_CONFIRM:
      if(evnt == EVENT_LONG_PRESS){
        context.currentState = STATE_OFF;
        context.brightnessIndex = 0;
        analogWrite(LED_PIN,brightnessList[context.brightnessIndex]);
        lcd.noDisplay();
      } else if (evnt == EVENT_SHORT_PRESS){
        context.currentState = STATE_MENU;
      }
  } 
}

void updateDisplay() {
  DateTime now = rtc.now();
  //Serial.println(context.currentState);

  switch((AppState)context.currentState) {
    case STATE_OFF:
      lcd.noDisplay();
      break;

    case STATE_MAIN:
      lcd.setCursor(0, 0);
      lcd.print("Main Screen    ");  // Extra spaces to overwrite old text
      lcd.setCursor(0, 1);
      sprintf(line,"%02d:%02d:%02d        ",now.hour(),now.minute(),now.second());
      lcd.print(line);
      break;

    case STATE_MENU:
      lcd.setCursor(0, 0);
      lcd.print("Menu:           ");
      lcd.setCursor(0, 1);
      sprintf(line,"%-16s",context.menu.options[context.menu.currentIndex].label);
      lcd.print(line);
      break;

    case STATE_EDIT_TIME:
      lcd.setCursor(0, 0);
      lcd.print("Edit Time:      ");
      lcd.setCursor(0, 1);
      
      // Display the current field or its value
      if (!context.timeEdit.editingActive) {
        // Not editing, show field names
        switch (context.timeEdit.currentField) {
          case FIELD_HOUR:   sprintf(line, "Hour            "); break;
          case FIELD_MINUTE: sprintf(line, "Minute          "); break;
          case FIELD_SECOND: sprintf(line, "Second          "); break;
          case FIELD_BACK:   sprintf(line, "Back            "); break;
        }
      } else {
        // Editing, show the current value of the frozen time
        switch (context.timeEdit.currentField) {
          case FIELD_HOUR:   
            sprintf(line, "Hour: %02d        ", context.timeEdit.frozenTime.hour()); 
            break;
          case FIELD_MINUTE: 
            sprintf(line, "Minute: %02d      ", context.timeEdit.frozenTime.minute()); 
            break;
          case FIELD_SECOND: 
            sprintf(line, "Second: %02d      ", context.timeEdit.frozenTime.second()); 
            break;
          case FIELD_BACK:   
            sprintf(line, "Back            "); 
            break;
        }
      }
      lcd.print(line);
      break;

    case STATE_OFF_CONFIRM:
      lcd.setCursor(0, 0);
      lcd.print("Turn Off?       ");
      lcd.setCursor(0, 1);
      lcd.print("Long Press: Yes ");
      break;

    default:
      lcd.setCursor(0, 0);
      lcd.print("Invalid State:  ");
      lcd.setCursor(0, 1);
      sprintf(line,"%-16s",context.currentState);
      lcd.print(line);
      break;
  }
  
}


void setup() {
  Serial.begin(9600);
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Can't find RTC  ");
    while (1);
  }

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.print("RTC & LED Test  ");

  // Set LED pin and button pin
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_1_PIN, INPUT_PULLUP);

}

void loop() {
  ButtonEvent evnt = checkButtonEvent(); // Get current button event

  if (evnt != EVENT_NONE) {
    //Serial.print(evnt);
    //Serial.print(" | ");
    handleEvent(evnt);          // Update the state machine
  }

  updateDisplay();              // Render the current screen
  delay(50);                    // Small delay to reduce flicker
  //Serial.println(context.currentState);
}


