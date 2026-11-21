// ============================================================================
//  AuthTypes.h
//  Account data structure used by AuthService.
//  Extracted from ParkNexa_Login.cpp (original project).
// ============================================================================
#pragma once
#include <string>

struct Account {
    std::string userId;
    std::string salt;
    std::string passHash;
    std::string name;
    std::string plate;
    int         vehicle        = 1;   // VehicleType int
    int         failedAttempts = 0;   // in-memory only
    bool        locked         = false;
};
