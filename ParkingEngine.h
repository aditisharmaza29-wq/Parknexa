// ============================================================================
//  ParkingEngine.h
//  The core parking logic engine.
//
//  This is a refactored version of ParkingService from ParkNexa_Services.h.
//  All std::cin / std::cout has been removed.
//  Every method now returns data that the Qt UI can display.
//
//  DSA preserved:
//    - Dijkstra shortest path (priority_queue)
//    - Priority-based slot ranking (priority_queue + score formula)
//    - Doubly linked list (HistoryList)
//    - Stack per vehicle (undo)
//    - Queue (waiting list)
//    - Binary search (findSlotIdx)
// ============================================================================
#pragma once
#include "ParkingTypes.h"
#include <algorithm>
#include <climits>
#include <ctime>
#include <fstream>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
//  HistoryList -- doubly linked list of reservation history records
//  (preserved exactly from original ParkNexa_Services.h)
// ---------------------------------------------------------------------------
struct HNode {
    Record rec;
    HNode *prev = nullptr;
    HNode *next = nullptr;
    explicit HNode(const Record& r) : rec(r) {}
};

class HistoryList {
    HNode *head = nullptr;
    HNode *tail = nullptr;
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

    // Returns all records for a given plate (forward or reverse order)
    std::vector<Record> getFor(const std::string& plate, bool forward) const {
        std::vector<Record> out;
        if (forward) {
            for (HNode* c = head; c; c = c->next)
                if (c->rec.plate == plate) out.push_back(c->rec);
        } else {
            for (HNode* c = tail; c; c = c->prev)
                if (c->rec.plate == plate) out.push_back(c->rec);
        }
        return out;
    }

    // For persistence: iterate all records
    template<class F> void forEach(F f) const {
        for (HNode* c = head; c; c = c->next) f(c->rec);
    }
};

// ---------------------------------------------------------------------------
//  Comparator for ranking priority queue (lower score = better slot)
// ---------------------------------------------------------------------------
struct CandCmp {
    bool operator()(const Candidate& a, const Candidate& b) const {
        return a.score > b.score;  // min-heap by score
    }
};

// ---------------------------------------------------------------------------
//  ParkingEngine -- the full parking service engine
// ---------------------------------------------------------------------------
class ParkingEngine {
    // ── Mall graph ──────────────────────────────────────────────────────────
    std::vector<std::vector<std::pair<int,int>>> adj;  // adjacency list
    std::vector<std::string> nodeName;
    std::vector<int> parent;
    std::vector<int> distFromEntrance;

    // ── Parking data ────────────────────────────────────────────────────────
    std::vector<Slot>                         m_slots;         // sorted by id
    std::unordered_map<int, Reservation>      reservations;
    std::unordered_map<std::string, std::stack<int>> undoStacks; // per plate
    std::queue<WaitEntry>                     waitQ;
    HistoryList                               history;
    int nextResId = 1001;

    std::string dataFile;   // path to parking_data.txt

    // ── Graph helpers ────────────────────────────────────────────────────────
    void addEdge(int a, int b, int w) {
        adj[a].push_back({b, w});
        adj[b].push_back({a, w});
    }

    // Dijkstra from node src  (preserved exactly from original)
    void dijkstra(int src) {
        int n = static_cast<int>(adj.size());
        distFromEntrance.assign(n, INT_MAX);
        parent.assign(n, -1);
        std::priority_queue<std::pair<int,int>,
                            std::vector<std::pair<int,int>>,
                            std::greater<>> pq;
        distFromEntrance[src] = 0;
        pq.push({0, src});
        while (!pq.empty()) {
            auto [d, u] = pq.top(); pq.pop();
            if (d > distFromEntrance[u]) continue;
            for (auto [v, w] : adj[u]) {
                if (d + w < distFromEntrance[v]) {
                    distFromEntrance[v] = d + w;
                    parent[v] = u;
                    pq.push({distFromEntrance[v], v});
                }
            }
        }
    }

    // Build the sample mall (same layout as original)
    void buildMall() {
        // 13 nodes: 0=Entrance, 1-3=Junctions A/B/C, 4-12=Slots S1-S9
        nodeName = {"Mall Entrance", "Junction A", "Junction B", "Junction C"};
        for (int i = 1; i <= 9; i++)
            nodeName.push_back("Slot S" + std::to_string(i));

        adj.assign(13, {});
        addEdge(0, 1, 20); addEdge(1, 2, 30); addEdge(1, 3, 30);
        addEdge(2, 3, 70);
        addEdge(1, 4, 10); addEdge(1, 5, 15); addEdge(1, 6, 20);  // Wing A
        addEdge(2, 7, 10); addEdge(2, 8, 15); addEdge(2, 9, 25);  // Wing B
        addEdge(3, 10, 12); addEdge(3, 11, 20); addEdge(3, 12, 30); // Wing C
        addEdge(9, 12, 40);
        dijkstra(0);

        m_slots = {
            {1, 'A', S_ACCESS,  true,  4},
            {2, 'A', S_REGULAR, true,  5},
            {3, 'A', S_COMPACT, true,  6},
            {4, 'B', S_EV,      false, 7},
            {5, 'B', S_REGULAR, false, 8},
            {6, 'B', S_REGULAR, false, 9},
            {7, 'C', S_COMPACT, false, 10},
            {8, 'C', S_REGULAR, false, 11},
            {9, 'C', S_EV,      false, 12},
        };
        for (auto& s : m_slots) s.dist = distFromEntrance[s.node];

        // Two walk-in occupied m_slots (same as original)
        m_slots[2].occupied = true;  // S3
        m_slots[5].occupied = true;  // S6
    }

    // ── Persistence ─────────────────────────────────────────────────────────
    void save() const {
        std::ofstream out(dataFile);
        std::vector<int> ids;
        for (auto& kv : reservations) ids.push_back(kv.first);
        std::sort(ids.begin(), ids.end());
        for (int id : ids) {
            const Reservation& r = reservations.at(id);
            out << "R|" << r.id << '|' << r.plate << '|' << r.owner
                << '|'  << r.slotId << '|' << r.time << '|' << r.active << '\n';
        }
        history.forEach([&](const Record& r){
            out << "H|" << r.resId << '|' << r.plate << '|' << r.slotId
                << '|'  << r.action << '|' << r.time << '\n';
        });
        std::queue<WaitEntry> c = waitQ;
        while (!c.empty()) {
            const WaitEntry& w = c.front();
            out << "W|" << w.who.plate << '|' << w.who.name << '|'
                << w.who.vehicle << '|' << w.pref.mode << '|'
                << w.pref.covered << '\n';
            c.pop();
        }
    }

    static std::vector<std::string> splitBar(const std::string& line) {
        std::vector<std::string> f;
        std::string tok;
        std::istringstream ss(line);
        while (std::getline(ss, tok, '|')) f.push_back(tok);
        return f;
    }

    void load() {
        std::ifstream in(dataFile);
        std::string line;
        std::vector<int> activeIds;
        while (std::getline(in, line)) {
            auto f = splitBar(line);
            try {
                if (f[0] == "R" && f.size() == 7) {
                    Reservation r{std::stoi(f[1]), f[2], f[3],
                                  std::stoi(f[4]), f[5], f[6] == "1"};
                    reservations[r.id] = r;
                    nextResId = std::max(nextResId, r.id + 1);
                    if (r.active) {
                        int idx = findSlotIdx(r.slotId);
                        if (idx >= 0) {
                            m_slots[idx].occupied = true;
                            m_slots[idx].resId = r.id;
                        }
                        activeIds.push_back(r.id);
                    }
                } else if (f[0] == "H" && f.size() == 6) {
                    history.add({std::stoi(f[1]), f[2],
                                 std::stoi(f[3]), f[4], f[5]});
                } else if (f[0] == "W" && f.size() == 6) {
                    WaitEntry w;
                    w.who = {f[1], f[2], std::stoi(f[3])};
                    w.pref.mode    = std::stoi(f[4]);
                    w.pref.covered = f[5] == "1";
                    waitQ.push(w);
                }
            } catch (...) { /* skip corrupt lines */ }
        }
        std::sort(activeIds.begin(), activeIds.end());
        for (int id : activeIds)
            undoStacks[reservations[id].plate].push(id);
    }

    // ── Lookups ──────────────────────────────────────────────────────────────
    // Binary search by slot id (preserved from original)
    int findSlotIdx(int id) const {
        int lo = 0, hi = static_cast<int>(m_slots.size()) - 1;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            if (m_slots[mid].id == id) return mid;
            if (m_slots[mid].id  < id) lo = mid + 1;
            else hi = mid - 1;
        }
        return -1;
    }

    Reservation* activeOf(const std::string& plate) {
        for (auto& kv : reservations)
            if (kv.second.active && kv.second.plate == plate)
                return &kv.second;
        return nullptr;
    }

    double areaOccupancy(char area) const {
        int total = 0, occ = 0;
        for (auto& s : m_slots)
            if (s.area == area) { total++; if (s.occupied) occ++; }
        return total ? static_cast<double>(occ) / total : 0.0;
    }

    int waitPosition(const std::string& plate) const {
        std::queue<WaitEntry> c = waitQ;
        for (int pos = 1; !c.empty(); pos++, c.pop())
            if (c.front().who.plate == plate) return pos;
        return 0;
    }

    void removeFromWaitlist(const std::string& plate) {
        std::queue<WaitEntry> keep;
        while (!waitQ.empty()) {
            if (waitQ.front().who.plate != plate) keep.push(waitQ.front());
            waitQ.pop();
        }
        waitQ = keep;
    }

    // ── Ranking (priority algorithm, preserved exactly) ───────────────────
    // Returns a penalty value; 99 means the slot is not compatible
    static double typePenalty(int veh, int st) {
        switch (veh) {
            case BIKE:       return st == S_COMPACT ? 0.0 : st == S_REGULAR ? 0.15 : 99;
            case CAR:        return st == S_REGULAR ? 0.0 : 99;
            case EV:         return st == S_EV ? -0.10 : st == S_REGULAR ? 0.10 : 99;
            case ACCESSIBLE: return st == S_ACCESS ? -0.10 : st == S_REGULAR ? 0.05 : 99;
        }
        return 99;
    }

    // Returns ranked candidates (min-heap by score)
    std::priority_queue<Candidate, std::vector<Candidate>, CandCmp>
    rank(int vehicle, const Pref& p) const {
        double wd = 0.5, wo = 0.5;
        if (p.mode == 1) { wd = 0.8; wo = 0.2; }
        if (p.mode == 2) { wd = 0.2; wo = 0.8; }
        int maxD = 1;
        for (auto& s : m_slots) maxD = std::max(maxD, s.dist);

        std::priority_queue<Candidate, std::vector<Candidate>, CandCmp> pq;
        for (int i = 0; i < static_cast<int>(m_slots.size()); i++) {
            const Slot& s = m_slots[i];
            if (s.occupied) continue;
            double pen = typePenalty(vehicle, s.type);
            if (pen > 50) continue;
            double score = wd * (static_cast<double>(s.dist) / maxD)
                         + wo * areaOccupancy(s.area) + pen;
            if (p.covered && s.covered) score -= 0.15;
            pq.push({score, i});
        }
        return pq;
    }

    // ── Core mutation helpers ────────────────────────────────────────────────
    static std::string nowStr() {
        time_t t = time(nullptr);
        char b[32];
        strftime(b, sizeof b, "%d-%b-%Y %H:%M:%S", localtime(&t));
        return b;
    }

    int createReservation(const Person& p, int slotIdx) {
        Slot& s = m_slots[slotIdx];
        int rid = nextResId++;
        s.occupied = true;
        s.resId    = rid;
        reservations[rid] = {rid, p.plate, p.name, s.id, nowStr(), true};
        undoStacks[p.plate].push(rid);
        history.add({rid, p.plate, s.id, "RESERVED", nowStr()});
        removeFromWaitlist(p.plate);
        return rid;
    }

    bool cancelReservation(int rid, const std::string& action) {
        auto it = reservations.find(rid);
        if (it == reservations.end() || !it->second.active) return false;
        Reservation& r = it->second;
        r.active = false;
        int idx = findSlotIdx(r.slotId);
        if (idx >= 0) { m_slots[idx].occupied = false; m_slots[idx].resId = 0; }
        history.add({rid, r.plate, r.slotId, action, nowStr()});
        processWaitingList();
        return true;
    }

    void processWaitingList() {
        size_t n = waitQ.size();
        for (size_t i = 0; i < n; i++) {
            WaitEntry w = waitQ.front(); waitQ.pop();
            auto pq = rank(w.who.vehicle, w.pref);
            if (pq.empty()) { waitQ.push(w); continue; }
            createReservation(w.who, pq.top().slotIdx);
        }
    }

public:
    // ── Constructor ──────────────────────────────────────────────────────────
    explicit ParkingEngine(const std::string& dataPath) : dataFile(dataPath) {
        buildMall();
        load();
    }

    // ── Read-only queries (return data to the UI) ────────────────────────────

    // All m_slots (for dashboard + parking grid)
    std::vector<Slot> getSlots() const { return m_slots; }

    // Active reservation for a plate (nullptr = none)
    const Reservation* getActiveReservation(const std::string& plate) {
        return activeOf(plate);
    }

    // Stats for dashboard
    int totalSlots()    const { return static_cast<int>(m_slots.size()); }
    int occupiedSlots() const {
        int n = 0; for (auto& s : m_slots) if (s.occupied) n++; return n;
    }
    int freeSlots()     const { return totalSlots() - occupiedSlots(); }
    int reservedSlots() const {
        int n = 0;
        for (auto& s : m_slots) if (s.occupied && s.resId != 0) n++;
        return n;
    }
    double areaOccupancyPct(char area) const {
        return areaOccupancy(area) * 100.0;
    }
    int waitListSize() const { return static_cast<int>(waitQ.size()); }
    int waitPositionFor(const std::string& plate) const {
        return waitPosition(plate);
    }

    // All reservations (for history page)
    std::vector<Reservation> getAllReservations() const {
        std::vector<Reservation> out;
        for (auto& kv : reservations) out.push_back(kv.second);
        std::sort(out.begin(), out.end(),
            [](const Reservation& a, const Reservation& b){ return a.id > b.id; });
        return out;
    }

    // Reservation history for a specific plate
    std::vector<Record> getHistoryFor(const std::string& plate, bool forward) const {
        return history.getFor(plate, forward);
    }

    // Route as a list of node names (for the map page)
    RoutePath getRoute(int slotId) const {
        int idx = findSlotIdx(slotId);
        if (idx < 0) return {};
        const Slot& s = m_slots[idx];
        std::vector<int> path;
        for (int v = s.node; v != -1; v = parent[v]) path.push_back(v);
        std::reverse(path.begin(), path.end());
        RoutePath names;
        for (int v : path) names.push_back(nodeName[v]);
        return names;
    }

    // Graph data for the map visualiser
    struct GraphEdge { int from, to, weight; };
    std::vector<GraphEdge> getEdges() const {
        std::vector<GraphEdge> edges;
        int n = static_cast<int>(adj.size());
        for (int u = 0; u < n; u++)
            for (auto [v, w] : adj[u])
                if (u < v) edges.push_back({u, v, w});
        return edges;
    }
    std::vector<std::string> getNodeNames() const { return nodeName; }
    int nodeCount() const { return static_cast<int>(adj.size()); }

    // Top-N ranked candidates for a vehicle + preference
    std::vector<Candidate> getRankedCandidates(int vehicle, const Pref& p,
                                               int topN = 3) {
        auto pq = rank(vehicle, p);
        std::vector<Candidate> out;
        while (!pq.empty() && static_cast<int>(out.size()) < topN) {
            out.push_back(pq.top()); pq.pop();
        }
        return out;
    }

    // Get a specific slot by its id
    Slot getSlotById(int id) const {
        int idx = findSlotIdx(id);
        if (idx >= 0) { return m_slots[idx]; }
        return Slot{};
    }

    // ── Mutations (called by UI actions) ─────────────────────────────────────

    // Reserve a slot; returns reservation id on success, -1 on failure
    int reserve(const Person& p, int slotIdx) {
        int sz = static_cast<int>(m_slots.size());
        if (slotIdx < 0 || slotIdx >= sz) { return -1; }
        if (m_slots[slotIdx].occupied) { return -1; }
        if (activeOf(p.plate) != nullptr) { return -2; }
        int rid = createReservation(p, slotIdx);
        save();
        return rid;
    }

    // Cancel a plate's active reservation; returns true on success
    bool cancelByPlate(const std::string& plate) {
        Reservation* r = activeOf(plate);
        if (!r) { return false; }
        bool ok = cancelReservation(r->id, "CANCELLED");
        if (ok) { save(); }
        return ok;
    }

    // Undo last reservation for a plate
    bool undoLast(const std::string& plate) {
        auto& st = undoStacks[plate];
        while (!st.empty()) {
            int rid = st.top(); st.pop();
            if (cancelReservation(rid, "UNDONE")) { save(); return true; }
        }
        return false;
    }

    // Join waiting list
    bool joinWaitList(const Person& p, const Pref& pref) {
        if (waitPosition(p.plate) > 0) { return false; }
        waitQ.push({p, pref});
        save();
        return true;
    }

    // Leave waiting list
    bool leaveWaitList(const std::string& plate) {
        int before = static_cast<int>(waitQ.size());
        removeFromWaitlist(plate);
        int after = static_cast<int>(waitQ.size());
        bool changed = (after < before);
        if (changed) { save(); }
        return changed;
    }

    // Release a walk-in occupied slot (admin action)
    bool releaseSlot(int slotId) {
        int idx = findSlotIdx(slotId);
        if (idx < 0) { return false; }
        if (!m_slots[idx].occupied) { return false; }
        if (m_slots[idx].resId != 0) {
            return cancelReservation(m_slots[idx].resId, "RELEASED");
        }
        m_slots[idx].occupied = false;
        processWaitingList();
        save();
        return true;
    }
};
