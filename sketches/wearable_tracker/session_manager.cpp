#include "session_manager.h"

// Session state variables
bool sessionActive = false;
String currentUser = "";

void initializeSession() {
  // Initialize session state
  sessionActive = false;
  currentUser = "";
}