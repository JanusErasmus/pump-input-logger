#ifndef UTILS_H_
#define UTILS_H_

uint8_t calc_crc(uint8_t * buff, uint8_t len);

void digitalClockDisplay();
void digitalClockDisplay(time_t timeStamp);
void digitalClockDisplay(tmElements_t tm);

void printDigits(byte digits);


#endif /* UTILS_H_ */
