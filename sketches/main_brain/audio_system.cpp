#include "audio_system.h"
#include "config.h"
#include <Arduino.h>

void playSessionStartSound() {
  // Cheerful ascending tone for successful login
  int melody[] = {523, 659, 784, 1047}; // C5, E5, G5, C6
  int durations[] = {200, 200, 200, 400};
  
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], durations[i]);
    delay(durations[i] + 50);
  }
  noTone(BUZZER_PIN);
}

void playAuthFailSound() {
  // Low descending tone for failed authentication
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 200, 300);
    delay(400);
  }
  noTone(BUZZER_PIN);
}

void playWorkSessionStartSound() {
  // Single confident tone
  tone(BUZZER_PIN, 880, 500); // A5
  delay(600);
  noTone(BUZZER_PIN);
}

void playBreakStartSound() {
  // Gentle double tone
  tone(BUZZER_PIN, 659, 300); // E5
  delay(400);
  tone(BUZZER_PIN, 523, 300); // C5
  delay(400);
  noTone(BUZZER_PIN);
}

void playLongBreakStartSound() {
  // Celebration melody
  int melody[] = {523, 659, 784, 659, 523}; // C5, E5, G5, E5, C5
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, melody[i], 200);
    delay(250);
  }
  noTone(BUZZER_PIN);
}

void playSessionCompleteSound() {
  // Achievement sound
  int melody[] = {784, 880, 988, 1047}; // G5, A5, B5, C6
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], 300);
    delay(350);
  }
  noTone(BUZZER_PIN);
}

void playReminderSound() {
  // Gentle reminder tone
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 440, 300); // A4
    delay(400);
  }
  noTone(BUZZER_PIN);
}

void playTouchAcknoledgmentSound() {
  // Short acknowledgment tone
  tone(BUZZER_PIN, 523, 200); // C5
  delay(250);
  noTone(BUZZER_PIN);
}