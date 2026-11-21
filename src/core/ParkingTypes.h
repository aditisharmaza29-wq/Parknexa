// ============================================================================
//  ParkingTypes.h
//  Core data structures shared across the entire ParkNexa application.
//  These are plain C++ structs with no Qt or console dependencies.
//  Extracted directly from ParkNexa_Services.h (original project).
// ============================================================================
#pragma once
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Enumerations (same values as original project -- do not change)
// ---------------------------------------------------------------------------
enum VehicleType { BIKE = 1, CAR = 2, EV = 3, ACCESSIBLE = 4 };
enum SlotType    { S_COMPACT = 1, S_REGULAR = 2, S_EV = 3, S_ACCESS = 4 };

// ---------------------------------------------------------------------------
// Slot  -- one physical parking space in the mall
// ---------------------------------------------------------------------------
struct Slot {
    int  id       = 0;
    char area     = 'A';   // A, B or C (wing of the mall)
    int  type     = S_REGULAR;
    bool covered  = false;
    int  node     = 0;     // index in the mall graph
    int  dist     = 0;     // Dijkstra distance from entrance (metres)
    bool occupied = false;
    int  resId    = 0;     // 0 = free / walk-in; >0 = reservation ID
};

// ---------------------------------------------------------------------------
// Reservation  -- one booking record
// ---------------------------------------------------------------------------
struct Reservation {
    int         id     = 0;
    std::string plate;
    std::string owner;
    int         slotId = 0;
    std::string time;
    bool        active = false;
};

// ---------------------------------------------------------------------------
// Person  -- the logged-in user's identity passed to the parking engine
// ---------------------------------------------------------------------------
struct Person {
    std::string plate;
    std::string name;
    int         vehicle = BIKE;  // VehicleType
};

// ---------------------------------------------------------------------------
// Pref  -- user's slot preference (used by the ranking algorithm)
// ---------------------------------------------------------------------------
struct Pref {
    int  mode    = 3;      // 1=closest, 2=least-crowded, 3=balanced
    bool covered = false;
};

// ---------------------------------------------------------------------------
// Candidate  -- intermediate ranking result (score + index into slots[])
// ---------------------------------------------------------------------------
struct Candidate {
    double score    = 0.0;
    int    slotIdx  = 0;
};

// ---------------------------------------------------------------------------
// WaitEntry  -- one entry in the waiting queue
// ---------------------------------------------------------------------------
struct WaitEntry {
    Person who;
    Pref   pref;
};

// ---------------------------------------------------------------------------
// Record  -- one line in the reservation history (doubly-linked list node)
// ---------------------------------------------------------------------------
struct Record {
    int         resId  = 0;
    std::string plate;
    int         slotId = 0;
    std::string action;
    std::string time;
};

// ---------------------------------------------------------------------------
// Bill  -- result returned by the charges engine after a session ends
// ---------------------------------------------------------------------------
struct Bill {
    int         sessionId    = 0;
    std::string plate;
    std::string ownerName;
    long long   entryTime    = 0;   // Unix timestamp
    long long   exitTime     = 0;   // Unix timestamp
    double      durationMins = 0.0;
    double      ratePerHour  = 50.0;
    double      totalCharge  = 0.0;
};

// ---------------------------------------------------------------------------
// VehicleRecord  -- vehicle stored by the charges engine
// ---------------------------------------------------------------------------
struct VehicleRecord {
    std::string username;
    std::string plate;
    std::string type;       // "Bike/Scooter", "Car", "Electric Vehicle", "Accessible Vehicle"
};

// ---------------------------------------------------------------------------
// ParkingSession  -- one entry/exit record (charges engine)
// ---------------------------------------------------------------------------
struct ParkingSession {
    int         id        = 0;
    std::string username;
    std::string plate;
    long long   entryTime = 0;
    long long   exitTime  = 0;  // 0 = still active
};

// ---------------------------------------------------------------------------
// Helper: route path (list of node names returned by the engine)
// ---------------------------------------------------------------------------
using RoutePath = std::vector<std::string>;
