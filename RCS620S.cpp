/*
 * RC-S620/S sample library for Arduino
 *
 * Copyright 2010 Sony Corporation
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"
#include "Print.h"
#include "HardwareSerial.h"

#include "RCS620S.h"

/* --------------------------------
 * Constant
 * -------------------------------- */

#define RCS620S_DEFAULT_TIMEOUT 1000

/* --------------------------------
 * Variable
 * -------------------------------- */

/* --------------------------------
 * Prototype Declaration
 * -------------------------------- */

/* --------------------------------
 * Macro
 * -------------------------------- */

/* --------------------------------
 * Function
 * -------------------------------- */

/* ------------------------
 * public
 * ------------------------ */

RCS620S::RCS620S(Stream &serial)
{
    _serial = &serial;
    this->timeout = RCS620S_DEFAULT_TIMEOUT;
}

void RCS620S::wakeUp(void)
{
    writeSerial((const uint8_t *)"\x55", 1);
#if defined __avr__
    delayMicroseconds(200);

#elif defined ESP_H
    ets_delay_us(200);
#endif
    cancel();
};

int RCS620S::getFirmwareVersion(uint8_t response[RCS620S_MAX_CARD_RESPONSE_LEN], uint16_t *responseLen)
{
    int ret;

    cancel();

    ret = rwCommand((const uint8_t *)"\xd4\x02", 2, response, responseLen);
    if (!ret || (*responseLen != 6) ||
        (memcmp(response, "\xd5\x03", 2) != 0))
    {
        return 0;
    }
    return 1;
}

int RCS620S::reset(void)
{
    int ret;
    uint8_t response[RCS620S_MAX_RW_RESPONSE_LEN];
    uint16_t responseLen;

    cancel();

    /* Reset */
    ret = rwCommand((const uint8_t *)"\xd4\x18\x01", 3,
                    response, &responseLen);
    if (!ret || (responseLen != 2) ||
        (memcmp(response, "\xd5\x19", 2) != 0))
    {
        return 0;
    }

    delay(10);
    return 1;
}

int RCS620S::initDevice(void)
{
    int ret;
    uint8_t response[RCS620S_MAX_RW_RESPONSE_LEN];
    uint16_t responseLen;

    /* RFConfiguration (various timings) */
    ret = rwCommand((const uint8_t *)"\xd4\x32\x02\x00\x00\x00", 6,
                    response, &responseLen);
    if (!ret || (responseLen != 2) ||
        (memcmp(response, "\xd5\x33", 2) != 0))
    {
        return 0;
    }

    /* RFConfiguration (max retries) */
    ret = rwCommand((const uint8_t *)"\xd4\x32\x05\x00\x00\x00", 6,
                    response, &responseLen);
    if (!ret || (responseLen != 2) ||
        (memcmp(response, "\xd5\x33", 2) != 0))
    {
        return 0;
    }

    /* RFConfiguration (additional wait time = 24ms) */
    ret = rwCommand((const uint8_t *)"\xd4\x32\x81\xb7", 4,
                    response, &responseLen);
    if (!ret || (responseLen != 2) ||
        (memcmp(response, "\xd5\x33", 2) != 0))
    {
        return 0;
    }

    /* SetParameters (18h) */
    ret = rwCommand((const uint8_t *)"\xd4\x12\x18", 3,
                    response, &responseLen);
    if (!ret || (responseLen != 2) ||
        (memcmp(response, "\xd5\x13", 2) != 0))
    {
        return 0;
    }

    /* 公式おまじない */
    ret = rwCommand((const uint8_t *)"\xd4\x08\x63\x0d\x00", 5,
                    response, &responseLen);
    if (!ret || (responseLen != 3) ||
        (memcmp(response, "\xd5\x09\x00", 3) != 0))
    {
        return 0;
    }

    return 1;
}

int RCS620S::tgInitTarget(const uint8_t *idm, const uint8_t *pmm, const uint8_t *rfu, uint8_t response[RCS620S_MAX_CARD_RESPONSE_LEN], uint16_t *responseLen)
{
    uint8_t command[RCS620S_MAX_RW_RESPONSE_LEN] = {0x00};
    uint8_t start[2] = {0xd4, 0x8c}; // command code & sub command code
    /*
        DEP Only は設定してはいけない -> 設定するとProprietary Command (Felica系？)が返ってこない

    */
    uint8_t activated[1] = {0x01};                               // Activated limit
    uint8_t params106[6] = {0x00, 0x04, 0x00, 0x00, 0x00, 0x40}; // 106kbpsParams(6byte)

    memcpy(&command[0], start, 2);
    memcpy(&command[2], activated, 1);
    memcpy(&command[3], params106, 6);
    memcpy(&command[9], idm, 8);  // NFCID2t(IDm)
    memcpy(&command[17], pmm, 8); // PAD(PMm)
    memcpy(&command[25], rfu, 2); // RFU(System Code)
    memcpy(&command[27], idm, 8); // NFCID3t(IDm)

    int ret = rwCommand(command, 37, response, responseLen);
    if (!ret)
    {
        return 0;
    }
    else if (memcmp(response, "\xd5\x8d", 2) != 0)
    {
        return 0;
    }

    return 1;
}

int RCS620S::tgResponseToInitiator(const uint8_t tgResponse[RCS620S_MAX_CARD_RESPONSE_LEN], const uint16_t tgResponseLen, uint8_t response[RCS620S_MAX_CARD_RESPONSE_LEN], uint16_t *responseLen)
{
    uint8_t command[RCS620S_MAX_RW_RESPONSE_LEN] = {0x00};
    uint8_t start[2] = {0xd4, 0x90}; // command code & sub command code

    memcpy(&command[0], start, 2);
    memcpy(&command[2], tgResponse, tgResponseLen);

    int ret = rwCommand(command, 2 + tgResponseLen, response, responseLen);
    if (!ret)
    {
        return 0;
    }
    else if (memcmp(response, "\xd5\x91", 2) != 0)
    {
        return 0;
    }
    return 1;
}

int RCS620S::tgGetInitiatorCommand(uint8_t response[RCS620S_MAX_CARD_RESPONSE_LEN], uint16_t *responseLen)
{
    uint8_t command[2] = {0xd4, 0x88}; // command code & sub command code

    int ret = rwCommand(command, 2, response, responseLen);
    if (!ret)
    {
        return 0;
    }
    else if (memcmp(response, "\xd5\x91", 2) != 0)
    {
        return 0;
    }
    return 1;
}

int RCS620S::polling(uint16_t systemCode)
{
    int ret;
    uint8_t buf[9];
    uint8_t response[RCS620S_MAX_RW_RESPONSE_LEN];
    uint16_t responseLen;

    /* InListPassiveTarget */
    memcpy(buf, "\xd4\x4a\x01\x01\x00\xff\xff\x00\x00", 9);
    buf[5] = (uint8_t)((systemCode >> 8) & 0xff);
    buf[6] = (uint8_t)((systemCode >> 0) & 0xff);

    ret = rwCommand(buf, 9, response, &responseLen);
    if (!ret || (responseLen != 22) ||
        (memcmp(response, "\xd5\x4b\x01\x01\x12\x01", 6) != 0))
    {
        return 0;
    }

    memcpy(this->idm, response + 6, 8);
    memcpy(this->pmm, response + 14, 8);

    return 1;
}

int RCS620S::cardCommand(
    const uint8_t *command,
    uint8_t commandLen,
    uint8_t response[RCS620S_MAX_CARD_RESPONSE_LEN],
    uint8_t *responseLen)
{
    int ret;
    uint16_t commandTimeout;
    uint8_t buf[RCS620S_MAX_RW_RESPONSE_LEN];
    uint16_t len;

    if (this->timeout >= (0x10000 / 2))
    {
        commandTimeout = 0xffff;
    }
    else
    {
        commandTimeout = (uint16_t)(this->timeout * 2);
    }

    /* CommunicateThruEX */
    buf[0] = 0xd4;
    buf[1] = 0xa0;
    buf[2] = (uint8_t)((commandTimeout >> 0) & 0xff);
    buf[3] = (uint8_t)((commandTimeout >> 8) & 0xff);
    buf[4] = (uint8_t)(commandLen + 1);
    memcpy(buf + 5, command, commandLen);

    ret = rwCommand(buf, 5 + commandLen, buf, &len);
    if (!ret || (len < 4) ||
        (buf[0] != 0xd5) || (buf[1] != 0xa1) || (buf[2] != 0x00) ||
        (len != (3 + buf[3])))
    {
        return 0;
    }

    *responseLen = (uint8_t)(buf[3] - 1);
    memcpy(response, buf + 4, *responseLen);

    return 1;
}

int RCS620S::rfOff(void)
{
    int ret;
    uint8_t response[RCS620S_MAX_RW_RESPONSE_LEN];
    uint16_t responseLen;

    /* RFConfiguration (RF field) */
    ret = rwCommand((const uint8_t *)"\xd4\x32\x01\x00", 4,
                    response, &responseLen);
    if (!ret || (responseLen != 2) ||
        (memcmp(response, "\xd5\x33", 2) != 0))
    {
        return 0;
    }

    return 1;
}

int RCS620S::push(
    const uint8_t *data,
    uint8_t dataLen)
{
    int ret;
    uint8_t buf[RCS620S_MAX_CARD_RESPONSE_LEN];
    uint8_t responseLen;

    if (dataLen > 224)
    {
        return 0;
    }

    /* Push */
    buf[0] = 0xb0;
    memcpy(buf + 1, this->idm, 8);
    buf[9] = dataLen;
    memcpy(buf + 10, data, dataLen);

    ret = cardCommand(buf, 10 + dataLen, buf, &responseLen);
    if (!ret || (responseLen != 10) || (buf[0] != 0xb1) ||
        (memcmp(buf + 1, this->idm, 8) != 0) || (buf[9] != dataLen))
    {
        return 0;
    }

    buf[0] = 0xa4;
    memcpy(buf + 1, this->idm, 8);
    buf[9] = 0x00;

    ret = cardCommand(buf, 10, buf, &responseLen);
    if (!ret || (responseLen != 10) || (buf[0] != 0xa5) ||
        (memcmp(buf + 1, this->idm, 8) != 0) || (buf[9] != 0x00))
    {
        return 0;
    }

    delay(1000);

    return 1;
}

/* ------------------------
 * private
 * ------------------------ */

int RCS620S::rwCommand(
    const uint8_t *command,
    uint16_t commandLen,
    uint8_t response[RCS620S_MAX_RW_RESPONSE_LEN],
    uint16_t *responseLen)
{
    int ret;
    uint8_t buf[9];

    flushSerial();

    uint8_t dcs = calcDCS(command, commandLen);

    /* transmit the command */
    buf[0] = 0x00;
    buf[1] = 0x00;
    buf[2] = 0xff;
    if (commandLen <= 255)
    {
        /* normal frame */
        buf[3] = commandLen;
        buf[4] = (uint8_t)-buf[3];
        writeSerial(buf, 5);
    }
    else
    {
        /* extended frame */
        buf[3] = 0xff;
        buf[4] = 0xff;
        buf[5] = (uint8_t)((commandLen >> 8) & 0xff);
        buf[6] = (uint8_t)((commandLen >> 0) & 0xff);
        buf[7] = (uint8_t) - (buf[5] + buf[6]);
        writeSerial(buf, 8);
    }
    writeSerial(command, commandLen);
    buf[0] = dcs;
    buf[1] = 0x00;
    writeSerial(buf, 2);

    /* receive an ACK */
    ret = readSerial(buf, 6);
    if (!ret || (memcmp(buf, "\x00\x00\xff\x00\xff\x00", 6) != 0))
    {
        cancel();
        return 0;
    }

    /* receive a response */
    ret = readSerial(buf, 5);
    if (!ret)
    {
        cancel();
        return 0;
    }
    else if (memcmp(buf, "\x00\x00\xff", 3) != 0)
    {
        return 0;
    }
    if ((buf[3] == 0xff) && (buf[4] == 0xff))
    {
        ret = readSerial(buf + 5, 3);
        if (!ret || (((buf[5] + buf[6] + buf[7]) & 0xff) != 0))
        {
            return 0;
        }
        *responseLen = (((uint16_t)buf[5] << 8) |
                        ((uint16_t)buf[6] << 0));
    }
    else
    {
        if (((buf[3] + buf[4]) & 0xff) != 0)
        {
            return 0;
        }
        *responseLen = buf[3];
    }
    if (*responseLen > RCS620S_MAX_RW_RESPONSE_LEN)
    {
        return 0;
    }

    ret = readSerial(response, *responseLen);
    if (!ret)
    {
        cancel();
        return 0;
    }

    dcs = calcDCS(response, *responseLen);

    ret = readSerial(buf, 2);
    if (!ret || (buf[0] != dcs) || (buf[1] != 0x00))
    {
        cancel();
        return 0;
    }

    return 1;
}

void RCS620S::cancel(void)
{
    /* transmit an ACK */
    writeSerial((const uint8_t *)"\x00\x00\xff\x00\xff\x00", 6);
    delay(1);
    flushSerial();
}

uint8_t RCS620S::calcDCS(
    const uint8_t *data,
    uint16_t len)
{
    uint8_t sum = 0;

    for (uint16_t i = 0; i < len; i++)
    {
        sum += data[i];
    }

    return (uint8_t) - (sum & 0xff);
}

void RCS620S::writeSerial(
    const uint8_t *data,
    uint16_t len)
{
    _serial->write(data, len);
}

int RCS620S::readSerial(
    uint8_t *data,
    uint16_t len)
{
    uint16_t nread = 0;
    unsigned long t0 = millis();

    while (nread < len)
    {
        if (checkTimeout(t0))
        {
            return 0;
        }

        if (_serial->available() > 0)
        {
            data[nread] = _serial->read();
            nread++;
        }
    }

    return 1;
}

void RCS620S::flushSerial(void)
{
    _serial->flush();
}

int RCS620S::checkTimeout(unsigned long t0)
{
    unsigned long t = millis();

    if ((t - t0) >= this->timeout)
    {
        return 1;
    }

    return 0;
}
