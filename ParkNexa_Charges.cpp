#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <limits>
#include <cmath>
#include <algorithm>

using namespace std;

const double RATE_PER_HOUR = 50.0;
const char DELIMITER = '|';

struct Account {
    string username;
    string password;
    string fullName;
};

struct Vehicle {
    string username;
    string plate;
    string type;
};

struct ParkingSession {
    int id;
    string username;
    string plate;
    time_t entryTime;
    time_t exitTime; // 0 means the parking session is still active.
};

const string ACCOUNTS_FILE = "parknexa_accounts.txt";
const string VEHICLES_FILE = "parknexa_vehicles.txt";
const string SESSIONS_FILE = "parknexa_sessions.txt";

string trim(const string& text) {
    const string whitespace = " \t\r\n";
    size_t first = text.find_first_not_of(whitespace);
    if (first == string::npos) return "";
    size_t last = text.find_last_not_of(whitespace);
    return text.substr(first, last - first + 1);
}

string readLine(const string& prompt) {
    string value;
    cout << prompt;
    getline(cin, value);
    return trim(value);
}

bool containsDelimiter(const string& value) {
    return value.find(DELIMITER) != string::npos;
}

bool validField(const string& value) {
    return !value.empty() && !containsDelimiter(value);
}

int readInt(const string& prompt, int minimum, int maximum) {
    string line;
    int value;
    while (true) {
        cout << prompt;
        getline(cin, line);
        stringstream input(line);
        char extra;
        if (input >> value && !(input >> extra) &&
            value >= minimum && value <= maximum) {
            return value;
        }
        cout << "Invalid choice. Enter a number from "
             << minimum << " to " << maximum << ".\n";
    }
}

vector<string> splitLine(const string& line) {
    vector<string> fields;
    string field;
    stringstream input(line);
    while (getline(input, field, DELIMITER)) fields.push_back(field);
    return fields;
}

vector<Account> loadAccounts() {
    vector<Account> data;
    ifstream file(ACCOUNTS_FILE.c_str());
    string line;
    while (getline(file, line)) {
        vector<string> f = splitLine(line);
        if (f.size() == 3) {
            Account a;
            a.username = f[0];
            a.password = f[1];
            a.fullName = f[2];
            data.push_back(a);
        }
    }
    return data;
}

vector<Vehicle> loadVehicles() {
    vector<Vehicle> data;
    ifstream file(VEHICLES_FILE.c_str());
    string line;
    while (getline(file, line)) {
        vector<string> f = splitLine(line);
        if (f.size() == 3) {
            Vehicle v;
            v.username = f[0];
            v.plate = f[1];
            v.type = f[2];
            data.push_back(v);
        }
    }
    return data;
}

vector<ParkingSession> loadSessions() {
    vector<ParkingSession> data;
    ifstream file(SESSIONS_FILE.c_str());
    string line;
    while (getline(file, line)) {
        vector<string> f = splitLine(line);
        if (f.size() == 5) {
            ParkingSession s;
            try {
                s.id = stoi(f[0]);
                s.username = f[1];
                s.plate = f[2];
                s.entryTime = static_cast<time_t>(stoll(f[3]));
                s.exitTime = static_cast<time_t>(stoll(f[4]));
                data.push_back(s);
            } catch (...) {
                // Ignore malformed records rather than stopping the program.
            }
        }
    }
    return data;
}

bool saveAccounts(const vector<Account>& data) {
    ofstream file(ACCOUNTS_FILE.c_str());
    if (!file) return false;
    for (size_t i = 0; i < data.size(); ++i) {
        file << data[i].username << DELIMITER
             << data[i].password << DELIMITER
             << data[i].fullName << '\n';
    }
    return static_cast<bool>(file);
}

bool saveVehicles(const vector<Vehicle>& data) {
    ofstream file(VEHICLES_FILE.c_str());
    if (!file) return false;
    for (size_t i = 0; i < data.size(); ++i) {
        file << data[i].username << DELIMITER
             << data[i].plate << DELIMITER
             << data[i].type << '\n';
    }
    return static_cast<bool>(file);
}

bool saveSessions(const vector<ParkingSession>& data) {
    ofstream file(SESSIONS_FILE.c_str());
    if (!file) return false;
    for (size_t i = 0; i < data.size(); ++i) {
        file << data[i].id << DELIMITER
             << data[i].username << DELIMITER
             << data[i].plate << DELIMITER
             << static_cast<long long>(data[i].entryTime) << DELIMITER
             << static_cast<long long>(data[i].exitTime) << '\n';
    }
    return static_cast<bool>(file);
}

bool usernameExists(const string& username) {
    vector<Account> accounts = loadAccounts();
    for (size_t i = 0; i < accounts.size(); ++i) {
        if (accounts[i].username == username) return true;
    }
    return false;
}

bool plateExists(const string& plate) {
    vector<Vehicle> vehicles = loadVehicles();
    string normalized = plate;
    transform(normalized.begin(), normalized.end(), normalized.begin(),
              [](unsigned char c) { return static_cast<char>(toupper(c)); });
    for (size_t i = 0; i < vehicles.size(); ++i) {
        string existing = vehicles[i].plate;
        transform(existing.begin(), existing.end(), existing.begin(),
                  [](unsigned char c) { return static_cast<char>(toupper(c)); });
        if (existing == normalized) return true;
    }
    return false;
}

string formatTime(time_t value) {
    if (value == 0) return "Not ended";
    tm* local = localtime(&value);
    if (!local) return "Unknown time";
    stringstream output;
    output << put_time(local, "%d-%m-%Y %I:%M:%S %p");
    return output.str();
}

string chooseVehicleType() {
    cout << "\nVehicle type:\n"
         << "1. Bike / Scooter\n"
         << "2. Car\n"
         << "3. Electric Vehicle\n"
         << "4. Accessible Vehicle\n";
    int choice = readInt("Choose vehicle type: ", 1, 4);
    switch (choice) {
        case 1: return "Bike/Scooter";
        case 2: return "Car";
        case 3: return "Electric Vehicle";
        default: return "Accessible Vehicle";
    }
}

void registerAccount() {
    cout << "\n========== CREATE PARKNEXA ACCOUNT ==========\n";
    string fullName = readLine("Enter your full name: ");
    string username = readLine("Create a username: ");
    string password = readLine("Create a password: ");

    if (!validField(fullName) || !validField(username) || !validField(password)) {
        cout << "Fields cannot be empty or contain the character '|'. Try again.\n";
        return;
    }
    if (usernameExists(username)) {
        cout << "That username is already registered. Please log in or choose another.\n";
        return;
    }

    vector<Account> accounts = loadAccounts();
    Account account;
    account.username = username;
    account.password = password;
    account.fullName = fullName;
    accounts.push_back(account);

    if (!saveAccounts(accounts)) {
        cout << "Could not save account. Check folder permissions.\n";
        return;
    }

    cout << "\nAccount created successfully, " << fullName << "!\n";
    cout << "You can now log in and add your vehicles.\n";
}

bool login(Account& loggedIn) {
    cout << "\n============== PARKNEXA LOGIN ==============\n";
    string username = readLine("Username: ");
    string password = readLine("Password: ");

    vector<Account> accounts = loadAccounts();
    for (size_t i = 0; i < accounts.size(); ++i) {
        if (accounts[i].username == username &&
            accounts[i].password == password) {
            loggedIn = accounts[i];
            cout << "\nLogin successful. Welcome, " << loggedIn.fullName << "!\n";
            return true;
        }
    }
    cout << "Incorrect username or password.\n";
    return false;
}

void addVehicle(const Account& account) {
    cout << "\n============== ADD A VEHICLE ==============\n";
    string plate = readLine("Enter vehicle registration number: ");
    if (!validField(plate)) {
        cout << "Vehicle number cannot be empty or contain '|'.\n";
        return;
    }
    transform(plate.begin(), plate.end(), plate.begin(),
              [](unsigned char c) { return static_cast<char>(toupper(c)); });

    if (plateExists(plate)) {
        cout << "This vehicle number is already registered to an account.\n";
        return;
    }

    Vehicle vehicle;
    vehicle.username = account.username;
    vehicle.plate = plate;
    vehicle.type = chooseVehicleType();

    vector<Vehicle> vehicles = loadVehicles();
    vehicles.push_back(vehicle);
    if (saveVehicles(vehicles)) {
        cout << "Vehicle " << plate << " added successfully.\n";
    } else {
        cout << "Could not save vehicle. Check folder permissions.\n";
    }
}

vector<Vehicle> getUserVehicles(const string& username) {
    vector<Vehicle> all = loadVehicles();
    vector<Vehicle> mine;
    for (size_t i = 0; i < all.size(); ++i) {
        if (all[i].username == username) mine.push_back(all[i]);
    }
    return mine;
}

void listVehicles(const Account& account) {
    vector<Vehicle> vehicles = getUserVehicles(account.username);
    cout << "\n================ MY VEHICLES ================\n";
    if (vehicles.empty()) {
        cout << "No vehicles registered yet. Choose 'Add vehicle' first.\n";
        return;
    }
    for (size_t i = 0; i < vehicles.size(); ++i) {
        cout << i + 1 << ". " << vehicles[i].plate
             << " | " << vehicles[i].type << '\n';
    }
}

int nextSessionId(const vector<ParkingSession>& sessions) {
    int largest = 0;
    for (size_t i = 0; i < sessions.size(); ++i) {
        if (sessions[i].id > largest) largest = sessions[i].id;
    }
    return largest + 1;
}

bool vehicleHasActiveSession(const string& plate,
                             const vector<ParkingSession>& sessions) {
    for (size_t i = 0; i < sessions.size(); ++i) {
        if (sessions[i].plate == plate && sessions[i].exitTime == 0) return true;
    }
    return false;
}

void startParking(const Account& account) {
    vector<Vehicle> vehicles = getUserVehicles(account.username);
    if (vehicles.empty()) {
        cout << "Add a vehicle to your account before starting parking.\n";
        return;
    }

    cout << "\n=========== START PARKING SESSION ===========\n";
    for (size_t i = 0; i < vehicles.size(); ++i) {
        cout << i + 1 << ". " << vehicles[i].plate
             << " (" << vehicles[i].type << ")\n";
    }
    int choice = readInt("Choose vehicle: ", 1, static_cast<int>(vehicles.size()));
    const Vehicle& vehicle = vehicles[choice - 1];

    vector<ParkingSession> sessions = loadSessions();
    if (vehicleHasActiveSession(vehicle.plate, sessions)) {
        cout << "This vehicle already has an active parking session.\n";
        return;
    }

    ParkingSession session;
    session.id = nextSessionId(sessions);
    session.username = account.username;
    session.plate = vehicle.plate;
    session.entryTime = time(NULL);
    session.exitTime = 0;
    sessions.push_back(session);

    if (saveSessions(sessions)) {
        cout << "\nParking started successfully!\n";
        cout << "Vehicle: " << session.plate << '\n';
        cout << "Entry time: " << formatTime(session.entryTime) << '\n';
        cout << "Remember to select 'End parking and generate bill' when leaving.\n";
    } else {
        cout << "Could not save parking session. Check folder permissions.\n";
    }
}

void endParking(const Account& account) {
    vector<ParkingSession> sessions = loadSessions();
    vector<size_t> activeIndices;

    cout << "\n========== END PARKING / GENERATE BILL ==========\n";
    for (size_t i = 0; i < sessions.size(); ++i) {
        if (sessions[i].username == account.username && sessions[i].exitTime == 0) {
            activeIndices.push_back(i);
        }
    }

    if (activeIndices.empty()) {
        cout << "You do not have any active parking sessions.\n";
        return;
    }

    for (size_t i = 0; i < activeIndices.size(); ++i) {
        const ParkingSession& s = sessions[activeIndices[i]];
        cout << i + 1 << ". " << s.plate
             << " | Entry: " << formatTime(s.entryTime) << '\n';
    }

    int choice = readInt("Choose the vehicle leaving: ",
                         1, static_cast<int>(activeIndices.size()));
    ParkingSession& session = sessions[activeIndices[choice - 1]];
    session.exitTime = time(NULL);

    if (session.exitTime < session.entryTime) {
        session.exitTime = session.entryTime;
    }

    if (!saveSessions(sessions)) {
        cout << "Could not save exit time. No bill was finalized.\n";
        return;
    }

double elapsedSeconds = difftime(session.exitTime, session.entryTime);
double elapsedMinutes = elapsedSeconds / 60.0;

// Minimum charge: Rs. 25 for any parking duration up to 30 minutes.
// Beyond 30 minutes, charge Rs. 50 per hour, prorated by elapsed time.
double charge;
if (elapsedMinutes <= 30.0) {
    charge = 25.0;
} else {
    charge = (elapsedSeconds / 3600.0) * RATE_PER_HOUR;
}

    cout << fixed << setprecision(2);
    cout << "\n================================================\n";
    cout << "             PARKNEXA PARKING BILL             \n";
    cout << "================================================\n";
    cout << "Bill / Session ID:  " << session.id << '\n';
    cout << "Customer:           " << account.fullName << '\n';
    cout << "Vehicle number:     " << session.plate << '\n';
    cout << "Entry time:         " << formatTime(session.entryTime) << '\n';
    cout << "Exit time:          " << formatTime(session.exitTime) << '\n';
    cout << "Parking duration:   " << elapsedMinutes << " minute(s)\n";
    cout << "Rate:               Rs. " << RATE_PER_HOUR << " per hour\n";
    cout << "TOTAL CHARGES:      Rs. " << charge << '\n';
    cout << "================================================\n";
    cout << "Thank you for using ParkNexa!\n";
}

void viewHistory(const Account& account) {
    vector<ParkingSession> sessions = loadSessions();
    bool found = false;
    cout << "\n============== MY PARKING HISTORY ==============\n";
    for (size_t i = 0; i < sessions.size(); ++i) {
        const ParkingSession& s = sessions[i];
        if (s.username != account.username) continue;
        found = true;
        cout << "\nSession ID: " << s.id << '\n';
        cout << "Vehicle: " << s.plate << '\n';
        cout << "Entry: " << formatTime(s.entryTime) << '\n';
        cout << "Exit:  " << formatTime(s.exitTime) << '\n';
        if (s.exitTime == 0) {
            cout << "Status: ACTIVE\n";
        } else {
            double seconds = difftime(s.exitTime, s.entryTime);
            double minutes = seconds / 60.0;
            double charge;

            if (minutes <= 30.0) {
                charge = 25.0;
            } else {
                charge = (seconds / 3600.0) * RATE_PER_HOUR;
            }

            cout << fixed << setprecision(2);
            cout << "Duration: " << minutes << " minute(s)\n";
            cout << "Charges: Rs. " << charge << '\n';
        }
    }
    if (!found) cout << "No parking history found.\n";
}

void accountMenu(const Account& account) {
    while (true) {
        cout << "\n============== PARKNEXA CHARGES ==============\n";
        cout << "Logged in as: " << account.fullName
             << " (" << account.username << ")\n";
        cout << "1. Add vehicle\n";
        cout << "2. View my vehicles\n";
        cout << "3. Start parking (record entry time)\n";
        cout << "4. End parking and generate bill\n";
        cout << "5. View parking history\n";
        cout << "0. Log out\n";
        int choice = readInt("Choose an option: ", 0, 5);

        switch (choice) {
            case 1: addVehicle(account); break;
            case 2: listVehicles(account); break;
            case 3: startParking(account); break;
            case 4: endParking(account); break;
            case 5: viewHistory(account); break;
            case 0:
                cout << "Logged out successfully.\n";
                return;
        }
    }
}

int main() {
    cout << "=============================================\n";
    cout << "       PARKNEXA - PARKING CHARGES             \n";
    cout << "=============================================\n";

    while (true) {
        cout << "\n================= MAIN MENU =================\n";
        cout << "1. Register a new account\n";
        cout << "2. Login\n";
        cout << "0. Exit\n";
        int choice = readInt("Choose an option: ", 0, 2);

        if (choice == 1) {
            registerAccount();
        } else if (choice == 2) {
            Account account;
            if (login(account)) accountMenu(account);
        } else {
            cout << "Thank you for using ParkNexa Parking Charges.\n";
            break;
        }
    }
    return 0;
}
