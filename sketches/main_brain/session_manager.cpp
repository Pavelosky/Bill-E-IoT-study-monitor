#include "session_manager.h"
#include "config.h"
#include "sound_effects.h"
#include "pomodoro.h"
#include "display_manager.h"

// Session state variables
bool sessionActive = false;
String currentUser = "";
unsigned long sessionStart = 0;

// External functions
void publishSessionState();
void publishPomodoroState();

void startSession(String userId) {
  sessionActive = true;
  currentUser = userId;
  sessionStart = millis();
  
  digitalWrite(LED_PIN, HIGH);
  
  // Play success sound
  playSessionStartSound();
  
  // Initialize Pomodoro timer
  initializePomodoro();
  
  // Publish session state to MQTT
  publishSessionState();
  
  Serial.println("Session started for: " + userId);
}

void endSession() {
  sessionActive = false;
  String lastUser = currentUser;
  currentUser = "";
  
  digitalWrite(LED_PIN, LOW);
  
  // Play session complete sound
  playSessionCompleteSound();
  
  // Reset Pomodoro timer
  pomodoro.currentState = IDLE;
  
  // Publish final session state
  publishSessionState();
  publishPomodoroState();
  
  showWelcomeScreen();
  
  Serial.printf("Session ended. Completed %d Pomodoro cycles.\n", pomodoro.completedCycles);
}