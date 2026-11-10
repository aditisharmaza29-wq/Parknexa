// ============================================================================
//  AppContext.h
//  Central application context: holds shared service instances and the
//  currently logged-in user. Passed by pointer to every UI page.
//
//  This is the "glue" between the Qt UI and the underlying business logic.
// ============================================================================
#pragma once
#include "../core/AuthTypes.h"
#include "../core/ParkingEngine.h"
#include "../core/ChargesEngine.h"
#include "../services/AuthService.h"
#include <QString>
#include <QStandardPaths>
#include <QDir>

class AppContext {
public:
    AuthService    auth;
    ParkingEngine  parking;
    ChargesEngine  charges;
    Account        currentUser;    // set after successful login
    bool           isLoggedIn = false;

    explicit AppContext(const QString& dataDir)
        : auth   (toStd(dataDir + "/accounts.txt"))
        , parking(toStd(dataDir + "/parking_data.txt"))
        , charges(toStd(dataDir + "/parknexa_vehicles.txt"),
                  toStd(dataDir + "/parknexa_sessions.txt"))
    {}

private:
    static std::string toStd(const QString& q) {
        return q.toStdString();
    }
};
