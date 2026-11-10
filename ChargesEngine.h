// ============================================================================
//  ChargesEngine.h
//  Billing and parking session logic.
//
//  Refactored from ParkNexa_Charges.cpp (original project).
//  All std::cin / std::cout removed.
//  The billing formula is preserved exactly:
//    - Rs. 25 flat for <= 30 minutes
//    - Rs. 50/hour (prorated) beyond 30 minutes
// ============================================================================
#pragma once
#include "ParkingTypes.h"
#include <algorithm>
#include <ctime>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class ChargesEngine {
    std::string vehiclesFile;
    std::string sessionsFile;

    static const char DELIM = '|';
    static constexpr double RATE_PER_HOUR = 50.0;  // Rs. 50/hour (preserved)

    // ── File I/O helpers ─────────────────────────────────────────────────────
    static std::vector<std::string> split(const std::string& line) {
        std::vector<std::string> f;
        std::string tok;
        std::istringstream ss(line);
        while (std::getline(ss, tok, DELIM)) f.push_back(tok);
        return f;
    }

    std::vector<VehicleRecord> loadVehicles() const {
        std::vector<VehicleRecord> data;
        std::ifstream file(vehiclesFile);
        std::string line;
        while (std::getline(file, line)) {
            auto f = split(line);
            if (f.size() == 3)
                data.push_back({f[0], f[1], f[2]});
        }
        return data;
    }

    bool saveVehicles(const std::vector<VehicleRecord>& data) const {
        std::ofstream file(vehiclesFile);
        if (!file) return false;
        for (auto& v : data)
            file << v.username << DELIM << v.plate << DELIM << v.type << '\n';
        return static_cast<bool>(file);
    }

    std::vector<ParkingSession> loadSessions() const {
        std::vector<ParkingSession> data;
        std::ifstream file(sessionsFile);
        std::string line;
        while (std::getline(file, line)) {
            auto f = split(line);
            if (f.size() == 5) {
                try {
                    ParkingSession s;
                    s.id        = std::stoi(f[0]);
                    s.username  = f[1];
                    s.plate     = f[2];
                    s.entryTime = std::stoll(f[3]);
                    s.exitTime  = std::stoll(f[4]);
                    data.push_back(s);
                } catch (...) {}
            }
        }
        return data;
    }

    bool saveSessions(const std::vector<ParkingSession>& data) const {
        std::ofstream file(sessionsFile);
        if (!file) return false;
        for (auto& s : data)
            file << s.id << DELIM << s.username << DELIM << s.plate << DELIM
                 << s.entryTime << DELIM << s.exitTime << '\n';
        return static_cast<bool>(file);
    }

    static int nextId(const std::vector<ParkingSession>& sessions) {
        int largest = 0;
        for (auto& s : sessions) largest = std::max(largest, s.id);
        return largest + 1;
    }

    // ── Billing calculation (preserved exactly from original) ─────────────
    static double calcCharge(long long entryTime, long long exitTime) {
        double secs = static_cast<double>(exitTime - entryTime);
        double mins = secs / 60.0;
        if (mins <= 30.0) return 25.0;          // flat minimum
        return (secs / 3600.0) * RATE_PER_HOUR; // hourly prorated
    }

public:
    explicit ChargesEngine(const std::string& vehiclesPath,
                           const std::string& sessionsPath)
        : vehiclesFile(vehiclesPath), sessionsFile(sessionsPath) {}

    // ── Vehicle management ───────────────────────────────────────────────────
    std::vector<VehicleRecord> getVehiclesForUser(const std::string& username) const {
        auto all = loadVehicles();
        std::vector<VehicleRecord> mine;
        for (auto& v : all)
            if (v.username == username) mine.push_back(v);
        return mine;
    }

    bool plateExists(const std::string& plate) const {
        auto all = loadVehicles();
        std::string norm = plate;
        std::transform(norm.begin(), norm.end(), norm.begin(),
                       [](unsigned char c){ return static_cast<char>(toupper(c)); });
        for (auto& v : all) {
            std::string ex = v.plate;
            std::transform(ex.begin(), ex.end(), ex.begin(),
                           [](unsigned char c){ return static_cast<char>(toupper(c)); });
            if (ex == norm) return true;
        }
        return false;
    }

    // Returns empty string on success, error message on failure
    std::string addVehicle(const std::string& username,
                           const std::string& plate,
                           const std::string& type) {
        if (plate.empty() || type.empty())
            return "Vehicle plate and type cannot be empty.";
        if (plateExists(plate))
            return "This vehicle number is already registered.";
        auto all = loadVehicles();
        std::string norm = plate;
        std::transform(norm.begin(), norm.end(), norm.begin(),
                       [](unsigned char c){ return static_cast<char>(toupper(c)); });
        all.push_back({username, norm, type});
        if (!saveVehicles(all))
            return "Could not save vehicle. Check folder permissions.";
        return "";
    }

    // Returns empty string on success, error message on failure
    std::string removeVehicle(const std::string& username,
                              const std::string& plate) {
        auto all = loadVehicles();
        auto it = std::remove_if(all.begin(), all.end(), [&](const VehicleRecord& v){
            return v.username == username && v.plate == plate;
        });
        if (it == all.end()) return "Vehicle not found.";
        all.erase(it, all.end());
        if (!saveVehicles(all)) return "Could not save changes.";
        return "";
    }

    // ── Session management ───────────────────────────────────────────────────
    bool hasActiveSession(const std::string& plate) const {
        auto sessions = loadSessions();
        for (auto& s : sessions)
            if (s.plate == plate && s.exitTime == 0) return true;
        return false;
    }

    // Start a parking session; returns session id on success, -1 on failure
    int startSession(const std::string& username, const std::string& plate) {
        if (hasActiveSession(plate)) return -1;
        auto sessions = loadSessions();
        ParkingSession s;
        s.id        = nextId(sessions);
        s.username  = username;
        s.plate     = plate;
        s.entryTime = static_cast<long long>(time(nullptr));
        s.exitTime  = 0;
        sessions.push_back(s);
        return saveSessions(sessions) ? s.id : -1;
    }

    // End a session and compute bill; returns empty Bill on failure
    Bill endSession(const std::string& username, const std::string& plate,
                    const std::string& ownerName) {
        auto sessions = loadSessions();
        for (auto& s : sessions) {
            if (s.username == username && s.plate == plate && s.exitTime == 0) {
                s.exitTime = static_cast<long long>(time(nullptr));
                if (s.exitTime < s.entryTime) s.exitTime = s.entryTime;
                if (!saveSessions(sessions)) return {};
                Bill b;
                b.sessionId    = s.id;
                b.plate        = s.plate;
                b.ownerName    = ownerName;
                b.entryTime    = s.entryTime;
                b.exitTime     = s.exitTime;
                b.durationMins = static_cast<double>(s.exitTime - s.entryTime) / 60.0;
                b.ratePerHour  = RATE_PER_HOUR;
                b.totalCharge  = calcCharge(s.entryTime, s.exitTime);
                return b;
            }
        }
        return {};
    }

    // ── History ──────────────────────────────────────────────────────────────
    std::vector<ParkingSession> getSessionHistory(const std::string& username) const {
        auto sessions = loadSessions();
        std::vector<ParkingSession> mine;
        for (auto& s : sessions)
            if (s.username == username) mine.push_back(s);
        std::sort(mine.begin(), mine.end(),
            [](const ParkingSession& a, const ParkingSession& b){
                return a.id > b.id;
            });
        return mine;
    }

    // All active sessions across all users (admin dashboard view)
    std::vector<ParkingSession> getAllActiveSessions() const {
        auto sessions = loadSessions();
        std::vector<ParkingSession> active;
        for (auto& s : sessions)
            if (s.exitTime == 0) active.push_back(s);
        return active;
    }

    // Total revenue from all completed sessions
    double totalRevenue() const {
        auto sessions = loadSessions();
        double total = 0.0;
        for (auto& s : sessions)
            if (s.exitTime != 0)
                total += calcCharge(s.entryTime, s.exitTime);
        return total;
    }

    // Re-compute bill details for a historical session (for display)
    Bill getBillForSession(const ParkingSession& s,
                           const std::string& ownerName = "") const {
        if (s.exitTime == 0) return {};
        Bill b;
        b.sessionId    = s.id;
        b.plate        = s.plate;
        b.ownerName    = ownerName;
        b.entryTime    = s.entryTime;
        b.exitTime     = s.exitTime;
        b.durationMins = static_cast<double>(s.exitTime - s.entryTime) / 60.0;
        b.ratePerHour  = RATE_PER_HOUR;
        b.totalCharge  = calcCharge(s.entryTime, s.exitTime);
        return b;
    }

    double getRatePerHour() const { return RATE_PER_HOUR; }
};
