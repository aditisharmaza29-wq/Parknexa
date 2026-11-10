# ParkNexa -- Smart Parking Management System

ParkNexa is a **C++-based Smart Parking Management System** designed to
make parking more organised, convenient, and user-friendly. It provides
an account-based portal through which users can manage their profiles,
access parking services, find suitable parking slots, make reservations,
and manage their bookings.

The project focuses on preference-based parking slot recommendations,
reservation management, parking availability, and automated parking-fee
calculation.

## Table of Contents

-   [Features](#features)
-   [Application Workflow](#application-workflow)
-   [Parking Slot Recommendation](#parking-slot-recommendation)
-   [Parking Services](#parking-services)
-   [Parking Charges](#parking-charges)
-   [Technology Stack](#technology-stack)
-   [Project Objectives](#project-objectives)

## Features

### 1. User Registration and Login

-   New users can register for an account.
-   Existing users can log in using their username and password.
-   Login is limited to **three attempts**. If the user does not enter
    the correct password within three attempts, the program returns to
    the initial registration/login screen.

### 2. User Portal

After logging in, users can access the following options:

-   **View My Profile** -- view their profile and registered vehicle
    information.
-   **Parking Services** -- access parking slot recommendations and
    reservation-related services.
-   **Change Password** -- update their account password.
-   **Logout** -- leave the account and return to the initial screen.

### 3. Multiple Vehicle Support

A user account can be associated with multiple vehicles, allowing users
to manage more than one vehicle through the same account.

## Application Workflow

1.  The application starts by displaying the **Register** and **Login**
    options.
2.  A new user creates an account, while an existing user logs in with
    their username and password.
3.  After successful login, the user enters the ParkNexa Portal.
4.  The user selects **Parking Services** to access the parking menu.
5.  The user can find and reserve a recommended parking slot, view or
    cancel a reservation, check parking status, use the waiting list,
    view reservation history, or undo the latest reservation.
6.  Users can return to the portal or log out.

## Parking Slot Recommendation

One of ParkNexa's key features is its preference-based parking slot
recommendation system.

When selecting **Find Best Slot & Reserve**, users can choose one of
three preferences:

  -----------------------------------------------------------------------
  Preference                          Purpose
  ----------------------------------- -----------------------------------
  Closest to entrance                 Prioritises slots nearer to the
                                      entrance

  Least crowded area                  Prioritises a less crowded parking
                                      area

  Balanced                            Considers multiple factors to
                                      provide a balanced recommendation
  -----------------------------------------------------------------------

Users can also specify whether they prefer **covered parking**.

Based on the selected preferences, the system displays recommended slots
in ranked order. The recommendation display includes:

  Field          Description
  -------------- ------------------------------------------
  Slot           Parking slot identifier
  Area           Parking area or section
  Type           Slot type
  Cover          Whether the slot is covered
  Distance (m)   Distance from the entrance, in metres
  Score          Recommendation score shown by the system

Users can select one of the displayed recommendations to reserve that
slot.

## Parking Services

The Parking Services menu provides the following options:

1.  **Find Best Slot & Reserve** -- receive recommendations based on
    parking preferences and reserve a selected slot.
2.  **My Reservation & Route** -- view reservation details and the route
    to the reserved parking slot.
3.  **Cancel My Reservation** -- cancel an existing reservation.
4.  **Parking Status (All Slots)** -- view the status of parking slots.
5.  **Waiting List** -- access the waiting-list facility when a suitable
    slot is unavailable.
6.  **My Reservation History** -- view previous reservation records.
7.  **Undo My Last Reservation** -- undo the most recent reservation.
8.  **Back to Portal** -- return to the main user portal.

## Parking Charges

ParkNexa also includes a parking-charges module that calculates fees
according to the duration of parking.

The pricing rules discussed for the module are:

  Parking duration      Charge
  ------------------- --------
  Up to 30 minutes         ₹25
  1 hour                   ₹50
  1 hour 30 minutes        ₹75
  2 hours                 ₹100

The minimum charge is **₹25**, including when a vehicle is parked for
less than 30 minutes. Beyond the first 30 minutes, the intended rate is
**₹50 per hour**, calculated proportionally.

The charges module also supports user registration and login and allows
multiple vehicles to be associated with one account.

> **Note:** The parking-charges module and the main parking-services
> application may be maintained as separate modules. Confirm their
> integration and the exact billing implementation against the final
> source code before describing them as one fully integrated workflow.

## Technology Stack

-   **Programming language:** C++
-   **Application type:** Console-based application

The project uses C++ programming concepts such as functions, conditional
statements, loops, user authentication, data handling, recommendation
logic, reservation management, and time-based calculations.

## Project Objectives

-   Make parking management more organised and convenient.
-   Provide users with individual accounts and vehicle-management
    facilities.
-   Recommend parking slots based on user preferences.
-   Support parking reservations and cancellations.
-   Allow users to check parking-slot status and reservation history.
-   Reduce manual effort in parking-fee calculation.

## Future Improvements

Possible future enhancements include:

-   A graphical user interface.
-   Integration with real-time parking sensors.
-   Online payment support.
-   QR-code-based entry and exit.
-   Live navigation to parking slots.
-   A central database for managing users, vehicles, slots, and
    reservations.

------------------------------------------------------------------------

**ParkNexa -- Making Parking Smarter and Simpler.**
