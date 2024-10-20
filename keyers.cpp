#include <stddef.h>
#include "keyers.h"

#define len(t) (sizeof(t)/sizeof(*t))

// Queue Set: A Set you can shift and pop.
class QSet {
    int arr[MAX_KEYER_QUEUE];
    unsigned int arrlen = 0;

public:
    int shift() {
        if (arrlen == 0) {
            return -1;
        }
        int ret = arr[0];
        arrlen--;
        for (int i = 0; i < arrlen; i++) {
            arr[i] = arr[i+1];
        }
        return ret;
    }

    int pop() {
        if (arrlen == 0) {
            return -1;
        }
        int ret = arr[arrlen];
        arrlen--;
        return ret;
    }

    void add(int val) {
        if (arrlen == MAX_KEYER_QUEUE) {
            return;
        }
        for (int i = 0; i < arrlen; i++) {
            if (arr[i] == val) {
                return;
            }
        }
        arr[arrlen] = val;
        arrlen++;
    }
};


class StraightKeyer: public Keyer {
public:
    Transmitter *output;
    unsigned int ditDuration;
    bool txRelays[2];

    StraightKeyer() {
        this->Reset();
    }

    void SetOutput(Transmitter *output) {
        this->output = output;
    }

    void Reset() {
        if (this->output) {
            this->output->EndTx();
        }
        this->ditDuration = 100;
    }

    void SetDitDuration(unsigned int duration) {
        this->ditDuration = duration;
    }

    void Release() {
        this->Reset();
    }

    bool TxClosed() {
        for (int i = 0; i < len(this->txRelays); i++) {
            if (this->TxClosed(i)) {
                return true;
            }
        }
        return false;
    }

    bool TxClosed(int relay) {
        return this->txRelays[relay];
    }

    void Tx(int relay, bool closed) {
        bool wasClosed = this->TxClosed();
        this->txRelays[relay] = closed;
        bool nowClosed = this->TxClosed();

        if (wasClosed != nowClosed) {
            if (nowClosed) {
                this->output->BeginTx();
            } else {
                this->output->EndTx();
            }
        }
    }

    void Key(Paddle key, bool pressed) {
        this->Tx(key, pressed);
    }

    void Tick(unsigned int millis) {};
};

class BugKeyer: public StraightKeyer {
public:
    unsigned int nextPulse = 0;
    bool keyPressed[2];

    using StraightKeyer::StraightKeyer;

    void Reset() {
        StraightKeyer::Reset();
        this->nextPulse = 0;
        this->keyPressed[0] = false;
        this->keyPressed[1] = false;
    }

    void Key(Paddle key, bool pressed) {
        this->keyPressed[key] = pressed;
        if (key == 0) {
            this->beginPulsing();
        } else {
            StraightKeyer::Key(key, pressed);
        }
    }

    void Tick(unsigned int millis) {
        if (this->nextPulse && (millis >= this->nextPulse)) {
            this->pulse(millis);
        }
    }

    void beginPulsing() {
        if (!this->nextPulse) {
            this->nextPulse = 1;
        }
    }

    virtual void pulse(unsigned int millis) {
        if (this->TxClosed(0)) {
            this->Tx(0, false);
        } else if (this->keyPressed[0]) {
            this->Tx(0, true);
        } else {
            this->nextPulse = 0;
            return;
        }
        this->nextPulse = millis + this->ditDuration;
    }
};

class ElBugKeyer: public BugKeyer {
public:
    unsigned int nextRepeat;

    using BugKeyer::BugKeyer;

    void Reset() {
        BugKeyer::Reset();
        this->nextRepeat = -1;
    }

    // Return which key is pressed. If none, return -1.
    int whichKeyPressed() {
        for (int i = 0; i < len(this->keyPressed); i++) {
            if (this->keyPressed[i]) {
                return i;
            }
        }
        return -1;
    }

    void Key(Paddle key, bool pressed) {
        this->keyPressed[key] = pressed;
        if (pressed) {
            this->nextRepeat = key;
            this->beginPulsing();
        } else {
            this->nextRepeat = this->whichKeyPressed();
        }
    }

    unsigned int keyDuration(int key) {
        switch (key) {
        case PADDLE_DIT:
            return this->ditDuration;
        case PADDLE_DAH:
            return 3 * (this->ditDuration);
        }
        return this->ditDuration; // XXX
    }

    virtual int nextTx() {
        if (this->whichKeyPressed() == -1) {
            return -1;
        }
        return this->nextRepeat;
    }

    virtual void pulse(unsigned int millis) {
        int nextPulse = 0;
        if (this->TxClosed(0)) {
            // Pause if we're currently transmitting
            nextPulse = this->keyDuration(PADDLE_DIT);
            this->Tx(0, false);
        } else {
            int next = this->nextTx();
            if (next >= 0) {
                nextPulse = this->keyDuration(next);
                this->Tx(0, true);
            }
        }

        if (nextPulse) {
            this->nextPulse = millis + nextPulse;
        } else {
            this->nextPulse = 0;
        }
    }
};

class UltimaticKeyer: public ElBugKeyer {
public:
    QSet queue;
    
    using ElBugKeyer::ElBugKeyer;

    void Key(Paddle key, bool pressed) {
        if (pressed) {
            this->queue.add(key);
        }
        ElBugKeyer::Key(key, pressed);
    }

    virtual int nextTx() {
        int key = this->queue.shift();
        if (key != -1) {
            return key;
        }
        return ElBugKeyer::nextTx();
    }
};

class SingleDotKeyer: public ElBugKeyer {
public:
    QSet queue;

    using ElBugKeyer::ElBugKeyer;
    
    void Key(Paddle key, bool pressed) {
        if (pressed && (key == PADDLE_DIT)) {
            this->queue.add(key);
        }
        ElBugKeyer::Key(key, pressed);
    }

    virtual int nextTx() {
        int key = this->queue.shift();
        if (key != -1) {
            return key;
        }
        if (this->keyPressed[1]) return 1;
        if (this->keyPressed[0]) return 0;
        return -1;
    }
};

class IambicKeyer: public ElBugKeyer {
public:

    using ElBugKeyer::ElBugKeyer;

    virtual int nextTx() {
        int next = ElBugKeyer::nextTx();
        if (this->keyPressed[PADDLE_DIT] && this->keyPressed[PADDLE_DAH]) {
            this->nextRepeat = 1 - this->nextRepeat;
        }
        return next;
    }
};

class IambicAKeyer: public IambicKeyer {
public:
    QSet queue;

    using IambicKeyer::IambicKeyer;
    
    void Key(Paddle key, bool pressed) {
        if (pressed && (key == PADDLE_DIT)) {
            this->queue.add(key);
        }
        IambicKeyer::Key(key, pressed);
    }

    virtual int nextTx() {
        int next = IambicKeyer::nextTx();
        int key = this->queue.shift();
        if (key != -1) {
            return key;
        }
        return next;
    }
};

class IambicBKeyer: public IambicKeyer {
public:
    QSet queue;

    using IambicKeyer::IambicKeyer;

    void Reset() {
        IambicKeyer::Reset();
    }

    void Key(Paddle key, bool pressed) {
        if (pressed) {
            this->queue.add(key);
        }
        IambicKeyer::Key(key, pressed);
    }

    virtual int nextTx() {
        for (int key = 0; key < len(this->keyPressed); key++) {
            if (this->keyPressed[key]) {
                this->queue.add(key);
            }
        }

        return this->queue.shift();
    }
};

class KeyaheadKeyer: public ElBugKeyer {
public:
    int queue[MAX_KEYER_QUEUE];
    unsigned int qlen;

    using ElBugKeyer::ElBugKeyer;

    void Reset() {
        ElBugKeyer::Reset();
        this->qlen = 0;
    }

    void Key(Paddle key, bool pressed) {
        if (pressed) {
            if (this->qlen < MAX_KEYER_QUEUE) {
                this->queue[this->qlen++] = key;
            }
        }
        ElBugKeyer::Key(key, pressed);
    }

    virtual int nextTx() {
        if (this->qlen > 0) {
            int next = this->queue[0];
            this->qlen--;
            for (int i = 0; i < this->qlen; i++) {
                this->queue[i] = this->queue[i+1];
            }
            return next;
        }
        return ElBugKeyer::nextTx();
    }
};

StraightKeyer straightKeyer = StraightKeyer();
BugKeyer bugKeyer = BugKeyer();
ElBugKeyer elBugKeyer = ElBugKeyer();
SingleDotKeyer singleDotKeyer = SingleDotKeyer();
UltimaticKeyer ultimaticKeyer = UltimaticKeyer();
IambicKeyer iambicKeyer = IambicKeyer();
IambicAKeyer iambicAKeyer = IambicAKeyer();
IambicBKeyer iambicBKeyer = IambicBKeyer();
KeyaheadKeyer keyaheadKeyer = KeyaheadKeyer();

Keyer *keyers[] = {
    NULL,
    &straightKeyer,
    &bugKeyer,
    &elBugKeyer,
    &singleDotKeyer,
    &ultimaticKeyer,
    &iambicKeyer,
    &iambicAKeyer,
    &iambicBKeyer,
    &keyaheadKeyer,
};

Keyer *GetKeyerByNumber(int n, Transmitter *output) {
    if (n >= len(keyers)) {
        return NULL;
    }

    Keyer *k = keyers[n];
    k->SetOutput(output);
    return k;
}
