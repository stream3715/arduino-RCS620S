/*
 * RC-S620/S sample library for Arduino
 *
 * Copyright 2010 Sony Corporation
 */

#ifndef RCS620S_H_
#define RCS620S_H_

#include <Arduino.h>

/* --------------------------------
 * Constant
 * -------------------------------- */

#define RCS620S_MAX_CARD_RESPONSE_LEN 254
#define RCS620S_MAX_RW_RESPONSE_LEN 265

/* --------------------------------
 * Class Declaration
 * -------------------------------- */

class RCS620S
{
public:
#if defined __AVR__
    RCS620S(Stream &serial = Serial);
#elif defined ESP32
    RCS620S(Stream &serial = Serial1);
#endif

    void wakeUp(void);
    int getFirmwareVersion(uint8_t response[RCS620S_MAX_CARD_RESPONSE_LEN], uint16_t *responseLen);
    int reset(void);
    int initDevice(void);
    int tgInitTarget(const uint8_t *idm, const uint8_t *pmm, const uint8_t *rfu, uint8_t response[RCS620S_MAX_CARD_RESPONSE_LEN], uint16_t *responseLen);
    int tgResponseToInitiator(const uint8_t tgResponse[RCS620S_MAX_CARD_RESPONSE_LEN], const uint16_t tgResponseLen, uint8_t response[RCS620S_MAX_CARD_RESPONSE_LEN], uint16_t *responseLen);
    int tgGetInitiatorCommand(uint8_t response[RCS620S_MAX_CARD_RESPONSE_LEN], uint16_t *responseLen);

    int polling(uint16_t systemCode = 0xffff);
    int cardCommand(
        const uint8_t *command,
        uint8_t commandLen,
        uint8_t response[RCS620S_MAX_CARD_RESPONSE_LEN],
        uint8_t *responseLen);
    int rfOff(void);

    int push(
        const uint8_t *data,
        uint8_t dataLen);

private:
    Stream *_serial;
    int rwCommand(
        const uint8_t *command,
        uint16_t commandLen,
        uint8_t response[RCS620S_MAX_RW_RESPONSE_LEN],
        uint16_t *responseLen);
    void cancel(void);
    uint8_t calcDCS(
        const uint8_t *data,
        uint16_t len);

    void writeSerial(
        const uint8_t *data,
        uint16_t len);
    int readSerial(
        uint8_t *data,
        uint16_t len);
    void flushSerial(void);

    int checkTimeout(unsigned long t0);

public:
    unsigned long timeout;
    uint8_t idm[8];
    uint8_t pmm[8];
};

#endif /* !RCS620S_H_ */
