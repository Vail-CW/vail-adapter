#pragma once

#define MAX_KEYER_QUEUE 5

typedef enum {
    PADDLE_DIT = 0,
    PADDLE_DAH = 1,
    PADDLE_STRAIGHT,
} Paddle;

class Transmitter {
public:
    virtual void BeginTx();
    virtual void EndTx();
    virtual void BeginTx(int relay);
    virtual void EndTx(int relay);
};

class Keyer {
public:
    virtual void SetOutput(Transmitter *output);
    virtual void Reset();
    virtual void SetDitDuration(unsigned int d);
    virtual void Release();
    virtual bool TxClosed();
    virtual bool TxClosed(int relay);
    virtual void Tx(int relay, bool closed);
    virtual void Key(Paddle key, bool pressed);
    virtual void Tick(unsigned int millis);
};

Keyer *GetKeyerByNumber(int n, Transmitter *output);
int getKeyerNumber(Keyer* k);