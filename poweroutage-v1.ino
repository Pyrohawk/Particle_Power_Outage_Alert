// Include Particle Device OS APIs
#include "Particle.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

FuelGauge fuel;
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 24 * 60 * 60 * 1000; // 24 hours in ms
int lastPowerSource = -1;
char deviceName[32] = "Unknown Board"; // Fallback name
bool nameRequested = false;

// 1. Define the memory structure
struct MemoryStructure {
    uint32_t magicNumber;
    char savedName[32];
};

MemoryStructure boardMemory;
const int EEPROM_ADDRESS = 10; // The "locker number" in memory we are using
const uint32_t MY_MAGIC_NUMBER = 0x1A2B3C4D; // Our secret code

// Forward declarations
int myStatusCheck(String command);
void sendNotification(String msg, String tag, int prio);

// Catches the device name from the cloud
void nameHandler(const char *topic, const char *data) {
    // Compare the new name from the cloud to our currently loaded name
    // strcmp returns 0 if the two strings are exactly identical
    if (strcmp(deviceName, data) != 0) {
        
        // They are different! Update our live variables
        strncpy(deviceName, data, sizeof(deviceName));
        strncpy(boardMemory.savedName, data, sizeof(boardMemory.savedName));
        boardMemory.magicNumber = MY_MAGIC_NUMBER;

        // Overwrite the EEPROM with the new struct
        // Note: We use EEPROM.put() to write, which pairs with your EEPROM.get()
        EEPROM.put(EEPROM_ADDRESS, boardMemory);

        Particle.publish("DEBUG", "New name saved to EEPROM!", PRIVATE);
    }
}

void setup() {
    // ---------------------------------------------------------
    // 1. CLOUD REGISTRATIONS (Always do these first!)
    // ---------------------------------------------------------
    // Ask the cloud for this device's human-readable name
    Particle.subscribe("particle/device/name", nameHandler);
    
    // Sync Name manually via the Console
    Particle.function("syncName", syncNameCommand);
    
    // Register the manual check function from ntfy
    Particle.function("statusUpdate", myStatusCheck);


    // ---------------------------------------------------------
    // 2. HARDWARE CALIBRATION
    // ---------------------------------------------------------
    delay(5000); // Fuel gauge calibration


    // ---------------------------------------------------------
    // 3. MEMORY CHECK (EEPROM)
    // ---------------------------------------------------------
    // Check the EEPROM for our saved data
    EEPROM.get(EEPROM_ADDRESS, boardMemory);
    
    if (boardMemory.magicNumber == MY_MAGIC_NUMBER) {
        // We found the magic number! The name is already saved.
        strncpy(deviceName, boardMemory.savedName, sizeof(deviceName));
        nameRequested = true; // Tell the loop we don't need to ask the cloud
        
        // Safety check: Only publish if the cloud connection is fully established
        if (Particle.connected()) {
            Particle.publish("DEBUG", "Name loaded from memory!", PRIVATE); 
        }
    } else {
        // Memory is empty or corrupted. We must ask the cloud.
        nameRequested = false; 
    }
}

void loop() {
    int powerSource = System.powerSource();
    float batterySoc = fuel.getNormalizedSoC();
    float batteryVolts = fuel.getVCell();

    // 1. ONE-TIME CLOUD REQUEST
    // You nailed this logic! It only asks once the cloud is connected.
    if (Particle.connected() && !nameRequested) {
        // Added the empty data string "" so the compiler knows PRIVATE is the flag
        Particle.publish("particle/device/name", "", PRIVATE);
        nameRequested = true; // Lock the gate so we don't ask again!
    }

    // 2. SET BASELINE ON BOOT
    // Establishes the initial state so we don't send false alerts on startup
    if (lastPowerSource == -1) {
        lastPowerSource = powerSource;
    }

    // 3. IMMEDIATE ALERT: Power Change
    if (powerSource != lastPowerSource && lastPowerSource != -1) {
        if (powerSource == POWER_SOURCE_BATTERY) {
            // Power Lost
            String msg = String::format("CRITICAL: Power Lost! Battery: %.1f%%", batterySoc);
            sendNotification(msg, "rotating_light", 5); // Priority 5
        } else {
            // Power Restored
            String msg = String::format("INFO: Power Restored. Battery: %.1f%%", batterySoc);
            sendNotification(msg, "zap", 3); // Priority 3
        }
        lastPowerSource = powerSource;
    }
    
    // 4. SCHEDULED ALERT: Heartbeat (Health Check)
    if (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL || lastHeartbeat == 0) {
        String msg = String::format("Heartbeat: System OK. Battery: %.1f%% (%.2fV)", batterySoc, batteryVolts);
        sendNotification(msg, "green_heart", 2); // Priority 2 (quiet)
        lastHeartbeat = millis();
    }

    // 5. BREATHER DELAY
    // Reduced from 10 seconds to 1 second so alerts trigger almost instantly
    delay(1000); 
}

// 3. MANUAL ALERT: Triggered from ntfy.sh button
int myStatusCheck(String command) {
    if (command == "check") {
        // 1. Get real-time battery stats
        float batterySoc = fuel.getNormalizedSoC();
        float batteryVolts = fuel.getVCell(); 
        
        // 2. Get real-time cellular signal strength
        CellularSignal sig = Cellular.RSSI();
        int cellSignal = (int)sig.getStrengthValue(); 
        
        // 3. Translate dBm into human-readable quality with emojis
        String sigQuality;
        if (cellSignal > -70) {
            sigQuality = "Excellent 🟢";
        } else if (cellSignal > -90) {
            sigQuality = "Good 🟡";
        } else if (cellSignal > -100) {
            sigQuality = "Fair 🟠";
        } else {
            sigQuality = "Poor 🔴";
        }
        
        // 4. Check the live power source
        int currentPower = System.powerSource();
        
        String msg;
        String tag;
        int priority;

        // 5. Format the alert based on the power state (added %s for the quality string)
        if (currentPower == POWER_SOURCE_BATTERY) {
            // Power is OFF (Running on Battery)
            msg = String::format("Manual Check: RUNNING ON BATTERY! Volts: %.2fV (%.1f%%) | Cell: %ddBm (%s)", batteryVolts, batterySoc, cellSignal, sigQuality.c_str());
            tag = "warning"; 
            priority = 4;    
        } else {
            // Power is ON (AC/USB)
            msg = String::format("Manual Check: System OK (AC Power). Volts: %.2fV (%.1f%%) | Cell: %ddBm (%s)", batteryVolts, batterySoc, cellSignal, sigQuality.c_str());
            tag = "mag_right"; 
            priority = 3;      
        }

        // 6. Send the dynamically built notification
        sendNotification(msg, tag, priority);
        
        return 1; 
    }
    return -1;
}

// Sync Name with Cloud
int syncNameCommand(String command) {
    // This asks the Particle Cloud to send the device's current name
    Particle.publish("particle/device/name");
    return 1;
}

// --- HELPER FUNCTION: Packages everything into a neat JSON string ---
void sendNotification(String msg, String tag, int priority) {
    // Create a buffer large enough to hold the JSON shell plus your variables
    char payload[256]; 
    
    // We can hardcode the board name here, or pass it as another variable!
    String boardName = deviceName; 

    // Pack the passed variables into the JSON format Particle Webhooks expects
    // Note: We use .c_str() to convert Particle Strings into standard C-strings for snprintf
    snprintf(payload, sizeof(payload), 
             "{\"board\":\"%s\",\"tag\":\"%s\",\"prio\":\"%d\",\"msg\":\"%s\"}", 
             boardName.c_str(), tag.c_str(), priority, msg.c_str());

    // Send the fully formatted JSON payload to the Particle Cloud
    Particle.publish("power_status", payload);
}

