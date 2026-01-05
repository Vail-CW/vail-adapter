/*
 * Spark Watch Challenge Data
 *
 * Contains all challenge definitions in PROGMEM for the Spark Watch game.
 * Challenges are ported from the Telegraph Relay web app plus new Summit-exclusive content.
 *
 * Challenge data format:
 * - id, title, difficulty
 * - morseTransmission (the message to decode)
 * - signalType, shipName, distressNature (expected answers)
 * - position fields (lat/lon degrees, minutes, direction)
 * - briefing, debriefing, hint (narrative content)
 * - campaignId, missionNumber (0 for standalone challenges)
 */

#ifndef GAME_SPARK_WATCH_DATA_H
#define GAME_SPARK_WATCH_DATA_H

#include "game_spark_watch.h"

// ============================================
// Campaign Definitions
// ============================================

const SparkWatchCampaign sparkCampaigns[] = {
    // Campaign 1: Through the Fog (1909) - RMS Republic
    {
        1,
        "Through the Fog (1909)",
        "RMS Republic",
        "First successful use of wireless distress signals. Operator Jack Binns transmitted CQD for 14 hours, saving 1,500 passengers.",
        1909,
        5,   // totalMissions
        0    // unlockRequirement (0 = always unlocked)
    },
    // Campaign 2: A Night to Remember (1912) - RMS Titanic
    {
        2,
        "A Night to Remember (1912)",
        "RMS Titanic",
        "The most famous maritime disaster. Operators Phillips and Bride transmitted distress calls for hours. 1,500+ died, but 710 were rescued by Carpathia.",
        1912,
        5,
        1    // Unlock after completing Campaign 1
    },
    // Campaign 3: Eighteen Minutes (1915) - RMS Lusitania
    {
        3,
        "Eighteen Minutes (1915)",
        "RMS Lusitania",
        "German U-boat torpedo. Ship sank in 18 minutes off Irish coast. 1,198 died, including 128 Americans. This event contributed to US entering WWI.",
        1915,
        5,
        2    // Unlock after completing Campaign 2
    },
    // Campaign 4: Dangerous Waters (1956) - SS Andrea Doria
    {
        4,
        "Dangerous Waters (1956)",
        "SS Andrea Doria",
        "Collision in fog off Nantucket despite both ships having radar. 1,660 rescued, only 46 died. Most successful large-scale maritime rescue of modern era.",
        1956,
        5,
        3    // Unlock after completing Campaign 3
    },
    // Campaign 5: Baltic Nightmare (1945) - Wilhelm Gustloff
    {
        5,
        "Baltic Nightmare (1945)",
        "MV Wilhelm Gustloff",
        "Three Soviet submarine torpedoes struck evacuation ship. Over 10,000 passengers. Only 1,252 rescued. Deadliest maritime disaster in history.",
        1945,
        5,
        4    // Unlock after completing Campaign 4
    }
};

const int SPARK_CAMPAIGN_COUNT = sizeof(sparkCampaigns) / sizeof(sparkCampaigns[0]);

// ============================================
// Easy Challenges (Signal + Ship Name)
// Base: 25 points | All speeds (0.5x - 2.0x)
// ============================================

// --- Easy 01: Rocky Shores (Historical - Slavonia 1909) ---
const char EASY_01_ID[] PROGMEM = "rocky-shores";
const char EASY_01_TITLE[] PROGMEM = "Rocky Shores";
const char EASY_01_MORSE[] PROGMEM = "CQD CQD CQD DE SLAVONIA";
const char EASY_01_SIGNAL[] PROGMEM = "CQD";
const char EASY_01_SHIP[] PROGMEM = "SLAVONIA";
const char EASY_01_BRIEF[] PROGMEM = "A vessel has run aground near the Azores. Copy the distress signal and identify the ship.";
const char EASY_01_DEBRIEF[] PROGMEM = "The Slavonia was one of the early ships to use wireless distress signals. All passengers were rescued.";

// --- Easy 02: Dead in the Water (Practice) ---
const char EASY_02_ID[] PROGMEM = "practice-001";
const char EASY_02_TITLE[] PROGMEM = "Dead in the Water";
const char EASY_02_MORSE[] PROGMEM = "SOS SOS SOS DE ATLANTIC STAR";
const char EASY_02_SIGNAL[] PROGMEM = "SOS";
const char EASY_02_SHIP[] PROGMEM = "ATLANTIC STAR";
const char EASY_02_BRIEF[] PROGMEM = "A cargo vessel reports engine failure in the Atlantic. Copy the distress signal and identify the ship.";
const char EASY_02_DEBRIEF[] PROGMEM = "Good copy! You correctly identified the SOS signal and ship name.";

// --- Easy 03: Through the Fog (Historical - Republic 1909) ---
const char EASY_03_ID[] PROGMEM = "through-the-fog";
const char EASY_03_TITLE[] PROGMEM = "Through the Fog";
const char EASY_03_MORSE[] PROGMEM = "CQD CQD CQD DE MKC REPUBLIC";
const char EASY_03_SIGNAL[] PROGMEM = "CQD";
const char EASY_03_SHIP[] PROGMEM = "MKC REPUBLIC";
const char EASY_03_BRIEF[] PROGMEM = "The first successful use of wireless to coordinate a maritime rescue. Copy the distress signal and identify the vessel.";
const char EASY_03_DEBRIEF[] PROGMEM = "Jack Binns transmitted CQD for 14 hours, leading to the rescue of over 1,500 passengers.";

// --- Easy 04: First Light (Practice) ---
const char EASY_04_ID[] PROGMEM = "practice-005";
const char EASY_04_TITLE[] PROGMEM = "First Light";
const char EASY_04_MORSE[] PROGMEM = "SOS SOS SOS DE MORNING LIGHT";
const char EASY_04_SIGNAL[] PROGMEM = "SOS";
const char EASY_04_SHIP[] PROGMEM = "MORNING LIGHT";
const char EASY_04_BRIEF[] PROGMEM = "An urgent distress call from a passenger vessel. Copy the signal and ship name.";
const char EASY_04_DEBRIEF[] PROGMEM = "Well done! You identified the distress signal correctly.";

// --- Easy 05: Small Craft Warning (Practice) ---
const char EASY_05_ID[] PROGMEM = "practice-006";
const char EASY_05_TITLE[] PROGMEM = "Small Craft Warning";
const char EASY_05_MORSE[] PROGMEM = "SOS SOS SOS DE SEA ROVER";
const char EASY_05_SIGNAL[] PROGMEM = "SOS";
const char EASY_05_SHIP[] PROGMEM = "SEA ROVER";
const char EASY_05_BRIEF[] PROGMEM = "A fishing vessel in distress. Copy the signal and identification.";
const char EASY_05_DEBRIEF[] PROGMEM = "Good work! Every second counts when responding to distress calls.";

// --- Easy 06: Making History (Historical - Arapahoe 1909) ---
const char EASY_06_ID[] PROGMEM = "first-sos";
const char EASY_06_TITLE[] PROGMEM = "Making History";
const char EASY_06_MORSE[] PROGMEM = "SOS SOS SOS DE ARAPAHOE";
const char EASY_06_SIGNAL[] PROGMEM = "SOS";
const char EASY_06_SHIP[] PROGMEM = "ARAPAHOE";
const char EASY_06_BRIEF[] PROGMEM = "August 1909. A vessel off Cape Hatteras transmits a historic signal - the first American ship to use SOS.";
const char EASY_06_DEBRIEF[] PROGMEM = "The SS Arapahoe was the first American ship to use the SOS distress signal.";

// --- Easy 07: The Other Ship (Historical - Florida 1909) ---
const char EASY_07_ID[] PROGMEM = "fog-collision";
const char EASY_07_TITLE[] PROGMEM = "The Other Ship";
const char EASY_07_MORSE[] PROGMEM = "CQD CQD CQD DE FLORIDA";
const char EASY_07_SIGNAL[] PROGMEM = "CQD";
const char EASY_07_SHIP[] PROGMEM = "FLORIDA";
const char EASY_07_BRIEF[] PROGMEM = "January 1909. In the same collision that made wireless history, another vessel also sends distress.";
const char EASY_07_DEBRIEF[] PROGMEM = "The SS Florida collided with RMS Republic in dense fog. Both ships sent wireless distress signals.";

// --- Easy 08: Silent Witness (Historical - Californian 1915) ---
const char EASY_08_ID[] PROGMEM = "silent-witness";
const char EASY_08_TITLE[] PROGMEM = "The Silent Witness";
const char EASY_08_MORSE[] PROGMEM = "SOS SOS SOS DE CALIFORNIAN";
const char EASY_08_SIGNAL[] PROGMEM = "SOS";
const char EASY_08_SHIP[] PROGMEM = "CALIFORNIAN";
const char EASY_08_BRIEF[] PROGMEM = "A ship infamous for its role in another disaster now sends her own distress call.";
const char EASY_08_DEBRIEF[] PROGMEM = "The Californian was controversially nearby during the Titanic disaster.";

// --- Easy 09: Children at Sea (Historical - City of Benares 1940) ---
const char EASY_09_ID[] PROGMEM = "childrens-voyage";
const char EASY_09_TITLE[] PROGMEM = "Children at Sea";
const char EASY_09_MORSE[] PROGMEM = "SOS SOS SOS DE CITY OF BENARES";
const char EASY_09_SIGNAL[] PROGMEM = "SOS";
const char EASY_09_SHIP[] PROGMEM = "CITY OF BENARES";
const char EASY_09_BRIEF[] PROGMEM = "September 1940. A passenger liner carrying evacuee children sends a distress call.";
const char EASY_09_DEBRIEF[] PROGMEM = "The City of Benares tragedy led to the end of the child evacuation program.";

// --- Easy 10: Hidden Danger Below (Historical - Niagara 1940) ---
const char EASY_10_ID[] PROGMEM = "mine-strike";
const char EASY_10_TITLE[] PROGMEM = "Hidden Danger Below";
const char EASY_10_MORSE[] PROGMEM = "SOS SOS SOS DE NIAGARA";
const char EASY_10_SIGNAL[] PROGMEM = "SOS";
const char EASY_10_SHIP[] PROGMEM = "NIAGARA";
const char EASY_10_BRIEF[] PROGMEM = "June 1940. A liner carrying gold bullion strikes a mine off New Zealand.";
const char EASY_10_DEBRIEF[] PROGMEM = "The gold from the Niagara was later recovered in a remarkable salvage operation.";

// --- Easy 11: Harbor Peril (Historical - Mont Blanc 1917) ---
const char EASY_11_ID[] PROGMEM = "halifax-dawn";
const char EASY_11_TITLE[] PROGMEM = "Harbor Peril";
const char EASY_11_MORSE[] PROGMEM = "CQD CQD CQD DE MONT BLANC";
const char EASY_11_SIGNAL[] PROGMEM = "CQD";
const char EASY_11_SHIP[] PROGMEM = "MONT BLANC";
const char EASY_11_BRIEF[] PROGMEM = "December 1917. An ammunition ship in Halifax Harbor sends an urgent signal after a collision.";
const char EASY_11_DEBRIEF[] PROGMEM = "The Halifax Explosion was the largest man-made explosion before the atomic bomb.";

// --- Easy 12: River Tragedy (Historical - Eastland 1915) ---
const char EASY_12_ID[] PROGMEM = "river-disaster";
const char EASY_12_TITLE[] PROGMEM = "River Tragedy";
const char EASY_12_MORSE[] PROGMEM = "SOS SOS SOS DE EASTLAND";
const char EASY_12_SIGNAL[] PROGMEM = "SOS";
const char EASY_12_SHIP[] PROGMEM = "EASTLAND";
const char EASY_12_BRIEF[] PROGMEM = "July 1915. A passenger steamer in the Chicago River sends a desperate call.";
const char EASY_12_DEBRIEF[] PROGMEM = "The Eastland capsized while still docked, killing 844 people.";

// --- Easy 13: Swift Messenger (Practice) ---
const char EASY_13_ID[] PROGMEM = "practice-013";
const char EASY_13_TITLE[] PROGMEM = "Swift Messenger";
const char EASY_13_MORSE[] PROGMEM = "SOS SOS SOS DE FALCON";
const char EASY_13_SIGNAL[] PROGMEM = "SOS";
const char EASY_13_SHIP[] PROGMEM = "FALCON";
const char EASY_13_BRIEF[] PROGMEM = "A small cargo vessel reports trouble at sea. Copy the distress signal.";
const char EASY_13_DEBRIEF[] PROGMEM = "Good copy! Quick identification of distress calls saves lives.";

// --- Easy 14: Celestial Navigator (Practice) ---
const char EASY_14_ID[] PROGMEM = "practice-014";
const char EASY_14_TITLE[] PROGMEM = "Celestial Navigator";
const char EASY_14_MORSE[] PROGMEM = "SOS SOS SOS DE NEPTUNE STAR";
const char EASY_14_SIGNAL[] PROGMEM = "SOS";
const char EASY_14_SHIP[] PROGMEM = "NEPTUNE STAR";
const char EASY_14_BRIEF[] PROGMEM = "A freighter drifting without power sends a distress call. Identify the vessel.";
const char EASY_14_DEBRIEF[] PROGMEM = "Well done! The ship's name helps rescuers identify and locate the vessel.";

// --- Easy 15: Shoal Waters (Practice) ---
const char EASY_15_ID[] PROGMEM = "practice-015";
const char EASY_15_TITLE[] PROGMEM = "Shoal Waters";
const char EASY_15_MORSE[] PROGMEM = "SOS SOS SOS DE COASTAL TRADER";
const char EASY_15_SIGNAL[] PROGMEM = "SOS";
const char EASY_15_SHIP[] PROGMEM = "COASTAL TRADER";
const char EASY_15_BRIEF[] PROGMEM = "A merchant vessel has run aground near the coast. Copy the signal and ship name.";
const char EASY_15_DEBRIEF[] PROGMEM = "Accurate identification is crucial for coordinating rescue efforts.";

// --- Easy 16: Northern Run (Practice) ---
const char EASY_16_ID[] PROGMEM = "practice-016";
const char EASY_16_TITLE[] PROGMEM = "Northern Run";
const char EASY_16_MORSE[] PROGMEM = "SOS SOS SOS DE BALTIC MERCHANT";
const char EASY_16_SIGNAL[] PROGMEM = "SOS";
const char EASY_16_SHIP[] PROGMEM = "BALTIC MERCHANT";
const char EASY_16_BRIEF[] PROGMEM = "A cargo ship in the Baltic Sea transmits a distress call. Identify the ship.";
const char EASY_16_DEBRIEF[] PROGMEM = "Good work copying through the static!";

// --- Easy 17: Harbor Bound (Practice) ---
const char EASY_17_ID[] PROGMEM = "practice-017";
const char EASY_17_TITLE[] PROGMEM = "Harbor Bound";
const char EASY_17_MORSE[] PROGMEM = "SOS SOS SOS DE HARBOR QUEEN";
const char EASY_17_SIGNAL[] PROGMEM = "SOS";
const char EASY_17_SHIP[] PROGMEM = "HARBOR QUEEN";
const char EASY_17_BRIEF[] PROGMEM = "A passenger ferry near port sends an emergency signal. Copy the distress call.";
const char EASY_17_DEBRIEF[] PROGMEM = "Even near port, emergencies require swift response.";

// --- Easy 18: Dawn Patrol (Practice) ---
const char EASY_18_ID[] PROGMEM = "practice-018";
const char EASY_18_TITLE[] PROGMEM = "Dawn Patrol";
const char EASY_18_MORSE[] PROGMEM = "SOS SOS SOS DE DAWN TREADER";
const char EASY_18_SIGNAL[] PROGMEM = "SOS";
const char EASY_18_SHIP[] PROGMEM = "DAWN TREADER";
const char EASY_18_BRIEF[] PROGMEM = "An early morning distress call from a vessel in trouble. Identify the ship.";
const char EASY_18_DEBRIEF[] PROGMEM = "Vigilance at all hours is essential for radio operators.";

// --- Easy 19: Moonlit Waters (Practice) ---
const char EASY_19_ID[] PROGMEM = "practice-019";
const char EASY_19_TITLE[] PROGMEM = "Moonlit Waters";
const char EASY_19_MORSE[] PROGMEM = "SOS SOS SOS DE SILVER WAKE";
const char EASY_19_SIGNAL[] PROGMEM = "SOS";
const char EASY_19_SHIP[] PROGMEM = "SILVER WAKE";
const char EASY_19_BRIEF[] PROGMEM = "A ship sends a distress signal under the night sky. Copy the call.";
const char EASY_19_DEBRIEF[] PROGMEM = "Night operations require extra attention to detail.";

// --- Easy 20: Tropical Trouble (Practice) ---
const char EASY_20_ID[] PROGMEM = "practice-020";
const char EASY_20_TITLE[] PROGMEM = "Tropical Trouble";
const char EASY_20_MORSE[] PROGMEM = "SOS SOS SOS DE CORAL BAY";
const char EASY_20_SIGNAL[] PROGMEM = "SOS";
const char EASY_20_SHIP[] PROGMEM = "CORAL BAY";
const char EASY_20_BRIEF[] PROGMEM = "A vessel in warm waters sends an urgent distress call. Identify the ship.";
const char EASY_20_DEBRIEF[] PROGMEM = "Excellent work! You've completed the Easy challenges.";

// ============================================
// Medium Challenges (Signal + Ship + Nature)
// Base: 50 points | Min 0.75x speed
// ============================================

// --- Medium 01: Smoke on the Horizon (Historical - Volturno 1913) ---
const char MED_01_ID[] PROGMEM = "smoke-horizon";
const char MED_01_TITLE[] PROGMEM = "Smoke on the Horizon";
const char MED_01_MORSE[] PROGMEM = "CQD CQD CQD DE VOLTURNO FIRE AT SEA";
const char MED_01_SIGNAL[] PROGMEM = "CQD";
const char MED_01_SHIP[] PROGMEM = "VOLTURNO";
const char MED_01_NATURE[] PROGMEM = "FIRE AT SEA";
const char MED_01_BRIEF[] PROGMEM = "Fire has broken out aboard a vessel at sea. Copy the signal, ship name, and nature of distress.";
const char MED_01_DEBRIEF[] PROGMEM = "Ten ships responded to the wireless calls, rescuing 521 passengers.";

// --- Medium 02: Rising Waters (Practice) ---
const char MED_02_ID[] PROGMEM = "practice-002";
const char MED_02_TITLE[] PROGMEM = "Rising Waters";
const char MED_02_MORSE[] PROGMEM = "SOS SOS SOS DE PACIFIC TRADER FLOODING";
const char MED_02_SIGNAL[] PROGMEM = "SOS";
const char MED_02_SHIP[] PROGMEM = "PACIFIC TRADER";
const char MED_02_NATURE[] PROGMEM = "FLOODING";
const char MED_02_BRIEF[] PROGMEM = "A merchant vessel is taking on water. Copy the full distress message including the nature of emergency.";
const char MED_02_DEBRIEF[] PROGMEM = "Good copy! Understanding the nature of distress helps coordinate the right response.";

// --- Medium 03: Inferno at Sea (Historical - Morro Castle 1934) ---
const char MED_03_ID[] PROGMEM = "inferno-at-sea";
const char MED_03_TITLE[] PROGMEM = "Inferno at Sea";
const char MED_03_MORSE[] PROGMEM = "SOS SOS SOS DE MORRO CASTLE FIRE";
const char MED_03_SIGNAL[] PROGMEM = "SOS";
const char MED_03_SHIP[] PROGMEM = "MORRO CASTLE";
const char MED_03_NATURE[] PROGMEM = "FIRE";
const char MED_03_BRIEF[] PROGMEM = "A cruise liner is ablaze off the coast. Copy the signal, ship name, and nature of distress.";
const char MED_03_DEBRIEF[] PROGMEM = "The Morro Castle disaster killed 137 people and led to major safety reforms.";

// --- Medium 04: Stuck Fast (Practice) ---
const char MED_04_ID[] PROGMEM = "practice-007";
const char MED_04_TITLE[] PROGMEM = "Stuck Fast";
const char MED_04_MORSE[] PROGMEM = "SOS SOS SOS DE IRON DUKE GROUNDED";
const char MED_04_SIGNAL[] PROGMEM = "SOS";
const char MED_04_SHIP[] PROGMEM = "IRON DUKE";
const char MED_04_NATURE[] PROGMEM = "GROUNDED";
const char MED_04_BRIEF[] PROGMEM = "A cargo vessel has run aground on a reef. Copy the full distress message.";
const char MED_04_DEBRIEF[] PROGMEM = "Grounding is a serious emergency that requires immediate assistance.";

// --- Medium 05: Power Lost (Practice) ---
const char MED_05_ID[] PROGMEM = "practice-008";
const char MED_05_TITLE[] PROGMEM = "Power Lost";
const char MED_05_MORSE[] PROGMEM = "SOS SOS SOS DE CRYSTAL BAY ENGINE FAILURE";
const char MED_05_SIGNAL[] PROGMEM = "SOS";
const char MED_05_SHIP[] PROGMEM = "CRYSTAL BAY";
const char MED_05_NATURE[] PROGMEM = "ENGINE FAILURE";
const char MED_05_BRIEF[] PROGMEM = "A tanker reports complete engine failure in heavy seas. Copy the distress call.";
const char MED_05_DEBRIEF[] PROGMEM = "Engine failure in heavy seas is extremely dangerous.";

// --- Medium 06: Fractured Steel (Historical - Principessa Mafalda 1927) ---
const char MED_06_ID[] PROGMEM = "fractured-hull";
const char MED_06_TITLE[] PROGMEM = "Fractured Steel";
const char MED_06_MORSE[] PROGMEM = "SOS SOS SOS DE PRINCIPESSA MAFALDA PROPELLER SHAFT BROKEN";
const char MED_06_SIGNAL[] PROGMEM = "SOS";
const char MED_06_SHIP[] PROGMEM = "PRINCIPESSA MAFALDA";
const char MED_06_NATURE[] PROGMEM = "PROPELLER SHAFT BROKEN";
const char MED_06_BRIEF[] PROGMEM = "October 1927. An Italian liner suffers catastrophic mechanical failure off Brazil.";
const char MED_06_DEBRIEF[] PROGMEM = "The Principessa Mafalda disaster killed 314 passengers and crew.";

// --- Medium 07: The Laconia Incident (Historical - Laconia 1942) ---
const char MED_07_ID[] PROGMEM = "laconia-incident";
const char MED_07_TITLE[] PROGMEM = "The Laconia Incident";
const char MED_07_MORSE[] PROGMEM = "SOS SOS SOS DE LACONIA TORPEDOED";
const char MED_07_SIGNAL[] PROGMEM = "SOS";
const char MED_07_SHIP[] PROGMEM = "LACONIA";
const char MED_07_NATURE[] PROGMEM = "TORPEDOED";
const char MED_07_BRIEF[] PROGMEM = "September 1942. A troopship is torpedoed off the West African coast.";
const char MED_07_DEBRIEF[] PROGMEM = "The German submarine commander attempted a rescue, leading to the Laconia Order.";

// --- Medium 08: No Survivors (Historical - Ceramic 1942) ---
const char MED_08_ID[] PROGMEM = "ceramic-tragedy";
const char MED_08_TITLE[] PROGMEM = "No Survivors";
const char MED_08_MORSE[] PROGMEM = "SOS SOS SOS DE CERAMIC TORPEDOED";
const char MED_08_SIGNAL[] PROGMEM = "SOS";
const char MED_08_SHIP[] PROGMEM = "CERAMIC";
const char MED_08_NATURE[] PROGMEM = "TORPEDOED";
const char MED_08_BRIEF[] PROGMEM = "December 1942. A liner carrying passengers and troops is attacked in the North Atlantic.";
const char MED_08_DEBRIEF[] PROGMEM = "Only one person survived from the 656 aboard the Ceramic.";

// --- Medium 09: Norwegian Rocks (Historical - Dresden 1934) ---
const char MED_09_ID[] PROGMEM = "norwegian-reef";
const char MED_09_TITLE[] PROGMEM = "Norwegian Rocks";
const char MED_09_MORSE[] PROGMEM = "SOS SOS SOS DE DRESDEN GROUNDED";
const char MED_09_SIGNAL[] PROGMEM = "SOS";
const char MED_09_SHIP[] PROGMEM = "DRESDEN";
const char MED_09_NATURE[] PROGMEM = "GROUNDED";
const char MED_09_BRIEF[] PROGMEM = "June 1934. A German cruise liner runs aground in Norwegian waters.";
const char MED_09_DEBRIEF[] PROGMEM = "All passengers were safely evacuated thanks to effective wireless communication.";

// --- Medium 10: Maiden Voyage Lost (Historical - Hans Hedtoft 1959) ---
const char MED_10_ID[] PROGMEM = "maiden-tragedy";
const char MED_10_TITLE[] PROGMEM = "Maiden Voyage Lost";
const char MED_10_MORSE[] PROGMEM = "SOS SOS SOS DE HANS HEDTOFT STRUCK ICEBERG";
const char MED_10_SIGNAL[] PROGMEM = "SOS";
const char MED_10_SHIP[] PROGMEM = "HANS HEDTOFT";
const char MED_10_NATURE[] PROGMEM = "STRUCK ICEBERG";
const char MED_10_BRIEF[] PROGMEM = "January 1959. A Danish liner on her maiden voyage strikes ice off Greenland.";
const char MED_10_DEBRIEF[] PROGMEM = "The Hans Hedtoft was lost with all 95 aboard. The wreck has never been found.";

// --- Medium 11: November Storm (Historical - Edmund Fitzgerald 1975) ---
const char MED_11_ID[] PROGMEM = "november-gale";
const char MED_11_TITLE[] PROGMEM = "November Storm";
const char MED_11_MORSE[] PROGMEM = "SOS SOS SOS DE EDMUND FITZGERALD HEAVY SEAS";
const char MED_11_SIGNAL[] PROGMEM = "SOS";
const char MED_11_SHIP[] PROGMEM = "EDMUND FITZGERALD";
const char MED_11_NATURE[] PROGMEM = "HEAVY SEAS";
const char MED_11_BRIEF[] PROGMEM = "November 1975. A Great Lakes freighter battles a deadly storm on Lake Superior.";
const char MED_11_DEBRIEF[] PROGMEM = "The Edmund Fitzgerald sank with all 29 crew. Immortalized in song by Gordon Lightfoot.";

// --- Medium 12: Winter Fury (Historical - Marine Electric 1983) ---
const char MED_12_ID[] PROGMEM = "february-storm";
const char MED_12_TITLE[] PROGMEM = "Winter Fury";
const char MED_12_MORSE[] PROGMEM = "SOS SOS SOS DE MARINE ELECTRIC FLOODING";
const char MED_12_SIGNAL[] PROGMEM = "SOS";
const char MED_12_SHIP[] PROGMEM = "MARINE ELECTRIC";
const char MED_12_NATURE[] PROGMEM = "FLOODING";
const char MED_12_BRIEF[] PROGMEM = "February 1983. A coal carrier succumbs to winter storms off Virginia.";
const char MED_12_DEBRIEF[] PROGMEM = "Only 3 of 34 crew survived. Led to major Coast Guard reforms.";

// --- Medium 13: River Explosion (Historical - Kiangya 1948) ---
const char MED_13_ID[] PROGMEM = "huangpu-tragedy";
const char MED_13_TITLE[] PROGMEM = "River Explosion";
const char MED_13_MORSE[] PROGMEM = "SOS SOS SOS DE KIANGYA STRUCK MINE";
const char MED_13_SIGNAL[] PROGMEM = "SOS";
const char MED_13_SHIP[] PROGMEM = "KIANGYA";
const char MED_13_NATURE[] PROGMEM = "STRUCK MINE";
const char MED_13_BRIEF[] PROGMEM = "December 1948. A Chinese passenger ship strikes a mine in the Huangpu River.";
const char MED_13_DEBRIEF[] PROGMEM = "The Kiangya disaster killed over 2,750 people.";

// --- Medium 14: Lost Rudder (Practice) ---
const char MED_14_ID[] PROGMEM = "practice-021";
const char MED_14_TITLE[] PROGMEM = "Lost Rudder";
const char MED_14_MORSE[] PROGMEM = "SOS SOS SOS DE PACIFIC WIND STEERING FAILURE";
const char MED_14_SIGNAL[] PROGMEM = "SOS";
const char MED_14_SHIP[] PROGMEM = "PACIFIC WIND";
const char MED_14_NATURE[] PROGMEM = "STEERING FAILURE";
const char MED_14_BRIEF[] PROGMEM = "A cargo vessel has lost steering control in heavy traffic.";
const char MED_14_DEBRIEF[] PROGMEM = "Steering failure in a shipping lane is extremely dangerous.";

// --- Medium 15: Cracked Hull (Practice) ---
const char MED_15_ID[] PROGMEM = "practice-022";
const char MED_15_TITLE[] PROGMEM = "Cracked Hull";
const char MED_15_MORSE[] PROGMEM = "SOS SOS SOS DE NORTHERN PROMISE HULL BREACH";
const char MED_15_SIGNAL[] PROGMEM = "SOS";
const char MED_15_SHIP[] PROGMEM = "NORTHERN PROMISE";
const char MED_15_NATURE[] PROGMEM = "HULL BREACH";
const char MED_15_BRIEF[] PROGMEM = "A freighter reports water ingress through hull damage.";
const char MED_15_DEBRIEF[] PROGMEM = "Hull breaches require immediate damage control.";

// --- Medium 16: Shifting Load (Practice) ---
const char MED_16_ID[] PROGMEM = "practice-023";
const char MED_16_TITLE[] PROGMEM = "Shifting Load";
const char MED_16_MORSE[] PROGMEM = "SOS SOS SOS DE EASTERN FORTUNE CARGO SHIFT";
const char MED_16_SIGNAL[] PROGMEM = "SOS";
const char MED_16_SHIP[] PROGMEM = "EASTERN FORTUNE";
const char MED_16_NATURE[] PROGMEM = "CARGO SHIFT";
const char MED_16_BRIEF[] PROGMEM = "A container ship reports dangerous cargo movement.";
const char MED_16_DEBRIEF[] PROGMEM = "Cargo shift can cause a vessel to capsize rapidly.";

// --- Medium 17: Engine Blaze (Practice) ---
const char MED_17_ID[] PROGMEM = "practice-024";
const char MED_17_TITLE[] PROGMEM = "Engine Blaze";
const char MED_17_MORSE[] PROGMEM = "SOS SOS SOS DE OCEAN PIONEER ENGINE ROOM FIRE";
const char MED_17_SIGNAL[] PROGMEM = "SOS";
const char MED_17_SHIP[] PROGMEM = "OCEAN PIONEER";
const char MED_17_NATURE[] PROGMEM = "ENGINE ROOM FIRE";
const char MED_17_BRIEF[] PROGMEM = "Fire has broken out in the engine room of a cargo vessel.";
const char MED_17_DEBRIEF[] PROGMEM = "Engine room fires are among the most dangerous shipboard emergencies.";

// --- Medium 18: Storm Damage (Practice) ---
const char MED_18_ID[] PROGMEM = "practice-025";
const char MED_18_TITLE[] PROGMEM = "Storm Damage";
const char MED_18_MORSE[] PROGMEM = "SOS SOS SOS DE ATLANTIC DAWN STRUCTURAL DAMAGE";
const char MED_18_SIGNAL[] PROGMEM = "SOS";
const char MED_18_SHIP[] PROGMEM = "ATLANTIC DAWN";
const char MED_18_NATURE[] PROGMEM = "STRUCTURAL DAMAGE";
const char MED_18_BRIEF[] PROGMEM = "A vessel has sustained serious damage in rough weather.";
const char MED_18_DEBRIEF[] PROGMEM = "Structural damage can compromise a ship's seaworthiness.";

// --- Medium 19: Heavy List (Practice) ---
const char MED_19_ID[] PROGMEM = "practice-026";
const char MED_19_TITLE[] PROGMEM = "Heavy List";
const char MED_19_MORSE[] PROGMEM = "SOS SOS SOS DE MOUNTAIN STAR LISTING HEAVILY";
const char MED_19_SIGNAL[] PROGMEM = "SOS";
const char MED_19_SHIP[] PROGMEM = "MOUNTAIN STAR";
const char MED_19_NATURE[] PROGMEM = "LISTING HEAVILY";
const char MED_19_BRIEF[] PROGMEM = "A ship is developing a dangerous list.";
const char MED_19_DEBRIEF[] PROGMEM = "A severe list can prevent lifeboat deployment.";

// --- Medium 20: Water Rising (Practice) ---
const char MED_20_ID[] PROGMEM = "practice-027";
const char MED_20_TITLE[] PROGMEM = "Water Rising";
const char MED_20_MORSE[] PROGMEM = "SOS SOS SOS DE WESTERN HORIZON TAKING ON WATER";
const char MED_20_SIGNAL[] PROGMEM = "SOS";
const char MED_20_SHIP[] PROGMEM = "WESTERN HORIZON";
const char MED_20_NATURE[] PROGMEM = "TAKING ON WATER";
const char MED_20_BRIEF[] PROGMEM = "A vessel reports uncontrolled flooding.";
const char MED_20_DEBRIEF[] PROGMEM = "Excellent work! You've completed the Medium challenges.";

// ============================================
// Hard Challenges (Signal + Ship + Nature + Position)
// Base: 100 points | Min 1.0x speed
// ============================================

// --- Hard 01: Into the Deep (Historical - Vestris 1928) ---
const char HARD_01_ID[] PROGMEM = "into-the-deep";
const char HARD_01_TITLE[] PROGMEM = "Into the Deep";
const char HARD_01_MORSE[] PROGMEM = "SOS SOS SOS DE VESTRIS SINKING 37N45 71W08";
const char HARD_01_SIGNAL[] PROGMEM = "SOS";
const char HARD_01_SHIP[] PROGMEM = "VESTRIS";
const char HARD_01_NATURE[] PROGMEM = "SINKING";
const char HARD_01_LAT_DEG[] PROGMEM = "37";
const char HARD_01_LAT_MIN[] PROGMEM = "45";
const char HARD_01_LON_DEG[] PROGMEM = "71";
const char HARD_01_LON_MIN[] PROGMEM = "08";
const char HARD_01_BRIEF[] PROGMEM = "A vessel is sinking in a storm. Record the full distress call including position coordinates.";
const char HARD_01_DEBRIEF[] PROGMEM = "The Vestris sinking killed 128 people due to design flaws.";

// --- Hard 02: Blind Impact (Practice) ---
const char HARD_02_ID[] PROGMEM = "practice-003";
const char HARD_02_TITLE[] PROGMEM = "Blind Impact";
const char HARD_02_MORSE[] PROGMEM = "SOS SOS SOS DE NORTHERN HORIZON COLLISION 48N22 05W30";
const char HARD_02_SIGNAL[] PROGMEM = "SOS";
const char HARD_02_SHIP[] PROGMEM = "NORTHERN HORIZON";
const char HARD_02_NATURE[] PROGMEM = "COLLISION";
const char HARD_02_LAT_DEG[] PROGMEM = "48";
const char HARD_02_LAT_MIN[] PROGMEM = "22";
const char HARD_02_LON_DEG[] PROGMEM = "05";
const char HARD_02_LON_MIN[] PROGMEM = "30";
const char HARD_02_BRIEF[] PROGMEM = "Collision reported at sea. Copy signal, ship name, nature of distress, and position.";
const char HARD_02_DEBRIEF[] PROGMEM = "Position coordinates are crucial for directing rescue vessels.";

// --- Hard 03: Dangerous Crossing (Historical - Andrea Doria 1956) ---
const char HARD_03_ID[] PROGMEM = "dangerous-crossing";
const char HARD_03_TITLE[] PROGMEM = "Dangerous Crossing";
const char HARD_03_MORSE[] PROGMEM = "SOS SOS SOS DE ICEH ANDREA DORIA COLLISION 40N30 69W53";
const char HARD_03_SIGNAL[] PROGMEM = "SOS";
const char HARD_03_SHIP[] PROGMEM = "ICEH ANDREA DORIA";
const char HARD_03_NATURE[] PROGMEM = "COLLISION";
const char HARD_03_LAT_DEG[] PROGMEM = "40";
const char HARD_03_LAT_MIN[] PROGMEM = "30";
const char HARD_03_LON_DEG[] PROGMEM = "69";
const char HARD_03_LON_MIN[] PROGMEM = "53";
const char HARD_03_BRIEF[] PROGMEM = "A liner has collided with another ship in foggy waters. Copy the full distress call with position.";
const char HARD_03_DEBRIEF[] PROGMEM = "The Andrea Doria collision resulted in a remarkably successful rescue of 1,660 passengers.";

// --- Hard 04: Engine Room Blaze (Practice) ---
const char HARD_04_ID[] PROGMEM = "practice-009";
const char HARD_04_TITLE[] PROGMEM = "Engine Room Blaze";
const char HARD_04_MORSE[] PROGMEM = "SOS SOS SOS DE GOLDEN GATE FIRE 34N08 119W24";
const char HARD_04_SIGNAL[] PROGMEM = "SOS";
const char HARD_04_SHIP[] PROGMEM = "GOLDEN GATE";
const char HARD_04_NATURE[] PROGMEM = "FIRE";
const char HARD_04_LAT_DEG[] PROGMEM = "34";
const char HARD_04_LAT_MIN[] PROGMEM = "08";
const char HARD_04_LON_DEG[] PROGMEM = "119";
const char HARD_04_LON_MIN[] PROGMEM = "24";
const char HARD_04_BRIEF[] PROGMEM = "Fire in the engine room of a container ship. Copy complete distress with position.";
const char HARD_04_DEBRIEF[] PROGMEM = "Engine room fires require specialized firefighting response.";

// --- Hard 05: Cold Descent (Practice) ---
const char HARD_05_ID[] PROGMEM = "practice-010";
const char HARD_05_TITLE[] PROGMEM = "Cold Descent";
const char HARD_05_MORSE[] PROGMEM = "SOS SOS SOS DE ARCTIC EXPLORER SINKING 58N42 02E15";
const char HARD_05_SIGNAL[] PROGMEM = "SOS";
const char HARD_05_SHIP[] PROGMEM = "ARCTIC EXPLORER";
const char HARD_05_NATURE[] PROGMEM = "SINKING";
const char HARD_05_LAT_DEG[] PROGMEM = "58";
const char HARD_05_LAT_MIN[] PROGMEM = "42";
const char HARD_05_LON_DEG[] PROGMEM = "02";
const char HARD_05_LON_MIN[] PROGMEM = "15";
const char HARD_05_BRIEF[] PROGMEM = "A research vessel is sinking in frigid waters. Copy all details including coordinates.";
const char HARD_05_DEBRIEF[] PROGMEM = "Cold water emergencies require rapid response to prevent hypothermia.";

// --- Hard 06: First Blood (Historical - Athenia 1939) ---
const char HARD_06_ID[] PROGMEM = "wars-first-victim";
const char HARD_06_TITLE[] PROGMEM = "First Blood";
const char HARD_06_MORSE[] PROGMEM = "SOS SOS SOS DE ATHENIA TORPEDOED 56N42 14W05";
const char HARD_06_SIGNAL[] PROGMEM = "SOS";
const char HARD_06_SHIP[] PROGMEM = "ATHENIA";
const char HARD_06_NATURE[] PROGMEM = "TORPEDOED";
const char HARD_06_LAT_DEG[] PROGMEM = "56";
const char HARD_06_LAT_MIN[] PROGMEM = "42";
const char HARD_06_LON_DEG[] PROGMEM = "14";
const char HARD_06_LON_MIN[] PROGMEM = "05";
const char HARD_06_BRIEF[] PROGMEM = "September 3, 1939. The first ship torpedoed in WWII sends her position.";
const char HARD_06_DEBRIEF[] PROGMEM = "The Athenia was sunk just hours after Britain declared war on Germany.";

// --- Hard 07: Atlantic Ambush (Historical - Empress of Britain 1940) ---
const char HARD_07_ID[] PROGMEM = "empress-attack";
const char HARD_07_TITLE[] PROGMEM = "Atlantic Ambush";
const char HARD_07_MORSE[] PROGMEM = "SOS SOS SOS DE EMPRESS OF BRITAIN BOMBED 55N16 09W50";
const char HARD_07_SIGNAL[] PROGMEM = "SOS";
const char HARD_07_SHIP[] PROGMEM = "EMPRESS OF BRITAIN";
const char HARD_07_NATURE[] PROGMEM = "BOMBED";
const char HARD_07_LAT_DEG[] PROGMEM = "55";
const char HARD_07_LAT_MIN[] PROGMEM = "16";
const char HARD_07_LON_DEG[] PROGMEM = "09";
const char HARD_07_LON_MIN[] PROGMEM = "50";
const char HARD_07_BRIEF[] PROGMEM = "October 1940. A grand liner is attacked by German bombers off Ireland.";
const char HARD_07_DEBRIEF[] PROGMEM = "The Empress of Britain was the largest ship sunk by a U-boat in WWII.";

// --- Hard 08: Dunkirk's Shadow (Historical - Lancastria 1940) ---
const char HARD_08_ID[] PROGMEM = "dunkirk-shadow";
const char HARD_08_TITLE[] PROGMEM = "Dunkirk's Shadow";
const char HARD_08_MORSE[] PROGMEM = "SOS SOS SOS DE LANCASTRIA BOMBED SINKING 47N12 02W20";
const char HARD_08_SIGNAL[] PROGMEM = "SOS";
const char HARD_08_SHIP[] PROGMEM = "LANCASTRIA";
const char HARD_08_NATURE[] PROGMEM = "BOMBED SINKING";
const char HARD_08_LAT_DEG[] PROGMEM = "47";
const char HARD_08_LAT_MIN[] PROGMEM = "12";
const char HARD_08_LON_DEG[] PROGMEM = "02";
const char HARD_08_LON_MIN[] PROGMEM = "20";
const char HARD_08_BRIEF[] PROGMEM = "June 1940. During the evacuation of France, a troopship is bombed.";
const char HARD_08_DEBRIEF[] PROGMEM = "The Lancastria disaster was Britain's worst maritime loss - news was suppressed.";

// --- Hard 09: Christmas Tragedy (Historical - Leopoldville 1944) ---
const char HARD_09_ID[] PROGMEM = "christmas-eve";
const char HARD_09_TITLE[] PROGMEM = "Christmas Tragedy";
const char HARD_09_MORSE[] PROGMEM = "SOS SOS SOS DE LEOPOLDVILLE TORPEDOED 49N54 01W33";
const char HARD_09_SIGNAL[] PROGMEM = "SOS";
const char HARD_09_SHIP[] PROGMEM = "LEOPOLDVILLE";
const char HARD_09_NATURE[] PROGMEM = "TORPEDOED";
const char HARD_09_LAT_DEG[] PROGMEM = "49";
const char HARD_09_LAT_MIN[] PROGMEM = "54";
const char HARD_09_LON_DEG[] PROGMEM = "01";
const char HARD_09_LON_MIN[] PROGMEM = "33";
const char HARD_09_BRIEF[] PROGMEM = "December 24, 1944. A troopship is torpedoed in the English Channel on Christmas Eve.";
const char HARD_09_DEBRIEF[] PROGMEM = "Over 760 US soldiers died on Christmas Eve, just 5 miles from shore.";

// --- Hard 10: Red Sea Tragedy (Historical - Salem Express 1991) ---
const char HARD_10_ID[] PROGMEM = "red-sea-reef";
const char HARD_10_TITLE[] PROGMEM = "Red Sea Tragedy";
const char HARD_10_MORSE[] PROGMEM = "SOS SOS SOS DE SALEM EXPRESS COLLISION REEF 26N28 33E55";
const char HARD_10_SIGNAL[] PROGMEM = "SOS";
const char HARD_10_SHIP[] PROGMEM = "SALEM EXPRESS";
const char HARD_10_NATURE[] PROGMEM = "COLLISION REEF";
const char HARD_10_LAT_DEG[] PROGMEM = "26";
const char HARD_10_LAT_MIN[] PROGMEM = "28";
const char HARD_10_LON_DEG[] PROGMEM = "33";
const char HARD_10_LON_MIN[] PROGMEM = "55";
const char HARD_10_BRIEF[] PROGMEM = "December 1991. A ferry strikes a reef in the Red Sea during a storm.";
const char HARD_10_DEBRIEF[] PROGMEM = "The Salem Express sank in 20 minutes with 470 passengers lost.";

// --- Hard 11: Bahamas Passage (Historical - Yarmouth Castle 1965) ---
const char HARD_11_ID[] PROGMEM = "bahamas-fire";
const char HARD_11_TITLE[] PROGMEM = "Bahamas Passage";
const char HARD_11_MORSE[] PROGMEM = "SOS SOS SOS DE YARMOUTH CASTLE FIRE 24N20 79W25";
const char HARD_11_SIGNAL[] PROGMEM = "SOS";
const char HARD_11_SHIP[] PROGMEM = "YARMOUTH CASTLE";
const char HARD_11_NATURE[] PROGMEM = "FIRE";
const char HARD_11_LAT_DEG[] PROGMEM = "24";
const char HARD_11_LAT_MIN[] PROGMEM = "20";
const char HARD_11_LON_DEG[] PROGMEM = "79";
const char HARD_11_LON_MIN[] PROGMEM = "25";
const char HARD_11_BRIEF[] PROGMEM = "November 1965. A cruise ship catches fire between Miami and Nassau.";
const char HARD_11_DEBRIEF[] PROGMEM = "The Yarmouth Castle fire killed 90 and led to new cruise ship safety laws.";

// --- Hard 12: Aegean Tempest (Historical - Heraklion 1966) ---
const char HARD_12_ID[] PROGMEM = "aegean-storm";
const char HARD_12_TITLE[] PROGMEM = "Aegean Tempest";
const char HARD_12_MORSE[] PROGMEM = "SOS SOS SOS DE HERAKLION CAPSIZING 35N50 25E10";
const char HARD_12_SIGNAL[] PROGMEM = "SOS";
const char HARD_12_SHIP[] PROGMEM = "HERAKLION";
const char HARD_12_NATURE[] PROGMEM = "CAPSIZING";
const char HARD_12_LAT_DEG[] PROGMEM = "35";
const char HARD_12_LAT_MIN[] PROGMEM = "50";
const char HARD_12_LON_DEG[] PROGMEM = "25";
const char HARD_12_LON_MIN[] PROGMEM = "10";
const char HARD_12_BRIEF[] PROGMEM = "December 1966. A Greek ferry capsizes in a violent storm.";
const char HARD_12_DEBRIEF[] PROGMEM = "The Heraklion disaster killed 217 people.";

// --- Hard 13: Tragic Mistake (Historical - Cap Arcona 1945) ---
const char HARD_13_ID[] PROGMEM = "friendly-fire";
const char HARD_13_TITLE[] PROGMEM = "Tragic Mistake";
const char HARD_13_MORSE[] PROGMEM = "SOS SOS SOS DE CAP ARCONA BOMBED 54N05 10E50";
const char HARD_13_SIGNAL[] PROGMEM = "SOS";
const char HARD_13_SHIP[] PROGMEM = "CAP ARCONA";
const char HARD_13_NATURE[] PROGMEM = "BOMBED";
const char HARD_13_LAT_DEG[] PROGMEM = "54";
const char HARD_13_LAT_MIN[] PROGMEM = "05";
const char HARD_13_LON_DEG[] PROGMEM = "10";
const char HARD_13_LON_MIN[] PROGMEM = "50";
const char HARD_13_BRIEF[] PROGMEM = "May 1945. In the final days of the war, a ship filled with prisoners is bombed by Allied aircraft.";
const char HARD_13_DEBRIEF[] PROGMEM = "Over 4,500 concentration camp prisoners died when RAF planes mistakenly attacked.";

// --- Hard 14-20: Practice challenges with positions ---
const char HARD_14_ID[] PROGMEM = "practice-028";
const char HARD_14_TITLE[] PROGMEM = "Aleutian Emergency";
const char HARD_14_MORSE[] PROGMEM = "SOS SOS SOS DE PACIFIC NAVIGATOR FLOODING 52N18 174W30";
const char HARD_14_SIGNAL[] PROGMEM = "SOS";
const char HARD_14_SHIP[] PROGMEM = "PACIFIC NAVIGATOR";
const char HARD_14_NATURE[] PROGMEM = "FLOODING";
const char HARD_14_LAT_DEG[] PROGMEM = "52";
const char HARD_14_LAT_MIN[] PROGMEM = "18";
const char HARD_14_LON_DEG[] PROGMEM = "174";
const char HARD_14_LON_MIN[] PROGMEM = "30";
const char HARD_14_BRIEF[] PROGMEM = "A cargo vessel is flooding in the remote Aleutian Islands.";
const char HARD_14_DEBRIEF[] PROGMEM = "Remote locations make rescue operations extremely challenging.";

const char HARD_15_ID[] PROGMEM = "practice-029";
const char HARD_15_TITLE[] PROGMEM = "Cape Horn Peril";
const char HARD_15_MORSE[] PROGMEM = "SOS SOS SOS DE SOUTHERN CROSS STORM DAMAGE 56S00 67W30";
const char HARD_15_SIGNAL[] PROGMEM = "SOS";
const char HARD_15_SHIP[] PROGMEM = "SOUTHERN CROSS";
const char HARD_15_NATURE[] PROGMEM = "STORM DAMAGE";
const char HARD_15_LAT_DEG[] PROGMEM = "56";
const char HARD_15_LAT_MIN[] PROGMEM = "00";
const char HARD_15_LON_DEG[] PROGMEM = "67";
const char HARD_15_LON_MIN[] PROGMEM = "30";
const char HARD_15_BRIEF[] PROGMEM = "A ship battles severe weather rounding Cape Horn.";
const char HARD_15_DEBRIEF[] PROGMEM = "Cape Horn is one of the most dangerous passages in the world.";

const char HARD_16_ID[] PROGMEM = "practice-030";
const char HARD_16_TITLE[] PROGMEM = "Gibraltar Collision";
const char HARD_16_MORSE[] PROGMEM = "SOS SOS SOS DE MEDITERRANEAN STAR COLLISION 36N00 05W30";
const char HARD_16_SIGNAL[] PROGMEM = "SOS";
const char HARD_16_SHIP[] PROGMEM = "MEDITERRANEAN STAR";
const char HARD_16_NATURE[] PROGMEM = "COLLISION";
const char HARD_16_LAT_DEG[] PROGMEM = "36";
const char HARD_16_LAT_MIN[] PROGMEM = "00";
const char HARD_16_LON_DEG[] PROGMEM = "05";
const char HARD_16_LON_MIN[] PROGMEM = "30";
const char HARD_16_BRIEF[] PROGMEM = "A collision in the busy Strait of Gibraltar.";
const char HARD_16_DEBRIEF[] PROGMEM = "High traffic areas require constant vigilance.";

const char HARD_17_ID[] PROGMEM = "practice-031";
const char HARD_17_TITLE[] PROGMEM = "Mozambique Channel";
const char HARD_17_MORSE[] PROGMEM = "SOS SOS SOS DE INDIAN OCEAN FIRE 18S30 41E15";
const char HARD_17_SIGNAL[] PROGMEM = "SOS";
const char HARD_17_SHIP[] PROGMEM = "INDIAN OCEAN";
const char HARD_17_NATURE[] PROGMEM = "FIRE";
const char HARD_17_LAT_DEG[] PROGMEM = "18";
const char HARD_17_LAT_MIN[] PROGMEM = "30";
const char HARD_17_LON_DEG[] PROGMEM = "41";
const char HARD_17_LON_MIN[] PROGMEM = "15";
const char HARD_17_BRIEF[] PROGMEM = "Fire aboard a tanker in the Mozambique Channel.";
const char HARD_17_DEBRIEF[] PROGMEM = "Tanker fires pose significant environmental risks.";

const char HARD_18_ID[] PROGMEM = "practice-032";
const char HARD_18_TITLE[] PROGMEM = "Arctic Passage";
const char HARD_18_MORSE[] PROGMEM = "SOS SOS SOS DE BERING STRAIT ICE DAMAGE 65N30 169W00";
const char HARD_18_SIGNAL[] PROGMEM = "SOS";
const char HARD_18_SHIP[] PROGMEM = "BERING STRAIT";
const char HARD_18_NATURE[] PROGMEM = "ICE DAMAGE";
const char HARD_18_LAT_DEG[] PROGMEM = "65";
const char HARD_18_LAT_MIN[] PROGMEM = "30";
const char HARD_18_LON_DEG[] PROGMEM = "169";
const char HARD_18_LON_MIN[] PROGMEM = "00";
const char HARD_18_BRIEF[] PROGMEM = "Ice damage to a vessel attempting the Northwest Passage.";
const char HARD_18_DEBRIEF[] PROGMEM = "Arctic navigation remains extremely hazardous.";

const char HARD_19_ID[] PROGMEM = "practice-033";
const char HARD_19_TITLE[] PROGMEM = "Channel Grounding";
const char HARD_19_MORSE[] PROGMEM = "SOS SOS SOS DE CHANNEL RUNNER GROUNDED 50N45 01W20";
const char HARD_19_SIGNAL[] PROGMEM = "SOS";
const char HARD_19_SHIP[] PROGMEM = "CHANNEL RUNNER";
const char HARD_19_NATURE[] PROGMEM = "GROUNDED";
const char HARD_19_LAT_DEG[] PROGMEM = "50";
const char HARD_19_LAT_MIN[] PROGMEM = "45";
const char HARD_19_LON_DEG[] PROGMEM = "01";
const char HARD_19_LON_MIN[] PROGMEM = "20";
const char HARD_19_BRIEF[] PROGMEM = "A ferry has run aground in the English Channel.";
const char HARD_19_DEBRIEF[] PROGMEM = "The English Channel is one of the busiest waterways in the world.";

const char HARD_20_ID[] PROGMEM = "practice-034";
const char HARD_20_TITLE[] PROGMEM = "Hurricane Season";
const char HARD_20_MORSE[] PROGMEM = "SOS SOS SOS DE CARIBBEAN SUN HURRICANE 18N30 64W45";
const char HARD_20_SIGNAL[] PROGMEM = "SOS";
const char HARD_20_SHIP[] PROGMEM = "CARIBBEAN SUN";
const char HARD_20_NATURE[] PROGMEM = "HURRICANE";
const char HARD_20_LAT_DEG[] PROGMEM = "18";
const char HARD_20_LAT_MIN[] PROGMEM = "30";
const char HARD_20_LON_DEG[] PROGMEM = "64";
const char HARD_20_LON_MIN[] PROGMEM = "45";
const char HARD_20_BRIEF[] PROGMEM = "A cruise ship caught in a hurricane in the Caribbean.";
const char HARD_20_DEBRIEF[] PROGMEM = "You've completed the Hard challenges!";

// ============================================
// Expert Challenges (Complex multi-part messages)
// Base: 150 points | Min 1.25x speed
// ============================================

// --- Expert 01: River of Shadows (Historical - Empress of Ireland 1914) ---
const char EXPERT_01_ID[] PROGMEM = "river-of-shadows";
const char EXPERT_01_TITLE[] PROGMEM = "River of Shadows";
const char EXPERT_01_MORSE[] PROGMEM = "CQD CQD CQD DE EMPRESS OF IRELAND COLLISION SINKING FAST 48N38 68W24";
const char EXPERT_01_SIGNAL[] PROGMEM = "CQD";
const char EXPERT_01_SHIP[] PROGMEM = "EMPRESS OF IRELAND";
const char EXPERT_01_NATURE[] PROGMEM = "COLLISION";
const char EXPERT_01_LAT_DEG[] PROGMEM = "48";
const char EXPERT_01_LAT_MIN[] PROGMEM = "38";
const char EXPERT_01_LON_DEG[] PROGMEM = "68";
const char EXPERT_01_LON_MIN[] PROGMEM = "24";
const char EXPERT_01_BRIEF[] PROGMEM = "Major disaster in the St. Lawrence. Complex distress with abbreviated formats.";
const char EXPERT_01_DEBRIEF[] PROGMEM = "Canada's worst maritime disaster - 1,012 died in just 14 minutes.";

// --- Expert 02: Urgent Plea (Practice) ---
const char EXPERT_02_ID[] PROGMEM = "practice-004";
const char EXPERT_02_TITLE[] PROGMEM = "Urgent Plea";
const char EXPERT_02_MORSE[] PROGMEM = "SOS SOS SOS DE EASTERN WIND FLOODING REQUIRE IMMEDIATE ASSISTANCE 35N18 14E42";
const char EXPERT_02_SIGNAL[] PROGMEM = "SOS";
const char EXPERT_02_SHIP[] PROGMEM = "EASTERN WIND";
const char EXPERT_02_NATURE[] PROGMEM = "FLOODING";
const char EXPERT_02_LAT_DEG[] PROGMEM = "35";
const char EXPERT_02_LAT_MIN[] PROGMEM = "18";
const char EXPERT_02_LON_DEG[] PROGMEM = "14";
const char EXPERT_02_LON_MIN[] PROGMEM = "42";
const char EXPERT_02_BRIEF[] PROGMEM = "Emergency flooding reported. Listen carefully for all details. Some information may be abbreviated.";
const char EXPERT_02_DEBRIEF[] PROGMEM = "Complex messages require careful attention to extract all critical information.";

// --- Expert 03: Hidden Danger (Historical - Britannic 1916) ---
const char EXPERT_03_ID[] PROGMEM = "hidden-danger";
const char EXPERT_03_TITLE[] PROGMEM = "Hidden Danger";
const char EXPERT_03_MORSE[] PROGMEM = "SOS SOS SOS DE BRITANNIC STRUCK MINE SINKING 37N42 24E17";
const char EXPERT_03_SIGNAL[] PROGMEM = "SOS";
const char EXPERT_03_SHIP[] PROGMEM = "BRITANNIC";
const char EXPERT_03_NATURE[] PROGMEM = "STRUCK MINE";
const char EXPERT_03_LAT_DEG[] PROGMEM = "37";
const char EXPERT_03_LAT_MIN[] PROGMEM = "42";
const char EXPERT_03_LON_DEG[] PROGMEM = "24";
const char EXPERT_03_LON_MIN[] PROGMEM = "17";
const char EXPERT_03_BRIEF[] PROGMEM = "The Titanic's sister ship, now a hospital ship, has struck a mine in the Aegean.";
const char EXPERT_03_DEBRIEF[] PROGMEM = "The Britannic was the largest ship lost in WWI. Only 30 of 1,066 died.";

// --- Expert 04: Breach Below (Practice) ---
const char EXPERT_04_ID[] PROGMEM = "practice-011";
const char EXPERT_04_TITLE[] PROGMEM = "Breach Below";
const char EXPERT_04_MORSE[] PROGMEM = "SOS SOS SOS DE MIDNIGHT SUN HULL BREACH FLOODING RAPIDLY 62N30 17W45";
const char EXPERT_04_SIGNAL[] PROGMEM = "SOS";
const char EXPERT_04_SHIP[] PROGMEM = "MIDNIGHT SUN";
const char EXPERT_04_NATURE[] PROGMEM = "HULL BREACH";
const char EXPERT_04_LAT_DEG[] PROGMEM = "62";
const char EXPERT_04_LAT_MIN[] PROGMEM = "30";
const char EXPERT_04_LON_DEG[] PROGMEM = "17";
const char EXPERT_04_LON_MIN[] PROGMEM = "45";
const char EXPERT_04_BRIEF[] PROGMEM = "A cruise ship is taking on water after hull damage. Complex distress with abbreviated formats.";
const char EXPERT_04_DEBRIEF[] PROGMEM = "Hull breaches in cold northern waters are extremely dangerous.";

// --- Expert 05: Armed Merchant Cruiser (Historical - Rawalpindi 1939) ---
const char EXPERT_05_ID[] PROGMEM = "armed-merchant";
const char EXPERT_05_TITLE[] PROGMEM = "Armed Merchant Cruiser";
const char EXPERT_05_MORSE[] PROGMEM = "SOS SOS SOS DE RAWALPINDI ENGAGING ENEMY VESSEL REQUIRE ASSISTANCE 63N40 11W40";
const char EXPERT_05_SIGNAL[] PROGMEM = "SOS";
const char EXPERT_05_SHIP[] PROGMEM = "RAWALPINDI";
const char EXPERT_05_NATURE[] PROGMEM = "ENGAGING ENEMY VESSEL";
const char EXPERT_05_LAT_DEG[] PROGMEM = "63";
const char EXPERT_05_LAT_MIN[] PROGMEM = "40";
const char EXPERT_05_LON_DEG[] PROGMEM = "11";
const char EXPERT_05_LON_MIN[] PROGMEM = "40";
const char EXPERT_05_BRIEF[] PROGMEM = "November 1939. A converted liner engages German battleships.";
const char EXPERT_05_DEBRIEF[] PROGMEM = "The Rawalpindi engaged the Scharnhorst and Gneisenau, buying time for a convoy.";

// --- Expert 06: Typhoon Fury (Historical - Toya Maru 1954) ---
const char EXPERT_06_ID[] PROGMEM = "typhoon-marie";
const char EXPERT_06_TITLE[] PROGMEM = "Typhoon Fury";
const char EXPERT_06_MORSE[] PROGMEM = "SOS SOS SOS DE TOYA MARU TYPHOON CAPSIZING 41N46 140E40";
const char EXPERT_06_SIGNAL[] PROGMEM = "SOS";
const char EXPERT_06_SHIP[] PROGMEM = "TOYA MARU";
const char EXPERT_06_NATURE[] PROGMEM = "TYPHOON CAPSIZING";
const char EXPERT_06_LAT_DEG[] PROGMEM = "41";
const char EXPERT_06_LAT_MIN[] PROGMEM = "46";
const char EXPERT_06_LON_DEG[] PROGMEM = "140";
const char EXPERT_06_LON_MIN[] PROGMEM = "40";
const char EXPERT_06_BRIEF[] PROGMEM = "September 1954. A Japanese ferry is caught in Typhoon Marie.";
const char EXPERT_06_DEBRIEF[] PROGMEM = "The Toya Maru disaster killed 1,153 people.";

// --- Expert 07: Philippine Seas (Historical - Dona Paz 1987) ---
const char EXPERT_07_ID[] PROGMEM = "philippine-tragedy";
const char EXPERT_07_TITLE[] PROGMEM = "Philippine Seas";
const char EXPERT_07_MORSE[] PROGMEM = "SOS SOS SOS DE DONA PAZ COLLISION FIRE SINKING 12N25 121E50";
const char EXPERT_07_SIGNAL[] PROGMEM = "SOS";
const char EXPERT_07_SHIP[] PROGMEM = "DONA PAZ";
const char EXPERT_07_NATURE[] PROGMEM = "COLLISION FIRE SINKING";
const char EXPERT_07_LAT_DEG[] PROGMEM = "12";
const char EXPERT_07_LAT_MIN[] PROGMEM = "25";
const char EXPERT_07_LON_DEG[] PROGMEM = "121";
const char EXPERT_07_LON_MIN[] PROGMEM = "50";
const char EXPERT_07_BRIEF[] PROGMEM = "December 1987. The deadliest peacetime maritime disaster.";
const char EXPERT_07_DEBRIEF[] PROGMEM = "Over 4,300 died when the Dona Paz collided with an oil tanker.";

// --- Expert 08: Baltic Catastrophe (Historical - Estonia 1994) ---
const char EXPERT_08_ID[] PROGMEM = "baltic-storm-1994";
const char EXPERT_08_TITLE[] PROGMEM = "Baltic Catastrophe";
const char EXPERT_08_MORSE[] PROGMEM = "SOS SOS SOS DE ESTONIA BOW DOOR FAILURE SINKING 59N22 21E42";
const char EXPERT_08_SIGNAL[] PROGMEM = "SOS";
const char EXPERT_08_SHIP[] PROGMEM = "ESTONIA";
const char EXPERT_08_NATURE[] PROGMEM = "BOW DOOR FAILURE SINKING";
const char EXPERT_08_LAT_DEG[] PROGMEM = "59";
const char EXPERT_08_LAT_MIN[] PROGMEM = "22";
const char EXPERT_08_LON_DEG[] PROGMEM = "21";
const char EXPERT_08_LON_MIN[] PROGMEM = "42";
const char EXPERT_08_BRIEF[] PROGMEM = "September 1994. A ferry sinks in the Baltic Sea with massive loss of life.";
const char EXPERT_08_DEBRIEF[] PROGMEM = "852 people died when the Estonia's bow door failed in heavy seas.";

// --- Expert 09: Ship of Gold (Historical - Central America 1857) ---
const char EXPERT_09_ID[] PROGMEM = "ship-of-gold";
const char EXPERT_09_TITLE[] PROGMEM = "Ship of Gold";
const char EXPERT_09_MORSE[] PROGMEM = "CQD CQD CQD DE CENTRAL AMERICA HURRICANE SINKING 31N25 77W10";
const char EXPERT_09_SIGNAL[] PROGMEM = "CQD";
const char EXPERT_09_SHIP[] PROGMEM = "CENTRAL AMERICA";
const char EXPERT_09_NATURE[] PROGMEM = "HURRICANE SINKING";
const char EXPERT_09_LAT_DEG[] PROGMEM = "31";
const char EXPERT_09_LAT_MIN[] PROGMEM = "25";
const char EXPERT_09_LON_DEG[] PROGMEM = "77";
const char EXPERT_09_LON_MIN[] PROGMEM = "10";
const char EXPERT_09_BRIEF[] PROGMEM = "September 1857. A steamship carrying California gold sinks in a hurricane.";
const char EXPERT_09_DEBRIEF[] PROGMEM = "The loss of gold contributed to the Panic of 1857.";

// --- Expert 10: Secret Cargo (Historical - Indianapolis 1945) ---
const char EXPERT_10_ID[] PROGMEM = "secret-mission";
const char EXPERT_10_TITLE[] PROGMEM = "Secret Cargo";
const char EXPERT_10_MORSE[] PROGMEM = "SOS SOS SOS DE INDIANAPOLIS TORPEDOED SURVIVORS IN WATER 12N02 134E48";
const char EXPERT_10_SIGNAL[] PROGMEM = "SOS";
const char EXPERT_10_SHIP[] PROGMEM = "INDIANAPOLIS";
const char EXPERT_10_NATURE[] PROGMEM = "TORPEDOED SURVIVORS IN WATER";
const char EXPERT_10_LAT_DEG[] PROGMEM = "12";
const char EXPERT_10_LAT_MIN[] PROGMEM = "02";
const char EXPERT_10_LON_DEG[] PROGMEM = "134";
const char EXPERT_10_LON_MIN[] PROGMEM = "48";
const char EXPERT_10_BRIEF[] PROGMEM = "July 1945. A cruiser returning from a secret mission is torpedoed.";
const char EXPERT_10_DEBRIEF[] PROGMEM = "The Indianapolis had just delivered atomic bomb components to Tinian.";

// ============================================
// Master Challenges (Highest difficulty)
// Base: 250 points | Min 1.5x speed
// ============================================

// --- Master 01: A Cold Night (Historical - Titanic 1912) ---
const char MASTER_01_ID[] PROGMEM = "cold-night";
const char MASTER_01_TITLE[] PROGMEM = "A Cold Night";
const char MASTER_01_MORSE[] PROGMEM = "CQD CQD CQD DE MGY TITANIC STRUCK ICEBERG REQUIRE IMMEDIATE ASSISTANCE 41N46 50W14";
const char MASTER_01_SIGNAL[] PROGMEM = "CQD";
const char MASTER_01_SHIP[] PROGMEM = "MGY TITANIC";
const char MASTER_01_NATURE[] PROGMEM = "STRUCK ICEBERG";
const char MASTER_01_LAT_DEG[] PROGMEM = "41";
const char MASTER_01_LAT_MIN[] PROGMEM = "46";
const char MASTER_01_LON_DEG[] PROGMEM = "50";
const char MASTER_01_LON_MIN[] PROGMEM = "14";
const char MASTER_01_BRIEF[] PROGMEM = "The most famous maritime disaster. Copy the original distress call from this legendary vessel.";
const char MASTER_01_DEBRIEF[] PROGMEM = "Phillips and Bride transmitted for over 2 hours. 710 were rescued by Carpathia.";

// --- Master 02: Enemy Below (Historical - Lusitania 1915) ---
const char MASTER_02_ID[] PROGMEM = "enemy-below";
const char MASTER_02_TITLE[] PROGMEM = "Enemy Below";
const char MASTER_02_MORSE[] PROGMEM = "SOS SOS SOS DE LUSITANIA TORPEDOED SINKING FAST 51N25 08W30 SEND ALL HELP";
const char MASTER_02_SIGNAL[] PROGMEM = "SOS";
const char MASTER_02_SHIP[] PROGMEM = "LUSITANIA";
const char MASTER_02_NATURE[] PROGMEM = "TORPEDOED";
const char MASTER_02_LAT_DEG[] PROGMEM = "51";
const char MASTER_02_LAT_MIN[] PROGMEM = "25";
const char MASTER_02_LON_DEG[] PROGMEM = "08";
const char MASTER_02_LON_MIN[] PROGMEM = "30";
const char MASTER_02_BRIEF[] PROGMEM = "Wartime disaster off the coast of Ireland. Copy the urgent distress call.";
const char MASTER_02_DEBRIEF[] PROGMEM = "1,198 died including 128 Americans. Helped bring US into WWI.";

// --- Master 03: Hunter Hunted (Historical - Carpathia 1918) ---
const char MASTER_03_ID[] PROGMEM = "hunter-hunted";
const char MASTER_03_TITLE[] PROGMEM = "Hunter Becomes Hunted";
const char MASTER_03_MORSE[] PROGMEM = "SOS SOS SOS DE CARPATHIA TORPEDOED BY U BOAT SINKING REQUIRE IMMEDIATE ASSISTANCE 49N25 10W30";
const char MASTER_03_SIGNAL[] PROGMEM = "SOS";
const char MASTER_03_SHIP[] PROGMEM = "CARPATHIA";
const char MASTER_03_NATURE[] PROGMEM = "TORPEDOED BY U BOAT";
const char MASTER_03_LAT_DEG[] PROGMEM = "49";
const char MASTER_03_LAT_MIN[] PROGMEM = "25";
const char MASTER_03_LON_DEG[] PROGMEM = "10";
const char MASTER_03_LON_MIN[] PROGMEM = "30";
const char MASTER_03_BRIEF[] PROGMEM = "A rescue ship known for heroism is now in mortal danger herself.";
const char MASTER_03_DEBRIEF[] PROGMEM = "The Carpathia that rescued Titanic survivors was sunk by U-55 in 1918.";

// --- Master 04: Perfect Storm (Practice) ---
const char MASTER_04_ID[] PROGMEM = "practice-012";
const char MASTER_04_TITLE[] PROGMEM = "Perfect Storm";
const char MASTER_04_MORSE[] PROGMEM = "SOS SOS SOS DE PACIFIC ENDEAVOR COLLISION FLOODING ENGINE ROOM ABANDON SHIP 23N15 162W48";
const char MASTER_04_SIGNAL[] PROGMEM = "SOS";
const char MASTER_04_SHIP[] PROGMEM = "PACIFIC ENDEAVOR";
const char MASTER_04_NATURE[] PROGMEM = "COLLISION";
const char MASTER_04_LAT_DEG[] PROGMEM = "23";
const char MASTER_04_LAT_MIN[] PROGMEM = "15";
const char MASTER_04_LON_DEG[] PROGMEM = "162";
const char MASTER_04_LON_MIN[] PROGMEM = "48";
const char MASTER_04_BRIEF[] PROGMEM = "A complex emergency with multiple hazards. Listen carefully for all details.";
const char MASTER_04_DEBRIEF[] PROGMEM = "Master-level transmissions require focus on the most critical information.";

// --- Master 05: Frozen Hell (Historical - Wilhelm Gustloff 1945) ---
const char MASTER_05_ID[] PROGMEM = "baltic-nightmare";
const char MASTER_05_TITLE[] PROGMEM = "Frozen Hell";
const char MASTER_05_MORSE[] PROGMEM = "SOS SOS SOS DE WILHELM GUSTLOFF TORPEDOED EVACUATE ALL PASSENGERS 55N07 17E25";
const char MASTER_05_SIGNAL[] PROGMEM = "SOS";
const char MASTER_05_SHIP[] PROGMEM = "WILHELM GUSTLOFF";
const char MASTER_05_NATURE[] PROGMEM = "TORPEDOED EVACUATE ALL PASSENGERS";
const char MASTER_05_LAT_DEG[] PROGMEM = "55";
const char MASTER_05_LAT_MIN[] PROGMEM = "07";
const char MASTER_05_LON_DEG[] PROGMEM = "17";
const char MASTER_05_LON_MIN[] PROGMEM = "25";
const char MASTER_05_BRIEF[] PROGMEM = "The deadliest maritime disaster in history. Copy the desperate distress.";
const char MASTER_05_DEBRIEF[] PROGMEM = "Over 9,000 died - mostly refugees fleeing the advancing Red Army.";

// --- Master 06: Convoy Destruction (Historical - Goya 1945) ---
const char MASTER_06_ID[] PROGMEM = "goya-disaster";
const char MASTER_06_TITLE[] PROGMEM = "Convoy Destruction";
const char MASTER_06_MORSE[] PROGMEM = "SOS SOS SOS DE GOYA TORPEDOED SINKING IMMEDIATELY 55N12 18E18";
const char MASTER_06_SIGNAL[] PROGMEM = "SOS";
const char MASTER_06_SHIP[] PROGMEM = "GOYA";
const char MASTER_06_NATURE[] PROGMEM = "TORPEDOED SINKING IMMEDIATELY";
const char MASTER_06_LAT_DEG[] PROGMEM = "55";
const char MASTER_06_LAT_MIN[] PROGMEM = "12";
const char MASTER_06_LON_DEG[] PROGMEM = "18";
const char MASTER_06_LON_MIN[] PROGMEM = "18";
const char MASTER_06_BRIEF[] PROGMEM = "April 1945. Another evacuation ship meets a tragic end.";
const char MASTER_06_DEBRIEF[] PROGMEM = "The Goya sank in 7 minutes with over 6,000 dead.";

// --- Master 07: Pride of the Fleet (Historical - Hood 1941) ---
const char MASTER_07_ID[] PROGMEM = "mighty-hood";
const char MASTER_07_TITLE[] PROGMEM = "Pride of the Fleet";
const char MASTER_07_MORSE[] PROGMEM = "SOS SOS SOS DE HOOD MAGAZINE EXPLOSION SINKING FAST 63N20 31W50";
const char MASTER_07_SIGNAL[] PROGMEM = "SOS";
const char MASTER_07_SHIP[] PROGMEM = "HOOD";
const char MASTER_07_NATURE[] PROGMEM = "MAGAZINE EXPLOSION SINKING FAST";
const char MASTER_07_LAT_DEG[] PROGMEM = "63";
const char MASTER_07_LAT_MIN[] PROGMEM = "20";
const char MASTER_07_LON_DEG[] PROGMEM = "31";
const char MASTER_07_LON_MIN[] PROGMEM = "50";
const char MASTER_07_BRIEF[] PROGMEM = "May 1941. The Royal Navy's pride explodes in battle with Bismarck.";
const char MASTER_07_DEBRIEF[] PROGMEM = "Only 3 of 1,418 crew survived when Hood's magazine exploded.";

// --- Master 08: River Inferno (Historical - Sultana 1865) ---
const char MASTER_08_ID[] PROGMEM = "sultana-disaster";
const char MASTER_08_TITLE[] PROGMEM = "River Inferno";
const char MASTER_08_MORSE[] PROGMEM = "CQD CQD CQD DE SULTANA BOILER EXPLOSION FIRE SINKING 35N08 90W04";
const char MASTER_08_SIGNAL[] PROGMEM = "CQD";
const char MASTER_08_SHIP[] PROGMEM = "SULTANA";
const char MASTER_08_NATURE[] PROGMEM = "BOILER EXPLOSION FIRE SINKING";
const char MASTER_08_LAT_DEG[] PROGMEM = "35";
const char MASTER_08_LAT_MIN[] PROGMEM = "08";
const char MASTER_08_LON_DEG[] PROGMEM = "90";
const char MASTER_08_LON_MIN[] PROGMEM = "04";
const char MASTER_08_BRIEF[] PROGMEM = "April 1865. The worst maritime disaster in American history.";
const char MASTER_08_DEBRIEF[] PROGMEM = "1,800+ died - mostly Union soldiers returning from Confederate prisons.";

// --- Master 09: Rogue Wave (Practice) ---
const char MASTER_09_ID[] PROGMEM = "practice-048";
const char MASTER_09_TITLE[] PROGMEM = "Rogue Wave";
const char MASTER_09_MORSE[] PROGMEM = "SOS SOS SOS DE POSEIDON STAR ROGUE WAVE CAPSIZED INVERTED 35S30 20E00";
const char MASTER_09_SIGNAL[] PROGMEM = "SOS";
const char MASTER_09_SHIP[] PROGMEM = "POSEIDON STAR";
const char MASTER_09_NATURE[] PROGMEM = "ROGUE WAVE CAPSIZED INVERTED";
const char MASTER_09_LAT_DEG[] PROGMEM = "35";
const char MASTER_09_LAT_MIN[] PROGMEM = "30";
const char MASTER_09_LON_DEG[] PROGMEM = "20";
const char MASTER_09_LON_MIN[] PROGMEM = "00";
const char MASTER_09_BRIEF[] PROGMEM = "A cruise ship struck by a massive rogue wave has capsized.";
const char MASTER_09_DEBRIEF[] PROGMEM = "You've mastered the most challenging distress calls!";

// --- Master 10: Ultimate Challenge (Practice) ---
const char MASTER_10_ID[] PROGMEM = "practice-047";
const char MASTER_10_TITLE[] PROGMEM = "Ultimate Challenge";
const char MASTER_10_MORSE[] PROGMEM = "SOS SOS SOS DE FINAL VOYAGE MULTIPLE EMERGENCIES CRITICAL REQUIRE ALL ASSISTANCE 40N00 30W00";
const char MASTER_10_SIGNAL[] PROGMEM = "SOS";
const char MASTER_10_SHIP[] PROGMEM = "FINAL VOYAGE";
const char MASTER_10_NATURE[] PROGMEM = "MULTIPLE EMERGENCIES CRITICAL";
const char MASTER_10_LAT_DEG[] PROGMEM = "40";
const char MASTER_10_LAT_MIN[] PROGMEM = "00";
const char MASTER_10_LON_DEG[] PROGMEM = "30";
const char MASTER_10_LON_MIN[] PROGMEM = "00";
const char MASTER_10_BRIEF[] PROGMEM = "Multiple simultaneous emergencies aboard a vessel. The most complex distress call.";
const char MASTER_10_DEBRIEF[] PROGMEM = "Congratulations! You've completed all Master challenges!";

// ============================================
// Challenge Array (80 challenges total)
// 20 Easy + 20 Medium + 20 Hard + 10 Expert + 10 Master
// ============================================

// Helper macro for Easy challenges (no nature, no position)
#define EASY_CHALLENGE(n) { EASY_##n##_ID, EASY_##n##_TITLE, SPARK_EASY, \
    EASY_##n##_MORSE, EASY_##n##_SIGNAL, EASY_##n##_SHIP, \
    nullptr, nullptr, nullptr, 0, nullptr, nullptr, 0, \
    EASY_##n##_BRIEF, EASY_##n##_DEBRIEF, nullptr, 0, 0 }

// Helper macro for Medium challenges (has nature, no position)
#define MED_CHALLENGE(n) { MED_##n##_ID, MED_##n##_TITLE, SPARK_MEDIUM, \
    MED_##n##_MORSE, MED_##n##_SIGNAL, MED_##n##_SHIP, \
    MED_##n##_NATURE, nullptr, nullptr, 0, nullptr, nullptr, 0, \
    MED_##n##_BRIEF, MED_##n##_DEBRIEF, nullptr, 0, 0 }

// Helper macro for Hard/Expert/Master challenges (has nature and position)
#define HARD_CHALLENGE_N(n, latD, lonD) { HARD_##n##_ID, HARD_##n##_TITLE, SPARK_HARD, \
    HARD_##n##_MORSE, HARD_##n##_SIGNAL, HARD_##n##_SHIP, \
    HARD_##n##_NATURE, HARD_##n##_LAT_DEG, HARD_##n##_LAT_MIN, latD, \
    HARD_##n##_LON_DEG, HARD_##n##_LON_MIN, lonD, \
    HARD_##n##_BRIEF, HARD_##n##_DEBRIEF, nullptr, 0, 0 }

#define EXPERT_CHALLENGE_N(n, latD, lonD) { EXPERT_##n##_ID, EXPERT_##n##_TITLE, SPARK_EXPERT, \
    EXPERT_##n##_MORSE, EXPERT_##n##_SIGNAL, EXPERT_##n##_SHIP, \
    EXPERT_##n##_NATURE, EXPERT_##n##_LAT_DEG, EXPERT_##n##_LAT_MIN, latD, \
    EXPERT_##n##_LON_DEG, EXPERT_##n##_LON_MIN, lonD, \
    EXPERT_##n##_BRIEF, EXPERT_##n##_DEBRIEF, nullptr, 0, 0 }

#define MASTER_CHALLENGE_N(n, latD, lonD) { MASTER_##n##_ID, MASTER_##n##_TITLE, SPARK_MASTER, \
    MASTER_##n##_MORSE, MASTER_##n##_SIGNAL, MASTER_##n##_SHIP, \
    MASTER_##n##_NATURE, MASTER_##n##_LAT_DEG, MASTER_##n##_LAT_MIN, latD, \
    MASTER_##n##_LON_DEG, MASTER_##n##_LON_MIN, lonD, \
    MASTER_##n##_BRIEF, MASTER_##n##_DEBRIEF, nullptr, 0, 0 }

const SparkWatchChallenge sparkChallenges[] = {
    // ===== EASY CHALLENGES (20) =====
    EASY_CHALLENGE(01), EASY_CHALLENGE(02), EASY_CHALLENGE(03), EASY_CHALLENGE(04), EASY_CHALLENGE(05),
    EASY_CHALLENGE(06), EASY_CHALLENGE(07), EASY_CHALLENGE(08), EASY_CHALLENGE(09), EASY_CHALLENGE(10),
    EASY_CHALLENGE(11), EASY_CHALLENGE(12), EASY_CHALLENGE(13), EASY_CHALLENGE(14), EASY_CHALLENGE(15),
    EASY_CHALLENGE(16), EASY_CHALLENGE(17), EASY_CHALLENGE(18), EASY_CHALLENGE(19), EASY_CHALLENGE(20),

    // ===== MEDIUM CHALLENGES (20) =====
    MED_CHALLENGE(01), MED_CHALLENGE(02), MED_CHALLENGE(03), MED_CHALLENGE(04), MED_CHALLENGE(05),
    MED_CHALLENGE(06), MED_CHALLENGE(07), MED_CHALLENGE(08), MED_CHALLENGE(09), MED_CHALLENGE(10),
    MED_CHALLENGE(11), MED_CHALLENGE(12), MED_CHALLENGE(13), MED_CHALLENGE(14), MED_CHALLENGE(15),
    MED_CHALLENGE(16), MED_CHALLENGE(17), MED_CHALLENGE(18), MED_CHALLENGE(19), MED_CHALLENGE(20),

    // ===== HARD CHALLENGES (20) =====
    HARD_CHALLENGE_N(01, 'N', 'W'), HARD_CHALLENGE_N(02, 'N', 'W'), HARD_CHALLENGE_N(03, 'N', 'W'),
    HARD_CHALLENGE_N(04, 'N', 'W'), HARD_CHALLENGE_N(05, 'N', 'E'), HARD_CHALLENGE_N(06, 'N', 'W'),
    HARD_CHALLENGE_N(07, 'N', 'W'), HARD_CHALLENGE_N(08, 'N', 'W'), HARD_CHALLENGE_N(09, 'N', 'W'),
    HARD_CHALLENGE_N(10, 'N', 'E'), HARD_CHALLENGE_N(11, 'N', 'W'), HARD_CHALLENGE_N(12, 'N', 'E'),
    HARD_CHALLENGE_N(13, 'N', 'E'), HARD_CHALLENGE_N(14, 'N', 'W'), HARD_CHALLENGE_N(15, 'S', 'W'),
    HARD_CHALLENGE_N(16, 'N', 'W'), HARD_CHALLENGE_N(17, 'S', 'E'), HARD_CHALLENGE_N(18, 'N', 'W'),
    HARD_CHALLENGE_N(19, 'N', 'W'), HARD_CHALLENGE_N(20, 'N', 'W'),

    // ===== EXPERT CHALLENGES (10) =====
    EXPERT_CHALLENGE_N(01, 'N', 'W'), EXPERT_CHALLENGE_N(02, 'N', 'E'), EXPERT_CHALLENGE_N(03, 'N', 'E'),
    EXPERT_CHALLENGE_N(04, 'N', 'W'), EXPERT_CHALLENGE_N(05, 'N', 'W'), EXPERT_CHALLENGE_N(06, 'N', 'E'),
    EXPERT_CHALLENGE_N(07, 'N', 'E'), EXPERT_CHALLENGE_N(08, 'N', 'E'), EXPERT_CHALLENGE_N(09, 'N', 'W'),
    EXPERT_CHALLENGE_N(10, 'N', 'E'),

    // ===== MASTER CHALLENGES (10) =====
    MASTER_CHALLENGE_N(01, 'N', 'W'), MASTER_CHALLENGE_N(02, 'N', 'W'), MASTER_CHALLENGE_N(03, 'N', 'W'),
    MASTER_CHALLENGE_N(04, 'N', 'W'), MASTER_CHALLENGE_N(05, 'N', 'E'), MASTER_CHALLENGE_N(06, 'N', 'E'),
    MASTER_CHALLENGE_N(07, 'N', 'W'), MASTER_CHALLENGE_N(08, 'N', 'W'), MASTER_CHALLENGE_N(09, 'S', 'E'),
    MASTER_CHALLENGE_N(10, 'N', 'W'),
};

const int SPARK_CHALLENGE_COUNT = sizeof(sparkChallenges) / sizeof(sparkChallenges[0]);

// ============================================
// Challenge Access Functions
// ============================================

// Get challenges filtered by difficulty
int getChallengesByDifficulty(SparkWatchDifficulty difficulty, const SparkWatchChallenge** results, int maxResults) {
    int count = 0;
    for (int i = 0; i < SPARK_CHALLENGE_COUNT && count < maxResults; i++) {
        if (sparkChallenges[i].difficulty == difficulty) {
            results[count++] = &sparkChallenges[i];
        }
    }
    return count;
}

// Get challenges for a specific campaign
int getChallengesByCampaign(int campaignId, const SparkWatchChallenge** results, int maxResults) {
    int count = 0;
    for (int i = 0; i < SPARK_CHALLENGE_COUNT && count < maxResults; i++) {
        if (sparkChallenges[i].campaignId == campaignId) {
            results[count++] = &sparkChallenges[i];
        }
    }
    return count;
}

// Get a specific campaign mission
const SparkWatchChallenge* getCampaignMission(int campaignId, int missionNumber) {
    for (int i = 0; i < SPARK_CHALLENGE_COUNT; i++) {
        if (sparkChallenges[i].campaignId == campaignId &&
            sparkChallenges[i].missionNumber == missionNumber) {
            return &sparkChallenges[i];
        }
    }
    return nullptr;
}

// Get challenge by index
const SparkWatchChallenge* getChallengeByIndex(int index) {
    if (index >= 0 && index < SPARK_CHALLENGE_COUNT) {
        return &sparkChallenges[index];
    }
    return nullptr;
}

// Get campaign by ID
const SparkWatchCampaign* getCampaignById(int campaignId) {
    for (int i = 0; i < SPARK_CAMPAIGN_COUNT; i++) {
        if (sparkCampaigns[i].id == campaignId) {
            return &sparkCampaigns[i];
        }
    }
    return nullptr;
}

#endif // GAME_SPARK_WATCH_DATA_H
