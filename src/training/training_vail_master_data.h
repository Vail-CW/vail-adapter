/*
 * Vail Master - Contest Data
 * Contains callsigns, names, states, and sections for contest exchange generation
 * Data stored in PROGMEM to save RAM
 */

#ifndef TRAINING_VAIL_MASTER_DATA_H
#define TRAINING_VAIL_MASTER_DATA_H

#include <Arduino.h>

// ============================================
// Sprint Contest Data
// ============================================

// Common amateur radio callsigns (realistic mix of US and DX)
const char* const VM_CALLSIGNS[] PROGMEM = {
    "W1AW", "K3LR", "N6RO", "W5XD", "K1ZZ", "N2IC", "W6YX", "K9CT",
    "N4BP", "W0AIH", "K7RL", "N5DX", "W4PA", "K6XT", "N1MM", "W3LPL",
    "K4XS", "N7DD", "W8PR", "K5ZD", "N0AX", "W2PV", "K8ND", "N3RS",
    "W9RE", "K1DG", "N4AF", "W6NV", "K2LE", "N8II", "W7RN", "K3WW",
    "N6TV", "W4MYA", "K5TR", "N1UR", "W0UO", "K6LA", "N4XD", "W3GH",
    "K7GM", "N5RZ", "W1RM", "K4AB", "N3ZL", "W8JI", "K2NV", "N7KT",
    "W5WMU", "K9RS", "N4PN", "W6RJ", "K1AR", "N3RS", "W4AN", "K8PO",
    "N7OU", "W2SC", "K5DJ", "N6MJ", "W9OP", "K3NA", "N4RJ", "W7EW",
    "K1VR", "N5TJ", "W0GJ", "K4ZW", "N8BJQ", "W6YI", "K2SX", "N3QE",
    "W3BGN", "K7QQ", "N4ZR", "W1MK", "K6RIM", "N5AW", "W4NF", "K9NW",
    "N2NT", "W7WA", "K3TC", "N6AA", "W5ASP", "K8GL", "N1LN", "W4PA",
    "K6VVA", "N7MH", "W9ILY", "K4JAF", "N3RR", "W6PH", "K5LZO", "N8PR"
};
const int VM_CALLSIGN_COUNT = 96;

// Common operator first names
const char* const VM_NAMES[] PROGMEM = {
    "JOHN", "BOB", "MIKE", "JIM", "TOM", "BILL", "DAVE", "STEVE",
    "RICK", "DAN", "MARK", "PAUL", "GARY", "SCOTT", "CHRIS", "JEFF",
    "BRIAN", "RON", "JOE", "ED", "AL", "KEN", "DON", "JACK",
    "PETE", "TONY", "FRANK", "RAY", "GREG", "LARRY", "JERRY", "DOUG",
    "TIM", "ROB", "PHIL", "SAM", "ANDY", "ART", "CARL", "WALT",
    "ROGER", "DEAN", "WAYNE", "STAN", "LEN", "FRED", "NICK", "VIC",
    "LEE", "MATT", "JAY", "BRUCE", "NEIL", "GENE", "RALPH", "ALAN"
};
const int VM_NAME_COUNT = 56;

// US state abbreviations
const char* const VM_STATES[] PROGMEM = {
    "AL", "AK", "AZ", "AR", "CA", "CO", "CT", "DE", "FL", "GA",
    "HI", "ID", "IL", "IN", "IA", "KS", "KY", "LA", "ME", "MD",
    "MA", "MI", "MN", "MS", "MO", "MT", "NE", "NV", "NH", "NJ",
    "NM", "NY", "NC", "ND", "OH", "OK", "OR", "PA", "RI", "SC",
    "SD", "TN", "TX", "UT", "VT", "VA", "WA", "WV", "WI", "WY"
};
const int VM_STATE_COUNT = 50;

// ============================================
// Sweepstakes Contest Data
// ============================================

// Precedence letters
const char VM_PRECEDENCE[] PROGMEM = {'Q', 'A', 'B', 'U', 'M', 'S'};
const int VM_PRECEDENCE_COUNT = 6;

// ARRL/RAC Sections (all 83 sections)
const char* const VM_SECTIONS[] PROGMEM = {
    // New England
    "CT", "EMA", "ME", "NH", "RI", "VT", "WMA",
    // New York/New Jersey
    "ENY", "NLI", "NNJ", "SNJ", "WNY",
    // Mid-Atlantic
    "DE", "EPA", "MDC", "WPA",
    // Southeast
    "AL", "GA", "KY", "NC", "NFL", "SC", "SFL", "WCF", "TN", "VA", "PR", "VI",
    // Great Lakes
    "MI", "OH", "WV",
    // Midwest
    "IL", "IN", "WI",
    // Delta
    "AR", "LA", "MS", "NM", "NTX", "OK", "STX", "WTX",
    // Rocky Mountain
    "CO", "KS", "MN", "MO", "NE", "ND", "SD", "UT", "WY",
    // Southwest
    "AZ", "EB", "LA", "ORG", "SB", "SCV", "SDG", "SF", "SJV", "SV",
    // Pacific
    "AK", "EWA", "ID", "MT", "NV", "OR", "PAC", "WWA",
    // Canada
    "AB", "BC", "MB", "NB", "NL", "NS", "ON", "PE", "QC", "SK"
};
const int VM_SECTION_COUNT = 80;

// ============================================
// Default Character Sets
// ============================================

const char VM_CHARSET_LETTERS[] PROGMEM = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char VM_CHARSET_NUMBERS[] PROGMEM = "0123456789";
const char VM_CHARSET_PUNCTUATION[] PROGMEM = ".,?/=";
const char VM_CHARSET_PROSIGNS[] PROGMEM = "+=";  // AR, BT represented
const char VM_CHARSET_ALL[] PROGMEM = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/=";

// Default custom charset (letters only)
const char VM_CHARSET_DEFAULT[] PROGMEM = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// ============================================
// Helper Functions for PROGMEM Access
// ============================================

// Get a random callsign from the pool
inline String vmGetRandomCallsign() {
    int idx = random(VM_CALLSIGN_COUNT);
    return String(VM_CALLSIGNS[idx]);
}

// Get a random name from the pool
inline String vmGetRandomName() {
    int idx = random(VM_NAME_COUNT);
    return String(VM_NAMES[idx]);
}

// Get a random state from the pool
inline String vmGetRandomState() {
    int idx = random(VM_STATE_COUNT);
    return String(VM_STATES[idx]);
}

// Get a random precedence character
inline char vmGetRandomPrecedence() {
    int idx = random(VM_PRECEDENCE_COUNT);
    return pgm_read_byte(&VM_PRECEDENCE[idx]);
}

// Get a random section from the pool
inline String vmGetRandomSection() {
    int idx = random(VM_SECTION_COUNT);
    return String(VM_SECTIONS[idx]);
}

// Get a character from a charset string stored in PROGMEM
inline char vmGetCharFromCharset(const char* charset, int idx) {
    return pgm_read_byte(&charset[idx]);
}

// Get charset length
inline int vmGetCharsetLength(const char* charset) {
    int len = 0;
    while (pgm_read_byte(&charset[len]) != 0) len++;
    return len;
}

#endif // TRAINING_VAIL_MASTER_DATA_H
