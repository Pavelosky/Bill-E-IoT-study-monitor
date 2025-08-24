#ifndef ENVIRONMENTAL_ANALYSIS_H
#define ENVIRONMENTAL_ANALYSIS_H

void checkEnvironmentalAlerts();
void controlFan();
void setFanManualOverride(bool enabled, bool state);
void publishFanStatus();

// Fan control variables
extern bool fanState;
extern bool manualOverride;
extern bool manualFanState;

#endif