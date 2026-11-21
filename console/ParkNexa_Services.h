// ============================================================================
//  ParkNexa_Services.h  -  Parking services module
//
//  Attached to ParkNexa_Login.cpp. After a successful login, the portal calls
//      runParkingServices(plate, ownerName, vehicleType);
//  and the user gets every main ParkNexa feature, using the identity and
//  vehicle type already known from their account:
//
//    1. Find & reserve the best available slot   (scoring + priority_queue)
//    2. My reservation + shortest route          (graph + Dijkstra)
//    3. Cancel my reservation                    (frees slot, serves waiting list)
//    4. Parking status of all slots              (sorted by distance, area occupancy)
//    5. Waiting list (position / leave)          (queue)
//    6. My reservation history                   (doubly linked list, both directions)
//    7. Undo my last reservation                 (stack)
//
//  Data (reservations, history, waiting list) is saved to "parking_data.txt"
//  so it survives restarts. Sample mall = 9 slots. Live sensors / web
//  integration are future enhancements and are NOT part of this module.
// ============================================================================
#pragma once
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

// ============================ shared definitions & helpers ==================
enum VehicleType { BIKE = 1, CAR = 2, EV = 3, ACCESSIBLE = 4 };
enum SlotType { S_COMPACT = 1, S_REGULAR = 2, S_EV = 3, S_ACCESS = 4 };

inline string vehicleName(int v) {
    switch (v) {
        case BIKE: return "Bike/Compact";
        case CAR: return "Car";
        case EV: return "Electric Vehicle";
        default: return "Accessible (permit)";
    }
}
inline string slotTypeName(int t) {
    switch (t) {
        case S_COMPACT: return "Compact";
        case S_REGULAR: return "Regular";
        case S_EV: return "EV-Charging";
        default: return "Accessible";
    }
}
inline string upper(string s) {
    for (auto& c : s) c = toupper((unsigned char)c);
    return s;
}
inline string nowStr() {
    time_t t = time(nullptr);
    char b[32];
    strftime(b, sizeof b, "%d-%b-%Y %H:%M:%S", localtime(&t));
    return b;
}
inline string readLine(const string& prompt) {
    string s;
    while (true) {
        cout << prompt;
        if (!getline(cin, s)) exit(0);
        size_t a = s.find_first_not_of(" \t\r"), b = s.find_last_not_of(" \t\r");
        if (a != string::npos) return s.substr(a, b - a + 1);
        cout << "  Input cannot be empty.\n";
    }
}
inline int readInt(const string& prompt, int lo, int hi) {
    while (true) {
        string s = readLine(prompt);
        try {
            size_t pos;
            int v = stoi(s, &pos);
            if (pos == s.size() && v >= lo && v <= hi) return v;
        } catch (...) {}
        cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
}
inline vector<string> splitBar(const string& line) {
    vector<string> f;
    string tok;
    stringstream ss(line);
    while (getline(ss, tok, '|')) f.push_back(tok);
    return f;
}

// ================================ data models ===============================
struct Person {  // the logged-in user, as handed over by the login module
    string plate, name;
    int vehicle;
};
struct Pref {
    int mode = 3;          // 1 closest to entrance, 2 least crowded, 3 balanced
    bool covered = false;  // prefer covered parking
};
struct Slot {
    int id;
    char area;
    int type;
    bool covered;
    int node;
    int dist = 0;
    bool occupied = false;
    int resId = 0;  // 0 = free, or taken by a walk-in car
};
struct Reservation {
    int id;
    string plate, owner;
    int slotId;
    string time;
    bool active;
};
struct Candidate {
    double score;
    int slotIdx;
};
struct CandCmp {
    bool operator()(const Candidate& a, const Candidate& b) const { return a.score > b.score; }
};
struct WaitEntry {
    Person who;
    Pref pref;
};

// ------------------------------- doubly linked list: reservation history ----
struct Record {
    int resId;
    string plate;
    int slotId;
    string action, time;
};
struct HNode {
    Record rec;
    HNode *prev, *next;
    explicit HNode(const Record& r) : rec(r), prev(nullptr), next(nullptr) {}
};
class HistoryList {
    HNode *head = nullptr, *tail = nullptr;
public:
    HistoryList() = default;
    HistoryList(const HistoryList&) = delete;
    HistoryList& operator=(const HistoryList&) = delete;
    ~HistoryList() {
        while (head) { HNode* t = head; head = head->next; delete t; }
    }
    void add(const Record& r) {
        HNode* n = new HNode(r);
        if (!tail) head = tail = n;
        else { tail->next = n; n->prev = tail; tail = n; }
    }
    template <class F> void forEach(F f) const {
        for (HNode* c = head; c; c = c->next) f(c->rec);
    }
    // prints only this plate's records; returns how many were printed
    int printFor(const string& plate, bool forward) const {
        int n = 0;
        auto show = [&](const Record& r) {
            cout << "  " << left << setw(21) << r.time << setw(11) << r.action << "Res#" << setw(6) << r.resId
                 << "Slot S" << r.slotId << "\n";
            n++;
        };
        if (forward) { for (HNode* c = head; c; c = c->next) if (c->rec.plate == plate) show(c->rec); }
        else         { for (HNode* c = tail; c; c = c->prev) if (c->rec.plate == plate) show(c->rec); }
        return n;
    }
};

// ============================== parking service engine ======================
class ParkingService {
    // graph of the mall
    vector<vector<pair<int, int>>> adj;
    vector<string> nodeName;
    vector<int> parent, distFromEntrance;

    vector<Slot> slots;  // sorted by id -> binary search
    unordered_map<int, Reservation> reservations;
    unordered_map<string, stack<int>> undoStacks;  // one stack per vehicle
    queue<WaitEntry> waitQ;
    HistoryList history;
    int nextResId = 1001;
    const string file = "parking_data.txt";

    // ---------------------------------------------------------- graph ------
    void addEdge(int a, int b, int w) {
        adj[a].push_back({b, w});
        adj[b].push_back({a, w});
    }
    void dijkstra(int src) {
        int n = adj.size();
        distFromEntrance.assign(n, INT_MAX);
        parent.assign(n, -1);
        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq;
        distFromEntrance[src] = 0;
        pq.push({0, src});
        while (!pq.empty()) {
            auto [d, u] = pq.top();
            pq.pop();
            if (d > distFromEntrance[u]) continue;
            for (auto [v, w] : adj[u])
                if (d + w < distFromEntrance[v]) {
                    distFromEntrance[v] = d + w;
                    parent[v] = u;
                    pq.push({distFromEntrance[v], v});
                }
        }
    }
    void buildMall() {
        // nodes: 0 Entrance, 1-3 junctions (wings A,B,C), 4-12 slots S1..S9
        nodeName = {"Mall Entrance", "Junction A", "Junction B", "Junction C"};
        for (int i = 1; i <= 9; i++) nodeName.push_back("Slot S" + to_string(i));
        adj.assign(13, {});
        addEdge(0, 1, 20); addEdge(1, 2, 30); addEdge(1, 3, 30); addEdge(2, 3, 70);
        addEdge(1, 4, 10); addEdge(1, 5, 15); addEdge(1, 6, 20);     // area A
        addEdge(2, 7, 10); addEdge(2, 8, 15); addEdge(2, 9, 25);     // area B
        addEdge(3, 10, 12); addEdge(3, 11, 20); addEdge(3, 12, 30);  // area C
        addEdge(9, 12, 40);
        dijkstra(0);

        slots = {
            {1, 'A', S_ACCESS,  true,  4}, {2, 'A', S_REGULAR, true,  5}, {3, 'A', S_COMPACT, true,  6},
            {4, 'B', S_EV,      false, 7}, {5, 'B', S_REGULAR, false, 8}, {6, 'B', S_REGULAR, false, 9},
            {7, 'C', S_COMPACT, false, 10}, {8, 'C', S_REGULAR, false, 11}, {9, 'C', S_EV,     false, 12},
        };
        for (auto& s : slots) s.dist = distFromEntrance[s.node];
        slots[2].occupied = true;  // S3 taken by a walk-in car
        slots[5].occupied = true;  // S6 taken by a walk-in car
    }

    // ------------------------------------------------------- persistence ---
    void save() const {
        ofstream out(file);
        vector<int> ids;
        for (auto& kv : reservations) ids.push_back(kv.first);
        sort(ids.begin(), ids.end());
        for (int id : ids) {
            const Reservation& r = reservations.at(id);
            out << "R|" << r.id << '|' << r.plate << '|' << r.owner << '|' << r.slotId << '|' << r.time << '|' << r.active << '\n';
        }
        history.forEach([&](const Record& r) {
            out << "H|" << r.resId << '|' << r.plate << '|' << r.slotId << '|' << r.action << '|' << r.time << '\n';
        });
        queue<WaitEntry> c = waitQ;
        while (!c.empty()) {
            const WaitEntry& w = c.front();
            out << "W|" << w.who.plate << '|' << w.who.name << '|' << w.who.vehicle << '|' << w.pref.mode << '|' << w.pref.covered << '\n';
            c.pop();
        }
    }
    void load() {
        ifstream in(file);
        string line;
        vector<int> activeIds;
        while (getline(in, line)) {
            auto f = splitBar(line);
            try {
                if (f[0] == "R" && f.size() == 7) {
                    Reservation r{stoi(f[1]), f[2], f[3], stoi(f[4]), f[5], f[6] == "1"};
                    reservations[r.id] = r;
                    nextResId = max(nextResId, r.id + 1);
                    if (r.active) {
                        int idx = findSlotIdx(r.slotId);
                        if (idx >= 0) { slots[idx].occupied = true; slots[idx].resId = r.id; }
                        activeIds.push_back(r.id);
                    }
                } else if (f[0] == "H" && f.size() == 6) {
                    history.add({stoi(f[1]), f[2], stoi(f[3]), f[4], f[5]});
                } else if (f[0] == "W" && f.size() == 6) {
                    WaitEntry w;
                    w.who = {f[1], f[2], stoi(f[3])};
                    w.pref.mode = stoi(f[4]);
                    w.pref.covered = f[5] == "1";
                    waitQ.push(w);
                }
            } catch (...) { /* skip corrupt line */ }
        }
        sort(activeIds.begin(), activeIds.end());
        for (int id : activeIds) undoStacks[reservations[id].plate].push(id);
    }

    // ----------------------------------------------------------- lookups ---
    int findSlotIdx(int id) const {  // binary search by slot id
        int lo = 0, hi = (int)slots.size() - 1;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            if (slots[mid].id == id) return mid;
            if (slots[mid].id < id) lo = mid + 1;
            else hi = mid - 1;
        }
        return -1;
    }
    Reservation* activeOf(const string& plate) {
        for (auto& kv : reservations)
            if (kv.second.active && kv.second.plate == plate) return &kv.second;
        return nullptr;
    }
    double areaOccupancy(char area) const {
        int total = 0, occ = 0;
        for (auto& s : slots)
            if (s.area == area) { total++; if (s.occupied) occ++; }
        return total ? (double)occ / total : 0.0;
    }
    int waitPosition(const string& plate) const {
        queue<WaitEntry> c = waitQ;
        for (int pos = 1; !c.empty(); pos++, c.pop())
            if (c.front().who.plate == plate) return pos;
        return 0;
    }
    void removeFromWaitlist(const string& plate) {
        queue<WaitEntry> keep;
        while (!waitQ.empty()) {
            if (waitQ.front().who.plate != plate) keep.push(waitQ.front());
            waitQ.pop();
        }
        waitQ = keep;
    }

    // ------------------------------------------------------------ ranking --
    // returns penalty (can be negative = bonus) or 99 if the slot is not allowed
    static double typePenalty(int veh, int st) {
        switch (veh) {
            case BIKE:       return st == S_COMPACT ? 0.0 : st == S_REGULAR ? 0.15 : 99;
            case CAR:        return st == S_REGULAR ? 0.0 : 99;
            case EV:         return st == S_EV ? -0.10 : st == S_REGULAR ? 0.10 : 99;
            case ACCESSIBLE: return st == S_ACCESS ? -0.10 : st == S_REGULAR ? 0.05 : 99;
        }
        return 99;
    }
    // score (lower = better) = wd*normDistance + wo*areaOccupancy + typePenalty - coveredBonus
    priority_queue<Candidate, vector<Candidate>, CandCmp> rank(int vehicle, const Pref& p) const {
        double wd = 0.5, wo = 0.5;
        if (p.mode == 1) { wd = 0.8; wo = 0.2; }
        if (p.mode == 2) { wd = 0.2; wo = 0.8; }
        int maxD = 1;
        for (auto& s : slots) maxD = max(maxD, s.dist);
        priority_queue<Candidate, vector<Candidate>, CandCmp> pq;
        for (int i = 0; i < (int)slots.size(); i++) {
            const Slot& s = slots[i];
            if (s.occupied) continue;
            double pen = typePenalty(vehicle, s.type);
            if (pen > 50) continue;
            double score = wd * ((double)s.dist / maxD) + wo * areaOccupancy(s.area) + pen;
            if (p.covered && s.covered) score -= 0.15;
            pq.push({score, i});
        }
        return pq;
    }

    // -------------------------------------------------------- core actions -
    int createReservation(const Person& p, int slotIdx) {
        Slot& s = slots[slotIdx];
        int rid = nextResId++;
        s.occupied = true;
        s.resId = rid;
        reservations[rid] = {rid, p.plate, p.name, s.id, nowStr(), true};
        undoStacks[p.plate].push(rid);
        history.add({rid, p.plate, s.id, "RESERVED", nowStr()});
        removeFromWaitlist(p.plate);
        return rid;
    }
    void printRoute(int slotIdx) const {
        const Slot& s = slots[slotIdx];
        vector<int> path;
        for (int v = s.node; v != -1; v = parent[v]) path.push_back(v);
        reverse(path.begin(), path.end());
        cout << "  Route: ";
        for (size_t i = 0; i < path.size(); i++) cout << nodeName[path[i]] << (i + 1 < path.size() ? " -> " : "");
        cout << "\n  Total distance from entrance: " << s.dist << " m\n";
    }
    void processWaitingList() {
        size_t n = waitQ.size();
        for (size_t i = 0; i < n; i++) {
            WaitEntry w = waitQ.front();
            waitQ.pop();
            auto pq = rank(w.who.vehicle, w.pref);
            if (pq.empty()) { waitQ.push(w); continue; }
            int idx = pq.top().slotIdx;
            int rid = createReservation(w.who, idx);
            cout << "  [WAITING LIST] Slot S" << slots[idx].id << " was auto-assigned to vehicle " << w.who.plate
                 << " (reservation #" << rid << ").\n";
        }
    }
    bool cancelReservation(int rid, const string& action) {
        auto it = reservations.find(rid);
        if (it == reservations.end() || !it->second.active) return false;
        Reservation& r = it->second;
        r.active = false;
        int idx = findSlotIdx(r.slotId);
        slots[idx].occupied = false;
        slots[idx].resId = 0;
        history.add({rid, r.plate, r.slotId, action, nowStr()});
        cout << "  Reservation #" << rid << " " << action << ". Slot S" << r.slotId << " is free again.\n";
        processWaitingList();
        return true;
    }

public:
    ParkingService() {
        buildMall();
        load();
    }

    // ------------------------------------------------------ menu actions ---
    void findAndReserve(const Person& p) {
        if (Reservation* r = activeOf(p.plate)) {
            cout << "  You already hold reservation #" << r->id << " (slot S" << r->slotId << "). Cancel it first to book another.\n";
            return;
        }
        if (int pos = waitPosition(p.plate)) {
            cout << "  You are already on the waiting list (position " << pos << "). You will be assigned automatically when a suitable slot frees up.\n";
            return;
        }
        Pref pref;
        cout << "  Preference: 1) Closest to entrance  2) Least crowded area  3) Balanced\n";
        pref.mode = readInt("  Choose (1-3): ", 1, 3);
        pref.covered = readInt("  Prefer covered parking? (1 = yes, 2 = no): ", 1, 2) == 1;

        auto pq = rank(p.vehicle, pref);
        if (pq.empty()) {
            cout << "  No suitable slot is free for a " << vehicleName(p.vehicle) << " right now.\n";
            if (readInt("  Join the waiting list? (1 = yes, 2 = no): ", 1, 2) == 1) {
                waitQ.push({p, pref});
                save();
                cout << "  Added to the waiting list at position " << waitPosition(p.plate) << ".\n";
            }
            return;
        }
        vector<Candidate> top;
        while (!pq.empty() && top.size() < 3) { top.push_back(pq.top()); pq.pop(); }

        cout << "\n  Recommended slots for your " << vehicleName(p.vehicle) << " (best first):\n";
        cout << "  #  Slot  Area  Type         Cover  Dist(m)  Score\n";
        for (size_t i = 0; i < top.size(); i++) {
            const Slot& s = slots[top[i].slotIdx];
            cout << "  " << i + 1 << "  S" << left << setw(4) << s.id << setw(6) << s.area << setw(13) << slotTypeName(s.type)
                 << setw(7) << (s.covered ? "Yes" : "No") << setw(9) << s.dist << fixed << setprecision(3) << top[i].score << "\n";
        }
        int ch = readInt("  Reserve which? (1-" + to_string(top.size()) + ", 0 = cancel): ", 0, (int)top.size());
        if (ch == 0) return;
        int idx = top[ch - 1].slotIdx;
        int rid = createReservation(p, idx);
        save();
        cout << "\n  Reservation #" << rid << " confirmed: slot S" << slots[idx].id << " for " << p.plate << ".\n";
        printRoute(idx);
    }

    void showMyReservation(const Person& p) {
        Reservation* r = activeOf(p.plate);
        if (!r) {
            if (int pos = waitPosition(p.plate)) cout << "  No active reservation. You are on the waiting list (position " << pos << ").\n";
            else cout << "  You have no active reservation.\n";
            return;
        }
        int idx = findSlotIdx(r->slotId);
        const Slot& s = slots[idx];
        cout << "  Reservation #" << r->id << " | Slot S" << s.id << " (Area " << s.area << ", " << slotTypeName(s.type)
             << (s.covered ? ", covered" : "") << ") | booked " << r->time << "\n";
        printRoute(idx);
    }

    void cancelMine(const Person& p) {
        Reservation* r = activeOf(p.plate);
        if (!r) { cout << "  You have no active reservation to cancel.\n"; return; }
        if (readInt("  Cancel reservation #" + to_string(r->id) + "? (1 = yes, 2 = no): ", 1, 2) != 1) return;
        cancelReservation(r->id, "CANCELLED");
        save();
    }

    void showStatus(const Person& p) {
        cout << "\n  Area occupancy: ";
        for (char a : {'A', 'B', 'C'}) cout << a << "=" << (int)(areaOccupancy(a) * 100) << "%  ";
        int freeCount = 0;
        for (auto& s : slots) if (!s.occupied) freeCount++;
        cout << "| Free slots: " << freeCount << "/" << slots.size() << "\n";
        cout << "\n  Slot  Area  Type         Cover  Dist(m)  Status\n";
        vector<Slot> sorted = slots;
        sort(sorted.begin(), sorted.end(), [](const Slot& a, const Slot& b) { return a.dist < b.dist; });
        for (auto& s : sorted) {
            string st = "FREE";
            if (s.occupied) {
                if (!s.resId) st = "OCCUPIED";
                else st = reservations[s.resId].plate == p.plate ? "RESERVED - YOURS (#" + to_string(s.resId) + ")" : "RESERVED";
            }
            cout << "  S" << left << setw(4) << s.id << setw(6) << s.area << setw(13) << slotTypeName(s.type)
                 << setw(7) << (s.covered ? "Yes" : "No") << setw(9) << s.dist << st << "\n";
        }
    }

    void waitingMenu(const Person& p) {
        cout << "  Waiting list length: " << waitQ.size() << "\n";
        int pos = waitPosition(p.plate);
        if (!pos) { cout << "  You are not on the waiting list.\n"; return; }
        cout << "  Your position: " << pos << "\n";
        if (readInt("  Leave the waiting list? (1 = yes, 2 = no): ", 1, 2) == 1) {
            removeFromWaitlist(p.plate);
            save();
            cout << "  You have left the waiting list.\n";
        }
    }

    void showHistory(const Person& p) {
        cout << "  1) Oldest first   2) Newest first\n";
        bool fwd = readInt("  Choose: ", 1, 2) == 1;
        if (history.printFor(p.plate, fwd) == 0) cout << "  (no history yet)\n";
    }

    void undoLast(const Person& p) {
        auto& st = undoStacks[p.plate];
        while (!st.empty()) {
            int rid = st.top();
            st.pop();
            if (cancelReservation(rid, "UNDONE")) { save(); return; }
        }
        cout << "  Nothing to undo.\n";
    }

    // entry point used by the portal
    void session(const Person& p) {
        cout << "\n==========  PARKING SERVICES  ==========\n"
             << "  " << p.name << " | " << vehicleName(p.vehicle) << " | " << p.plate << "\n";
        if (activeOf(p.plate)) { cout << "\n  Your current booking:\n"; showMyReservation(p); }
        else if (int pos = waitPosition(p.plate)) cout << "\n  You are on the waiting list (position " << pos << ").\n";
        while (true) {
            cout << "\n [Parking]  1. Find best slot & reserve\n"
                 << "            2. My reservation & route\n"
                 << "            3. Cancel my reservation\n"
                 << "            4. Parking status (all slots)\n"
                 << "            5. Waiting list\n"
                 << "            6. My reservation history\n"
                 << "            7. Undo my last reservation\n"
                 << "            0. Back to portal\n";
            switch (readInt(" Choice: ", 0, 7)) {
                case 1: findAndReserve(p); break;
                case 2: showMyReservation(p); break;
                case 3: cancelMine(p); break;
                case 4: showStatus(p); break;
                case 5: waitingMenu(p); break;
                case 6: showHistory(p); break;
                case 7: undoLast(p); break;
                case 0: return;
            }
        }
    }
};

// ---------------------------- hook called by the login portal ---------------
inline void runParkingServices(const string& plate, const string& ownerName, int vehicleType) {
    static ParkingService service;  // one shared instance for all users in this run
    service.session({plate, ownerName, vehicleType});
}
