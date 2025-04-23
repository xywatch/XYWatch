#ifndef BMA423_WATCH_H
#define BMA423_WATCH_H

bool bmaConfig();
void showAccelerometer();
uint16_t initBma423();
uint8_t bma423_get_direction();
float bma423_get_temp();
void resetStepCounter ();
void enableTiltWrist (bool enable);
void enableDoubleTap(bool enable);

#endif