// ============================================================================
//  AuthService.h
//  Thin Qt-friendly wrapper around the authentication logic.
//
//  Refactored from AuthSystem class in ParkNexa_Login.cpp (original project).
//  - All std::cin / std::cout removed.
//  - Methods return error strings instead of printing.
//  - Password hashing algorithm preserved exactly.
//  - File format (accounts.txt) preserved exactly.
// ============================================================================
#pragma once
#include "../core/AuthTypes.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class AuthService {
    std::unordered_map<std::string, Account> accounts;  // key = lower-case userId
    std::string accountsFile;
    static const int MAX_ATTEMPTS = 3;

    // ── Internal helpers ─────────────────────────────────────────────────────
    static std::string key(std::string id) {
        for (auto& c : id) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        return id;
    }

    void save() const {
        std::ofstream out(accountsFile);
        for (auto& kv : accounts) {
            const Account& a = kv.second;
            out << a.userId << '|' << a.salt << '|' << a.passHash << '|'
                << a.name << '|' << a.plate << '|' << a.vehicle << '\n';
        }
    }

    void load() {
        std::ifstream in(accountsFile);
        std::string line;
        while (std::getline(in, line)) {
            std::vector<std::string> f;
            std::string tok;
            std::istringstream ss(line);
            while (std::getline(ss, tok, '|')) f.push_back(tok);
            if (f.size() != 6) continue;
            Account a;
            a.userId   = f[0]; a.salt     = f[1]; a.passHash = f[2];
            a.name     = f[3]; a.plate    = f[4];
            try { a.vehicle = std::stoi(f[5]); } catch (...) { continue; }
            accounts[key(a.userId)] = a;
        }
    }

    bool plateTaken(const std::string& plate) const {
        for (auto& kv : accounts)
            if (kv.second.plate == plate) return true;
        return false;
    }

    // ── Security helpers (preserved from original) ─────────────────────────
    static std::string makeSalt() {
        static const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
        std::string s;
        for (int i = 0; i < 8; i++)
            s += chars[rand() % (sizeof(chars) - 1)];
        return s;
    }

    static std::string hashPassword(const std::string& salt,
                                    const std::string& pass) {
        size_t h = std::hash<std::string>{}(salt + pass);
        std::ostringstream ss;
        ss << std::hex << h;
        return ss.str();
    }

public:
    explicit AuthService(const std::string& filePath)
        : accountsFile(filePath) { load(); }

    // ── Validation (preserved from original) ──────────────────────────────
    static bool validUserId(const std::string& id, std::string& err) {
        if (id.size() < 4 || id.size() > 15) {
            err = "User ID must be 4-15 characters."; return false;
        }
        for (char c : id)
            if (!isalnum(static_cast<unsigned char>(c)) && c != '_') {
                err = "User ID may contain only letters, digits and '_'."; return false;
            }
        return true;
    }

    static bool validPassword(const std::string& p, std::string& err) {
        bool letter = false, digit = false;
        if (p.size() < 6) { err = "Password must be at least 6 characters."; return false; }
        if (p.find(' ') != std::string::npos || p.find('|') != std::string::npos) {
            err = "Password cannot contain spaces or '|'."; return false;
        }
        for (char c : p) {
            letter |= static_cast<bool>(isalpha(static_cast<unsigned char>(c)));
            digit  |= static_cast<bool>(isdigit(static_cast<unsigned char>(c)));
        }
        if (!letter || !digit) {
            err = "Password must contain at least one letter and one digit."; return false;
        }
        return true;
    }

    // ── Registration ─────────────────────────────────────────────────────────
    // Returns empty string on success, error message on failure
    std::string registerAccount(const std::string& userId,
                                const std::string& password,
                                const std::string& name,
                                const std::string& plate,
                                int vehicleType) {
        // Validate user id
        std::string err;
        if (!validUserId(userId, err)) return err;
        if (accounts.count(key(userId))) return "That user ID is already taken.";

        // Validate password
        if (!validPassword(password, err)) return err;

        // Validate name and plate
        if (name.empty() || name.find('|') != std::string::npos)
            return "Name cannot be empty or contain '|'.";
        std::string upperPlate = plate;
        for (auto& c : upperPlate)
            c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
        if (upperPlate.empty() || upperPlate.find('|') != std::string::npos)
            return "Plate cannot be empty or contain '|'.";
        if (plateTaken(upperPlate))
            return "This vehicle plate is already registered.";
        if (vehicleType < 1 || vehicleType > 4)
            return "Invalid vehicle type.";

        Account a;
        a.userId   = userId;
        a.name     = name;
        a.plate    = upperPlate;
        a.vehicle  = vehicleType;
        a.salt     = makeSalt();
        a.passHash = hashPassword(a.salt, password);
        accounts[key(userId)] = a;
        save();
        return "";
    }

    // ── Login ─────────────────────────────────────────────────────────────────
    // Returns nullptr on failure; updates failedAttempts / locked state
    // errMsg is set to a user-facing error on failure
    Account* login(const std::string& userId, const std::string& password,
                   std::string& errMsg) {
        auto it = accounts.find(key(userId));
        if (it == accounts.end()) {
            errMsg = "Invalid user ID or password."; return nullptr;
        }
        Account& a = it->second;
        if (a.locked) {
            errMsg = "Account is locked after too many failed attempts.";
            return nullptr;
        }
        if (hashPassword(a.salt, password) == a.passHash) {
            a.failedAttempts = 0;
            return &a;
        }
        a.failedAttempts++;
        if (a.failedAttempts >= MAX_ATTEMPTS) {
            a.locked = true;
            errMsg   = "Invalid credentials. Account is now locked.";
        } else {
            int left = MAX_ATTEMPTS - a.failedAttempts;
            errMsg = "Invalid user ID or password. "
                   + std::to_string(left) + " attempt(s) remaining.";
        }
        return nullptr;
    }

    // ── Change password ───────────────────────────────────────────────────────
    std::string changePassword(const std::string& userId,
                               const std::string& oldPass,
                               const std::string& newPass) {
        auto it = accounts.find(key(userId));
        if (it == accounts.end()) return "Account not found.";
        Account& a = it->second;
        if (hashPassword(a.salt, oldPass) != a.passHash)
            return "Current password is incorrect.";
        std::string err;
        if (!validPassword(newPass, err)) return err;
        a.salt     = makeSalt();
        a.passHash = hashPassword(a.salt, newPass);
        save();
        return "";
    }

    // Reload accounts from disk (call after external changes)
    void reload() { accounts.clear(); load(); }

    // Get a copy of an account by userId (for profile display)
    bool getAccount(const std::string& userId, Account& out) const {
        auto it = accounts.find(key(userId));
        if (it == accounts.end()) return false;
        out = it->second;
        return true;
    }

    // All accounts list (for admin dashboard)
    std::vector<Account> getAllAccounts() const {
        std::vector<Account> out;
        for (auto& kv : accounts) out.push_back(kv.second);
        return out;
    }

    int accountCount() const { return static_cast<int>(accounts.size()); }
};
