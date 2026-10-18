// ============================================================================
//  ParkNexa - Login & Access Module (menu-driven C++ console)
//
//  Vehicle owners of different types (Bike, Car, EV, Accessible) register with
//  a user id + password. Only after a successful login do they reach the portal.
//
//  Features : registration with validation, login with 3-attempt lockout,
//             salted password hashing, change password, profile view,
//             accounts saved to / loaded from "accounts.txt".
//  DSA      : unordered_map (user id -> account), vector, string processing.
//
//  NOTE: the hash (std::hash + salt) is fine for a college demo but is NOT
//  cryptographically secure. A real system should use bcrypt/argon2.
//  Password typing is not masked (masking is platform-specific).
//
//  Needs ParkNexa_Services.h in the same folder.
//  Build:  g++ -std=c++17 -O2 -o parknexa ParkNexa_Login.cpp
// ============================================================================
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "ParkNexa_Services.h"   // parking services module (also provides shared helpers)
using namespace std;
 
struct Account {
    string userId, salt, passHash;
    string name, plate;
    int vehicle;
    int failedAttempts = 0;  // in-memory only; resets when the program restarts
    bool locked = false;
};
 
// -------------------------------------------------------------- validation --
bool validUserId(const string& id, string& err) {
    if (id.size() < 4 || id.size() > 15) { err = "User id must be 4-15 characters."; return false; }
    for (char c : id)
        if (!isalnum((unsigned char)c) && c != '_') { err = "User id may contain only letters, digits and '_'."; return false; }
    return true;
}
bool validPassword(const string& p, string& err) {
    bool letter = false, digit = false;
    if (p.size() < 6) { err = "Password must be at least 6 characters."; return false; }
    if (p.find(' ') != string::npos || p.find('|') != string::npos) { err = "Password cannot contain spaces or '|'."; return false; }
    for (char c : p) { letter |= isalpha((unsigned char)c); digit |= isdigit((unsigned char)c); }
    if (!letter || !digit) { err = "Password must contain at least one letter and one digit."; return false; }
    return true;
}
bool cleanField(const string& s) { return s.find('|') == string::npos; }
 
// ---------------------------------------------------------------- security --
string makeSalt() {
    static const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    string s;
    for (int i = 0; i < 8; i++) s += chars[rand() % (sizeof(chars) - 1)];
    return s;
}
string hashPassword(const string& salt, const string& pass) {
    size_t h = hash<string>{}(salt + pass);
    stringstream ss;
    ss << hex << h;
    return ss.str();
}
 
// ================================================================ auth system
class AuthSystem {
    unordered_map<string, Account> accounts;  // key = lower-case user id
    const string file = "accounts.txt";
    static const int MAX_ATTEMPTS = 3;
 
    static string key(string id) {
        for (auto& c : id) c = tolower((unsigned char)c);
        return id;
    }
 
    void save() const {
        ofstream out(file);
        for (auto& kv : accounts) {
            const Account& a = kv.second;
            out << a.userId << '|' << a.salt << '|' << a.passHash << '|' << a.name << '|' << a.plate << '|' << a.vehicle << '\n';
        }
    }
 
    void load() {
        ifstream in(file);
        string line;
        while (getline(in, line)) {
            vector<string> f;
            string tok;
            stringstream ss(line);
            while (getline(ss, tok, '|')) f.push_back(tok);
            if (f.size() != 6) continue;
            Account a;
            a.userId = f[0]; a.salt = f[1]; a.passHash = f[2]; a.name = f[3]; a.plate = f[4];
            try { a.vehicle = stoi(f[5]); } catch (...) { continue; }
            accounts[key(a.userId)] = a;
        }
    }
 
    bool plateTaken(const string& plate) const {
        for (auto& kv : accounts)
            if (kv.second.plate == plate) return true;
        return false;
    }
 
public:
    AuthSystem() { load(); }
 
    // ---- registration ----
    void registerAccount() {
        cout << "\n--- New Registration ---\n";
        Account a;
        a.name = readLine("  Full name        : ");
        if (!cleanField(a.name)) { cout << "  Name cannot contain '|'.\n"; return; }
 
        cout << "  Vehicle type     : 1) Bike/Compact  2) Car  3) EV  4) Accessible (permit)\n";
        a.vehicle = readInt("  Choose (1-4)     : ", 1, 4);
 
        a.plate = upper(readLine("  Vehicle plate    : "));
        if (!cleanField(a.plate)) { cout << "  Plate cannot contain '|'.\n"; return; }
        if (plateTaken(a.plate)) { cout << "  This plate is already registered.\n"; return; }
 
        string err;
        while (true) {
            a.userId = readLine("  Choose user id   : ");
            if (!validUserId(a.userId, err)) { cout << "  " << err << "\n"; continue; }
            if (accounts.count(key(a.userId))) { cout << "  That user id is taken. Try another.\n"; continue; }
            break;
        }
        string pass;
        while (true) {
            pass = readLine("  Choose password  : ");
            if (!validPassword(pass, err)) { cout << "  " << err << "\n"; continue; }
            if (pass != readLine("  Confirm password : ")) { cout << "  Passwords do not match.\n"; continue; }
            break;
        }
        a.salt = makeSalt();
        a.passHash = hashPassword(a.salt, pass);
        accounts[key(a.userId)] = a;
        save();
        cout << "  Registration successful. You can now log in as '" << a.userId << "'.\n";
    }
 
    // ---- login: returns pointer to account on success, nullptr otherwise ----
    Account* login() {
        cout << "\n--- Login ---\n";
        string id = readLine("  User id  : ");
        string pass = readLine("  Password : ");
        auto it = accounts.find(key(id));
        // same message for unknown id and wrong password (does not reveal which ids exist)
        if (it == accounts.end()) { cout << "  Invalid user id or password.\n"; return nullptr; }
        Account& a = it->second;
        if (a.locked) { cout << "  Account is locked after too many failed attempts. Restart the program to retry.\n"; return nullptr; }
        if (hashPassword(a.salt, pass) == a.passHash) {
            a.failedAttempts = 0;
            return &a;
        }
        a.failedAttempts++;
        if (a.failedAttempts >= MAX_ATTEMPTS) {
            a.locked = true;
            cout << "  Invalid user id or password. Account locked.\n";
        } else {
            cout << "  Invalid user id or password. " << (MAX_ATTEMPTS - a.failedAttempts) << " attempt(s) left.\n";
        }
        return nullptr;
    }
 
    void changePassword(Account& a) {
        string old = readLine("  Current password : ");
        if (hashPassword(a.salt, old) != a.passHash) { cout << "  Current password is wrong.\n"; return; }
        string err, np;
        while (true) {
            np = readLine("  New password     : ");
            if (!validPassword(np, err)) { cout << "  " << err << "\n"; continue; }
            if (np != readLine("  Confirm password : ")) { cout << "  Passwords do not match.\n"; continue; }
            break;
        }
        a.salt = makeSalt();
        a.passHash = hashPassword(a.salt, np);
        save();
        cout << "  Password changed successfully.\n";
    }
};
 
// ============================================================ portal (after login)
void portal(AuthSystem& auth, Account& user) {
    cout << "\n==========================================\n"
         << "  Welcome to the ParkNexa Portal, " << user.name << "!\n"
         << "==========================================\n";
    while (true) {
        cout << "\n [Portal]  1. View my profile\n"
             << "           2. Parking services (find / reserve / cancel)\n"
             << "           3. Change password\n"
             << "           0. Logout\n";
        switch (readInt(" Choice: ", 0, 3)) {
            case 1:
                cout << "\n  User id : " << user.userId
                     << "\n  Name    : " << user.name
                     << "\n  Vehicle : " << vehicleName(user.vehicle) << " (" << user.plate << ")\n";
                break;
            case 2:
                runParkingServices(user.plate, user.name, user.vehicle);  // ParkNexa_Services.h
                break;
            case 3: auth.changePassword(user); break;
            case 0: cout << "  Logged out. Goodbye, " << user.name << "!\n"; return;
        }
    }
}
 
// ===================================================================== main ==
int main() {
    srand((unsigned)time(nullptr));
    AuthSystem auth;
    cout << "==========================================\n"
         << "   ParkNexa - Secure Access\n"
         << "==========================================\n";
    while (true) {
        cout << "\n 1. Login\n 2. Register new vehicle owner\n 0. Exit\n";
        switch (readInt(" Choice: ", 0, 2)) {
            case 1:
                if (Account* u = auth.login()) portal(auth, *u);
                break;
            case 2: auth.registerAccount(); break;
            case 0: cout << " Thank you for using ParkNexa!\n"; return 0;
        }
    }
}