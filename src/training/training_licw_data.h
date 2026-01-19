/*
 * LICW Training Data
 *
 * Curriculum data for the Long Island CW Club (LICW) training implementation.
 * Contains character progressions, word lists, and phrases for each lesson.
 *
 * LICW Curriculum Structure:
 * - BC1: 18 characters in 6 lessons (12/8 WPM)
 * - BC2: 26 characters in 9 lessons (12/10 WPM)
 * - BC3: 5 on-air prep lessons (12/12 WPM)
 * - INT1-INT3: Intermediate flow and speed building
 * - ADV1-ADV3: Advanced high-speed mastery
 *
 * IMPORTANT: NO morse pattern visuals (.-) - characters are SOUNDS
 *
 * Copyright (c) 2025 VAIL SUMMIT Contributors
 */

#ifndef TRAINING_LICW_DATA_H
#define TRAINING_LICW_DATA_H

#include "training_licw_core.h"

// ============================================
// BC1 LESSON DATA - Basic CW 1
// 18 characters in 6 lessons at 12/8 WPM
// ============================================

// BC1 Lesson 1: R, E, A
static const char* bc1_lesson1_words[] = {
    "ARE", "EAR", "ERA", "ERR", "REA",
    nullptr
};

static const char* bc1_lesson1_phrases[] = {
    "A RE", "E RE", "A A A", "R E A",
    nullptr
};

// BC1 Lesson 2: T, I, N
static const char* bc1_lesson2_words[] = {
    "IN", "IT", "AN", "AT", "TEN", "TIN", "TAN", "ANT", "ATE", "EAT",
    "NET", "NIT", "RAN", "RAT", "TAR", "TEA", "TIE", "IRE", "AIR",
    nullptr
};

static const char* bc1_lesson2_phrases[] = {
    "TIN EAR", "EAT IT", "AT TEN", "IN AIR", "AN ANT",
    nullptr
};

// BC1 Lesson 3: P, S, G
static const char* bc1_lesson3_words[] = {
    "PIG", "PAN", "PAT", "PEA", "PEN", "PET", "PIN", "PIT", "SAT", "SET",
    "SIN", "SIP", "SIT", "GAP", "GAS", "GET", "GIG", "GIN", "NAP", "NAG",
    "GAG", "PEG", "RAG", "RIG", "SAG", "TAG", "AGE", "APE", "APT", "ASP",
    "EGG", "GAL", "SEA", "SPA", "SPY", "TAP", "TIP", "GAP",
    nullptr
};

static const char* bc1_lesson3_phrases[] = {
    "GET SET", "PIG PEN", "SIT IN", "TAP IT", "GAS TANK",
    nullptr
};

// BC1 Lesson 4: L, C, D
static const char* bc1_lesson4_words[] = {
    "LAD", "LAP", "LAT", "LED", "LET", "LID", "LIP", "LIT", "CAD", "CAN",
    "CAP", "CAT", "CIT", "DAD", "DAN", "DIG", "DIN", "DIP", "ACE", "ACT",
    "ADD", "AID", "ALE", "ALL", "AND", "ARC", "CALL", "CARD", "CENT", "CLAD",
    "CLAN", "CLAP", "CLIP", "DISC", "LACE", "LAND", "LATE", "LEAD", "LEAN",
    "CELL", "DIAL", "NICE", "RICE", "TALL", "TELL",
    nullptr
};

static const char* bc1_lesson4_phrases[] = {
    "TALL CALL", "NICE LEAD", "DIG IN", "CELL CAD", "ACE IT",
    nullptr
};

// BC1 Lesson 5: H, O, F
static const char* bc1_lesson5_words[] = {
    "HAD", "HAM", "HAS", "HAT", "HEN", "HID", "HIM", "HIP", "HIS", "HIT",
    "HOG", "HOP", "HOT", "OFF", "OFT", "FOG", "FOR", "FOE", "FAN", "FAR",
    "FAT", "FED", "FIG", "FIN", "FIT", "HALF", "HAND", "HILL", "HOLD",
    "HOLE", "HOME", "HOPE", "HOST", "FOND", "FOOT", "FORM", "FORT", "FROG",
    "CHEF", "FISH", "HORN", "SOFT", "GOLF", "SHED",
    nullptr
};

static const char* bc1_lesson5_phrases[] = {
    "HOT DOG", "FAR OFF", "GO HOME", "HIT HARD", "HALF SHOT",
    nullptr
};

// BC1 Lesson 6: U, W, B
static const char* bc1_lesson6_words[] = {
    "BUD", "BUG", "BUN", "BUS", "BUT", "BUY", "CUB", "CUD", "CUP", "CUT",
    "DUB", "DUD", "DUE", "DUG", "FUN", "GUM", "GUN", "GUT", "HUB", "HUG",
    "HUM", "HUT", "JUG", "MUD", "MUG", "NUN", "NUT", "PUB", "PUN", "PUP",
    "PUT", "RUB", "RUG", "RUN", "RUT", "SUB", "SUM", "SUN", "TUB", "TUG",
    "WAS", "WAR", "WAX", "WAY", "WEB", "WED", "WET", "WHO", "WHY", "WIG",
    "WIN", "WIT", "WOE", "WOK", "WON", "BOW", "BOX", "BOY",
    nullptr
};

static const char* bc1_lesson6_phrases[] = {
    "BIG BUG", "WET RUG", "BUY GUM", "SUN UP", "WIN BIG",
    nullptr
};

// BC1 Lessons Array
static const LICWLesson bc1Lessons[] = {
    {1, 12, 8, "REA", "REA", "First three letters: R, E, A", bc1_lesson1_words, bc1_lesson1_phrases},
    {2, 12, 8, "TIN", "REATIN", "Common letters: T, I, N", bc1_lesson2_words, bc1_lesson2_phrases},
    {3, 12, 8, "PSG", "REATINPSG", "More letters: P, S, G", bc1_lesson3_words, bc1_lesson3_phrases},
    {4, 12, 8, "LCD", "REATINPSGLCD", "Building words: L, C, D", bc1_lesson4_words, bc1_lesson4_phrases},
    {5, 12, 8, "HOF", "REATINPSGLCDHOF", "Expanding: H, O, F", bc1_lesson5_words, bc1_lesson5_phrases},
    {6, 12, 8, "UWB", "REATINPSGLCDHOFUWB", "Completing BC1: U, W, B", bc1_lesson6_words, bc1_lesson6_phrases}
};

// ============================================
// BC2 LESSON DATA - Basic CW 2
// 26 more characters in 9 lessons at 12/10 WPM
// ============================================

// BC2 Lesson 1: K, M, Y
static const char* bc2_lesson1_words[] = {
    "KEY", "KID", "KIN", "KIT", "KICK", "KILL", "KIND", "KING", "KITE",
    "MAD", "MAN", "MAP", "MAT", "MAY", "MEN", "MET", "MID", "MIX", "MOB",
    "MOM", "MOP", "MUD", "YAK", "YAM", "YAP", "YEA", "YES", "YET",
    "MAKE", "MARK", "MASK", "MANY", "OKAY", "YELL", "YOUR",
    nullptr
};

static const char* bc2_lesson1_phrases[] = {
    "MY KEY", "MAKE IT", "YES MAN", "KIND MARK", "OKAY YET",
    nullptr
};

// BC2 Lesson 2: 5, 9, COMMA
static const char* bc2_lesson2_words[] = {
    "5", "9", "55", "59", "95", "99", "555", "599", "559", "959",
    nullptr
};

static const char* bc2_lesson2_phrases[] = {
    "RST 599", "RST 559", "RST 579", "5 BY 9", "5 AND 9",
    nullptr
};

// BC2 Lesson 3: Q, X, V
static const char* bc2_lesson3_words[] = {
    "QSO", "QTH", "QRM", "QRN", "QSB", "QSL", "QRZ", "QRS", "QRQ",
    "VAT", "VAN", "VIA", "VET", "VIE", "VOW", "VAIL", "VAST", "VERY",
    "VENT", "VIEW", "VINE", "VOTE", "AXE", "BOX", "FAX", "FIX", "FOX",
    "HEX", "MIX", "SIX", "TAX", "WAX", "APEX", "EXAM", "EXIT", "NEXT",
    nullptr
};

static const char* bc2_lesson3_phrases[] = {
    "QSO VIA", "QTH NEXT", "FIX IT", "VERY VAST", "SIX BOX",
    nullptr
};

// BC2 Lesson 4: 7, 3, QUESTION MARK
static const char* bc2_lesson4_words[] = {
    "3", "7", "33", "37", "73", "77", "333", "373", "737", "773",
    nullptr
};

static const char* bc2_lesson4_phrases[] = {
    "73 DE", "73 AND 88", "AGE 37", "QTH?", "NAME?", "RST?",
    nullptr
};

// BC2 Lesson 5: AR, SK, BT (Prosigns)
static const char* bc2_lesson5_words[] = {
    "AR", "SK", "BT",
    nullptr
};

static const char* bc2_lesson5_phrases[] = {
    "CQ CQ DE W1ABC AR", "TU 73 SK", "NAME BT JOHN BT",
    "QTH BT NEW YORK BT", "RST 599 BT 599 AR",
    nullptr
};

// BC2 Lesson 6: 1, 6, PERIOD
static const char* bc2_lesson6_words[] = {
    "1", "6", "11", "16", "61", "66", "116", "161", "611", "616",
    nullptr
};

static const char* bc2_lesson6_phrases[] = {
    "RST 519", "FREQ 14.061", "TIME 1630", "AGE 61", "QTH 16 ELM ST.",
    nullptr
};

// BC2 Lesson 7: Z, J, SLASH
static const char* bc2_lesson7_words[] = {
    "JAB", "JAM", "JAR", "JAW", "JAY", "JET", "JIG", "JOB", "JOG", "JOT",
    "JOY", "JUG", "ZAP", "ZED", "ZEN", "ZIP", "ZIT", "ZOO", "JAZZ", "JOKE",
    "JUST", "ZONE", "ZERO", "FIZZ", "FUZZ", "JAZZ", "QUIZ",
    nullptr
};

static const char* bc2_lesson7_phrases[] = {
    "W1ABC/M", "N2XYZ/P", "QSL VIA W1AW/QRP", "JUST JOKE", "ZERO IN",
    nullptr
};

// BC2 Lesson 8: 2, 8, BK (Prosign)
static const char* bc2_lesson8_words[] = {
    "2", "8", "22", "28", "82", "88", "228", "282", "822", "828",
    "BK",
    nullptr
};

static const char* bc2_lesson8_phrases[] = {
    "AGE 28", "RST 589", "88 TO YL", "QTH 82 MAIN", "UR 589 BK",
    nullptr
};

// BC2 Lesson 9: 4, 0 (zero)
static const char* bc2_lesson9_words[] = {
    "0", "4", "00", "04", "40", "44", "400", "404", "440", "044",
    "10", "20", "30", "40", "50", "60", "70", "80", "90", "100",
    nullptr
};

static const char* bc2_lesson9_phrases[] = {
    "RST 599 100W", "QTH 40 OAK", "TIME 1400", "FREQ 7.040", "AGE 40",
    nullptr
};

// BC2 Lessons Array
static const LICWLesson bc2Lessons[] = {
    {1, 12, 10, "KMY", "REATINPSGLCDHOFUWBKMY", "Adding K, M, Y", bc2_lesson1_words, bc2_lesson1_phrases},
    {2, 12, 10, "59,", "REATINPSGLCDHOFUWBKMY59,", "Numbers and comma: 5, 9, ,", bc2_lesson2_words, bc2_lesson2_phrases},
    {3, 12, 10, "QXV", "REATINPSGLCDHOFUWBKMY59,QXV", "Q-codes and more: Q, X, V", bc2_lesson3_words, bc2_lesson3_phrases},
    {4, 12, 10, "73?", "REATINPSGLCDHOFUWBKMY59,QXV73?", "73 and question: 7, 3, ?", bc2_lesson4_words, bc2_lesson4_phrases},
    {5, 12, 10, "AR SK BT", "REATINPSGLCDHOFUWBKMY59,QXV73?", "Prosigns: AR, SK, BT", bc2_lesson5_words, bc2_lesson5_phrases},
    {6, 12, 10, "16.", "REATINPSGLCDHOFUWBKMY59,QXV73?16.", "More numbers: 1, 6, .", bc2_lesson6_words, bc2_lesson6_phrases},
    {7, 12, 10, "ZJ/", "REATINPSGLCDHOFUWBKMY59,QXV73?16.ZJ/", "Final letters: Z, J, /", bc2_lesson7_words, bc2_lesson7_phrases},
    {8, 12, 10, "28 BK", "REATINPSGLCDHOFUWBKMY59,QXV73?16.ZJ/28", "Numbers and BK: 2, 8, BK", bc2_lesson8_words, bc2_lesson8_phrases},
    {9, 12, 10, "40", "REATINPSGLCDHOFUWBKMY59,QXV73?16.ZJ/2840", "Final numbers: 4, 0", bc2_lesson9_words, bc2_lesson9_phrases}
};

// ============================================
// BC3 LESSON DATA - On-Air Prep
// 5 QSO-focused lessons at 12/12 WPM
// ============================================

// BC3 Lesson 1: Standard QSO Protocol
static const char* bc3_lesson1_words[] = {
    "CQ", "DE", "K", "AR", "SK", "BT", "73", "88", "TU", "RST",
    "QTH", "NAME", "RIG", "ANT", "PWR", "WX", "HR", "UR", "ES", "FB",
    nullptr
};

static const char* bc3_lesson1_phrases[] = {
    "CQ CQ CQ DE W1ABC W1ABC K",
    "W1ABC DE N2XYZ N2XYZ K",
    "GM OM UR RST 599 599 BT NAME HR JOHN JOHN BT QTH BOSTON MA BOSTON MA BT HW CPY? AR W1ABC DE N2XYZ K",
    "R R GM JOHN TU FER CALL BT UR RST 599 599 BT NAME HR BOB BOB BT QTH NEW YORK NY NEW YORK BT AR N2XYZ DE W1ABC K",
    "R FB BOB TU FER RPT BT RIG HR IC7300 ES ANT DIPOLE BT PWR 100W BT WX HR SUNNY 72F BT AR W1ABC DE N2XYZ K",
    "R R TU JOHN FB QSO BT 73 ES GUD DX BT W1ABC DE N2XYZ SK",
    "TU BOB 73 ES HPE CUAGN BT N2XYZ DE W1ABC SK",
    nullptr
};

// BC3 Lesson 2: LICW Challenge Exchange
static const char* bc3_lesson2_words[] = {
    "LICW", "NR", "CHK", "SEC", "PREC", "CALL", "EXCH",
    nullptr
};

static const char* bc3_lesson2_phrases[] = {
    "CQ LICW CQ LICW DE W1ABC K",
    "W1ABC DE N2XYZ LICW NR 123 CHK 45 SEC NLI K",
    "TU NR 456 CHK 32 SEC SNJ K",
    "R R TU 73 W1ABC DE N2XYZ",
    nullptr
};

// BC3 Lesson 3: SKCC Exchange
static const char* bc3_lesson3_words[] = {
    "SKCC", "NR", "SKS", "WES", "BRAG",
    nullptr
};

static const char* bc3_lesson3_phrases[] = {
    "CQ SKCC CQ SKCC DE W1ABC K",
    "W1ABC DE N2XYZ N2XYZ SKCC NR 12345 K",
    "GM UR RST 599 599 BT SKCC NR 54321 K",
    "R TU 73 SK",
    nullptr
};

// BC3 Lesson 4: POTA/SOTA Exchange
static const char* bc3_lesson4_words[] = {
    "POTA", "SOTA", "PARK", "SUMMIT", "SPOTTER", "SPOT", "P2P", "S2S",
    nullptr
};

static const char* bc3_lesson4_phrases[] = {
    "CQ POTA CQ POTA DE W1ABC K",
    "W1ABC DE N2XYZ N2XYZ K",
    "GM UR 599 MA BT K-1234 BT TU 73 AR W1ABC K",
    "R R TU 599 NY BT 73 SK",
    "CQ SOTA CQ SOTA DE W1ABC/P K",
    "W1ABC DE N2XYZ UR 559 CA BT W6/NC-001 BT TU 73 K",
    nullptr
};

// BC3 Lesson 5: K1USN SST (Slow Speed Test)
static const char* bc3_lesson5_words[] = {
    "SST", "K1USN", "TEST", "EXCH",
    nullptr
};

static const char* bc3_lesson5_phrases[] = {
    "CQ SST CQ SST DE W1ABC K",
    "W1ABC DE N2XYZ",
    "N2XYZ TU BOB MA K",
    "R R TU JOHN NY 73 W1ABC DE N2XYZ",
    "TU 73 DE W1ABC SST",
    nullptr
};

// BC3 Lessons Array
static const LICWLesson bc3Lessons[] = {
    {1, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Standard QSO Protocol", bc3_lesson1_words, bc3_lesson1_phrases},
    {2, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "LICW Challenge Exchange", bc3_lesson2_words, bc3_lesson2_phrases},
    {3, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "SKCC Exchange", bc3_lesson3_words, bc3_lesson3_phrases},
    {4, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "POTA/SOTA Exchange", bc3_lesson4_words, bc3_lesson4_phrases},
    {5, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "K1USN SST Contest", bc3_lesson5_words, bc3_lesson5_phrases}
};

// ============================================
// INT1 LESSON DATA - Flow Skills Development
// 10 lessons at 12/12 WPM (loose focus)
// ============================================

// INT1 focuses on flow skills with known characters
static const char* int1_common_words[] = {
    "THE", "AND", "FOR", "ARE", "BUT", "NOT", "YOU", "ALL", "CAN", "HER",
    "WAS", "ONE", "OUR", "OUT", "DAY", "HAD", "HOT", "HIS", "HOW", "ITS",
    "LET", "MAY", "OLD", "SEE", "NOW", "WAY", "WHO", "BOY", "DID", "GET",
    "HAS", "HIM", "NEW", "OWN", "SAY", "SHE", "TOO", "USE", "MAN", "ANY",
    "CALL", "COPY", "DOWN", "FIND", "GIVE", "GOOD", "HAND", "HERE", "HIGH",
    "HOME", "JUST", "KNOW", "LAST", "LEFT", "LIFE", "LONG", "LOOK", "MADE",
    "MAKE", "MORE", "MOST", "MUCH", "MUST", "NAME", "NEXT", "ONLY", "OVER",
    "PART", "SOME", "SUCH", "TAKE", "TELL", "THAN", "THAT", "THEM", "THEN",
    "THIS", "TIME", "VERY", "WANT", "WELL", "WENT", "WHAT", "WHEN", "WILL",
    "WITH", "WORD", "WORK", "YEAR", "YOUR",
    nullptr
};

static const char* int1_phrases[] = {
    "THE QUICK FOX", "ALL GOOD THINGS", "COPY THAT WELL",
    "VERY GOOD SIGNAL", "YOUR NAME HERE", "NEXT TIME WILL",
    nullptr
};

// INT1 Lessons (all using same word set, progressively longer sessions)
static const LICWLesson int1Lessons[] = {
    {1, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Flow Introduction", int1_common_words, int1_phrases},
    {2, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Loose Focus Training", int1_common_words, int1_phrases},
    {3, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Character Flow 1", int1_common_words, int1_phrases},
    {4, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Character Flow 2", int1_common_words, int1_phrases},
    {5, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Word Building", int1_common_words, int1_phrases},
    {6, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Phrase Practice", int1_common_words, int1_phrases},
    {7, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "IFR Training 1", int1_common_words, int1_phrases},
    {8, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "IFR Training 2", int1_common_words, int1_phrases},
    {9, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Mixed Practice", int1_common_words, int1_phrases},
    {10, 12, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Flow Assessment", int1_common_words, int1_phrases}
};

// ============================================
// INT2 LESSON DATA - Increasing Effective Speed
// 10 lessons progressing from 12/12 to 16/14 WPM
// ============================================

static const char* int2_phrases[] = {
    "QUICK BROWN FOX JUMPS OVER THE LAZY DOG",
    "NOW IS THE TIME FOR ALL GOOD MEN",
    "PACK MY BOX WITH FIVE DOZEN LIQUOR JUGS",
    "THE FIVE BOXING WIZARDS JUMP QUICKLY",
    nullptr
};

static const LICWLesson int2Lessons[] = {
    {1, 14, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Speed Increase 1", int1_common_words, int2_phrases},
    {2, 14, 12, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Speed Increase 2", int1_common_words, int2_phrases},
    {3, 14, 13, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "TTR Focus 1", int1_common_words, int2_phrases},
    {4, 14, 13, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "TTR Focus 2", int1_common_words, int2_phrases},
    {5, 15, 13, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "IFR Advanced 1", int1_common_words, int2_phrases},
    {6, 15, 14, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "IFR Advanced 2", int1_common_words, int2_phrases},
    {7, 16, 14, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "CFP Training 1", int1_common_words, int2_phrases},
    {8, 16, 14, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "CFP Training 2", int1_common_words, int2_phrases},
    {9, 16, 14, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Speed Assessment", int1_common_words, int2_phrases},
    {10, 16, 14, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Level Complete", int1_common_words, int2_phrases}
};

// ============================================
// INT3 LESSON DATA - Word Discovery
// 10 lessons at 18-20 WPM focusing on word recognition
// ============================================

// Common ham radio words for word discovery
static const char* int3_ham_words[] = {
    "ANTENNA", "AMATEUR", "BEACON", "CALLING", "CONTEST", "DIPOLE",
    "EFFECTIVE", "FREQUENCY", "GROUND", "HORIZONTAL", "INVERTED",
    "KEYING", "LICENSE", "MOBILE", "NOVICE", "OPERATOR", "PORTABLE",
    "QUARTER", "RECEIVER", "SIGNAL", "TRANSMIT", "VERTICAL", "WATTS",
    "EXTRA", "YAGI", "ZERO", "AMPLIFIER", "BANDWIDTH", "CARRIER",
    nullptr
};

static const char* int3_phrases[] = {
    "GOOD SIGNAL INTO NEW YORK", "RUNNING ONE HUNDRED WATTS",
    "ANTENNA IS A DIPOLE", "CALLING CQ CONTEST",
    nullptr
};

static const LICWLesson int3Lessons[] = {
    {1, 18, 16, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Word Discovery Intro", int3_ham_words, int3_phrases},
    {2, 18, 16, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Common Words 1", int3_ham_words, int3_phrases},
    {3, 18, 17, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Common Words 2", int3_ham_words, int3_phrases},
    {4, 19, 17, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Ham Vocabulary 1", int3_ham_words, int3_phrases},
    {5, 19, 18, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Ham Vocabulary 2", int3_ham_words, int3_phrases},
    {6, 20, 18, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Phrase Recognition", int3_ham_words, int3_phrases},
    {7, 20, 18, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Word Anticipation", int3_ham_words, int3_phrases},
    {8, 20, 19, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Instant Recognition", int3_ham_words, int3_phrases},
    {9, 20, 19, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Mixed Practice", int3_ham_words, int3_phrases},
    {10, 20, 20, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Level Assessment", int3_ham_words, int3_phrases}
};

// ============================================
// ADV1 LESSON DATA - Conversational Head Copy
// 10 lessons at 20-25 WPM
// ============================================

static const char* adv1_phrases[] = {
    "THE WEATHER HERE IS VERY NICE TODAY WITH CLEAR SKIES",
    "MY ANTENNA IS AN INVERTED VEE UP ABOUT FORTY FEET",
    "THANKS FOR THE NICE QSO AND HOPE TO WORK YOU AGAIN",
    "RUNNING ABOUT ONE HUNDRED WATTS TO A DIPOLE ANTENNA",
    nullptr
};

static const LICWLesson adv1Lessons[] = {
    {1, 20, 20, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Conversational Intro", int3_ham_words, adv1_phrases},
    {2, 21, 20, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Head Copy Basics", int3_ham_words, adv1_phrases},
    {3, 22, 21, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Extended QSO", int3_ham_words, adv1_phrases},
    {4, 22, 21, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Ragchew Practice", int3_ham_words, adv1_phrases},
    {5, 23, 22, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "News Copy", int3_ham_words, adv1_phrases},
    {6, 23, 22, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Story Copy", int3_ham_words, adv1_phrases},
    {7, 24, 23, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Speed Building 1", int3_ham_words, adv1_phrases},
    {8, 24, 23, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Speed Building 2", int3_ham_words, adv1_phrases},
    {9, 25, 24, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Speed Assessment", int3_ham_words, adv1_phrases},
    {10, 25, 25, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Level Complete", int3_ham_words, adv1_phrases}
};

// ============================================
// ADV2 LESSON DATA - QRQ Fluency
// 10 lessons at 25-35 WPM
// ============================================

static const LICWLesson adv2Lessons[] = {
    {1, 25, 25, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "QRQ Introduction", int3_ham_words, adv1_phrases},
    {2, 27, 26, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Speed Building", int3_ham_words, adv1_phrases},
    {3, 28, 27, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Contest Style 1", int3_ham_words, adv1_phrases},
    {4, 29, 28, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Contest Style 2", int3_ham_words, adv1_phrases},
    {5, 30, 29, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Fast QSO 1", int3_ham_words, adv1_phrases},
    {6, 31, 30, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Fast QSO 2", int3_ham_words, adv1_phrases},
    {7, 32, 31, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "High Speed Copy", int3_ham_words, adv1_phrases},
    {8, 33, 32, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Adverse Conditions", int3_ham_words, adv1_phrases},
    {9, 34, 33, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Speed Assessment", int3_ham_words, adv1_phrases},
    {10, 35, 35, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Level Complete", int3_ham_words, adv1_phrases}
};

// ============================================
// ADV3 LESSON DATA - QRQ Mastery
// 10 lessons at 35-45+ WPM
// ============================================

static const LICWLesson adv3Lessons[] = {
    {1, 35, 35, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "High Speed Intro", int3_ham_words, adv1_phrases},
    {2, 36, 36, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Speed Building", int3_ham_words, adv1_phrases},
    {3, 38, 37, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Contest QRQ", int3_ham_words, adv1_phrases},
    {4, 39, 38, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "DX Pileup", int3_ham_words, adv1_phrases},
    {5, 40, 39, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Mastery Level 1", int3_ham_words, adv1_phrases},
    {6, 41, 40, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Mastery Level 2", int3_ham_words, adv1_phrases},
    {7, 42, 41, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Expert Copy", int3_ham_words, adv1_phrases},
    {8, 43, 42, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Elite Training", int3_ham_words, adv1_phrases},
    {9, 44, 43, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "Speed Assessment", int3_ham_words, adv1_phrases},
    {10, 45, 45, nullptr, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?/", "QRQ Master", int3_ham_words, adv1_phrases}
};

// ============================================
// CAROUSEL DEFINITIONS
// ============================================

static const LICWCarouselDef licwCarousels[] = {
    {
        LICW_BC1,
        "BC1: Basic CW 1",
        "BC1",
        "18 characters in 6 lessons at 12/8 WPM",
        6,
        12,
        8,
        8,
        bc1Lessons
    },
    {
        LICW_BC2,
        "BC2: Basic CW 2",
        "BC2",
        "26 more characters in 9 lessons at 12/10 WPM",
        9,
        12,
        10,
        10,
        bc2Lessons
    },
    {
        LICW_BC3,
        "BC3: On-Air Prep",
        "BC3",
        "QSO protocol in 5 lessons at 12/12 WPM",
        5,
        12,
        12,
        12,
        bc3Lessons
    },
    {
        LICW_INT1,
        "INT1: Flow Skills",
        "INT1",
        "Flow development in 10 lessons at 12 WPM",
        10,
        12,
        12,
        12,
        int1Lessons
    },
    {
        LICW_INT2,
        "INT2: Speed Building",
        "INT2",
        "Speed increase in 10 lessons to 16 WPM",
        10,
        16,
        12,
        14,
        int2Lessons
    },
    {
        LICW_INT3,
        "INT3: Word Discovery",
        "INT3",
        "Word recognition in 10 lessons to 20 WPM",
        10,
        20,
        16,
        20,
        int3Lessons
    },
    {
        LICW_ADV1,
        "ADV1: Conversational",
        "ADV1",
        "Head copy in 10 lessons at 20-25 WPM",
        10,
        25,
        20,
        25,
        adv1Lessons
    },
    {
        LICW_ADV2,
        "ADV2: QRQ Fluency",
        "ADV2",
        "High speed in 10 lessons at 25-35 WPM",
        10,
        35,
        25,
        35,
        adv2Lessons
    },
    {
        LICW_ADV3,
        "ADV3: QRQ Mastery",
        "ADV3",
        "Mastery in 10 lessons at 35-45+ WPM",
        10,
        45,
        35,
        45,
        adv3Lessons
    }
};

// ============================================
// ACCESS FUNCTIONS
// ============================================

// Get carousel definition by ID
const LICWCarouselDef* getLICWCarousel(LICWCarousel carousel) {
    if (carousel < 0 || carousel >= LICW_TOTAL_CAROUSELS) {
        return &licwCarousels[0];
    }
    return &licwCarousels[carousel];
}

// Get lesson definition
const LICWLesson* getLICWLesson(LICWCarousel carousel, int lessonNum) {
    const LICWCarouselDef* car = getLICWCarousel(carousel);
    if (lessonNum < 1 || lessonNum > car->totalLessons) {
        return &car->lessons[0];
    }
    return &car->lessons[lessonNum - 1];  // 1-based to 0-based
}

// Get cumulative characters for a lesson
const char* getLICWCumulativeChars(LICWCarousel carousel, int lessonNum) {
    const LICWLesson* lesson = getLICWLesson(carousel, lessonNum);
    return lesson->cumulativeChars;
}

// Get new characters for a lesson
const char* getLICWNewChars(LICWCarousel carousel, int lessonNum) {
    const LICWLesson* lesson = getLICWLesson(carousel, lessonNum);
    return lesson->newChars;
}

// Get word list for a lesson
const char** getLICWWords(LICWCarousel carousel, int lessonNum) {
    const LICWLesson* lesson = getLICWLesson(carousel, lessonNum);
    return lesson->words;
}

// Get phrase list for a lesson
const char** getLICWPhrases(LICWCarousel carousel, int lessonNum) {
    const LICWLesson* lesson = getLICWLesson(carousel, lessonNum);
    return lesson->phrases;
}

// Get character WPM for a lesson
int getLICWLessonCharWPM(LICWCarousel carousel, int lessonNum) {
    const LICWLesson* lesson = getLICWLesson(carousel, lessonNum);
    return lesson->characterWPM;
}

// Get effective (Farnsworth) WPM for a lesson
int getLICWLessonEffectiveWPM(LICWCarousel carousel, int lessonNum) {
    const LICWLesson* lesson = getLICWLesson(carousel, lessonNum);
    return lesson->effectiveWPM;
}

// Count words in a word list
int countLICWWords(const char** words) {
    if (words == nullptr) return 0;
    int count = 0;
    while (words[count] != nullptr) {
        count++;
    }
    return count;
}

// Get random word from lesson word list
const char* getLICWRandomWord(LICWCarousel carousel, int lessonNum) {
    const char** words = getLICWWords(carousel, lessonNum);
    int count = countLICWWords(words);
    if (count == 0) return "CQ";
    return words[random(count)];
}

// Get random phrase from lesson phrase list
const char* getLICWRandomPhrase(LICWCarousel carousel, int lessonNum) {
    const char** phrases = getLICWPhrases(carousel, lessonNum);
    if (phrases == nullptr) return "CQ CQ CQ";
    int count = 0;
    while (phrases[count] != nullptr) count++;
    if (count == 0) return "CQ CQ CQ";
    return phrases[random(count)];
}

// Get random character from cumulative set
char getLICWRandomChar(LICWCarousel carousel, int lessonNum) {
    const char* chars = getLICWCumulativeChars(carousel, lessonNum);
    int len = strlen(chars);
    if (len == 0) return 'E';
    return chars[random(len)];
}

// Generate random character group (2-5 chars from cumulative set)
void getLICWRandomGroup(LICWCarousel carousel, int lessonNum, char* buf, int maxLen) {
    const char* chars = getLICWCumulativeChars(carousel, lessonNum);
    int len = strlen(chars);
    if (len == 0 || maxLen < 3) {
        strcpy(buf, "CQ");
        return;
    }

    int groupLen = random(2, 6);  // 2 to 5 characters
    if (groupLen >= maxLen) groupLen = maxLen - 1;

    for (int i = 0; i < groupLen; i++) {
        buf[i] = chars[random(len)];
    }
    buf[groupLen] = '\0';
}

// ============================================
// QSO EXCHANGE TEMPLATES
// ============================================

// QSO exchange template structure
struct LICWQSOTemplate {
    const char* name;
    const char* description;
    const char** exchanges;  // Null-terminated array of exchange strings
};

// Standard QSO exchange patterns
static const char* standardQSOExchanges[] = {
    "CQ CQ CQ DE %CALL% %CALL% K",
    "%CALL% DE %MYCALL% %MYCALL% K",
    "GM OM UR RST %RST% %RST% BT NAME HR %NAME% %NAME% BT QTH %QTH% %QTH% BT HW CPY? AR %CALL% DE %MYCALL% K",
    "R R GM %NAME% TU FER CALL BT UR RST %RST% %RST% BT NAME HR %MYNAME% %MYNAME% BT QTH %MYQTH% %MYQTH% AR %MYCALL% DE %CALL% K",
    "R TU %NAME% FB QSO BT 73 ES GUD DX BT %CALL% DE %MYCALL% SK",
    "TU %MYNAME% 73 ES HPE CUAGN BT %MYCALL% DE %CALL% SK",
    nullptr
};

// POTA exchange patterns
static const char* potaExchanges[] = {
    "CQ POTA CQ POTA DE %CALL% %CALL% K",
    "%CALL% DE %MYCALL% %MYCALL% K",
    "GM UR %RST% %STATE% BT %PARK% BT TU 73 AR %CALL% K",
    "R R TU %RST% %MYSTATE% BT 73 SK",
    nullptr
};

// Contest exchange patterns
static const char* contestExchanges[] = {
    "CQ TEST CQ TEST DE %CALL% K",
    "%CALL% %RST% %EXCH%",
    "TU %CALL% DE %MYCALL% K",
    nullptr
};

static const LICWQSOTemplate licwQSOTemplates[] = {
    {"Standard QSO", "Full ragchew exchange", standardQSOExchanges},
    {"POTA", "Parks On The Air", potaExchanges},
    {"Contest", "Contest exchange", contestExchanges}
};

const int LICW_QSO_TEMPLATE_COUNT = sizeof(licwQSOTemplates) / sizeof(LICWQSOTemplate);

// Get QSO template by index
const LICWQSOTemplate* getLICWQSOTemplate(int index) {
    if (index < 0 || index >= LICW_QSO_TEMPLATE_COUNT) {
        return &licwQSOTemplates[0];
    }
    return &licwQSOTemplates[index];
}

#endif // TRAINING_LICW_DATA_H
