/* =========================================================
   PARKNEXA
   parking.js
========================================================= */

let currentResults =
    [...parkingData];

let currentRecommendation =
    null;


/* =========================================================
   RENDER PARKING CARDS
========================================================= */

function renderParkingCards(
    data,
    recommendationId = null
) {

    const container =
        document.getElementById(
            "slotResults"
        );


    container.innerHTML = "";


    if (
        !data ||
        data.length === 0
    ) {

        container.innerHTML = `

            <div
                class="slot-card"
                style="
                    grid-column:1/-1;
                    text-align:center;
                    padding:35px;
                ">

                <h3>
                    No suitable spaces found
                </h3>

                <p
                    style="
                        color:var(--muted);
                        margin-top:7px;
                        font-size:.83rem;
                    ">

                    Try changing your preferences.

                </p>

            </div>

        `;

        return;
    }


    data.forEach(slot => {

        const card =
            document.createElement(
                "article"
            );


        const isBest =
            recommendationId ===
            slot.id;


        card.className =
            `slot-card ${
                isBest
                    ? "best-choice"
                    : ""
            }`;


        card.innerHTML = `

            ${
                isBest
                    ? `
                        <span class="best-label">
                            BEST MATCH
                        </span>
                      `
                    : ""
            }


            <div class="slot-top">

                <div>

                    <div class="slot-id">
                        ${slot.id}
                    </div>

                    <small
                        style="color:var(--muted);">

                        Floor ${slot.floor}
                        · Zone ${slot.zone}

                    </small>

                </div>


                <span
                    class="availability available">

                    Available

                </span>

            </div>


            <div class="slot-details">

                <div class="slot-detail">

                    <span>
                        Distance
                    </span>

                    <strong>
                        ${slot.distance} m
                    </strong>

                </div>


                <div class="slot-detail">

                    <span>
                        Walk
                    </span>

                    <strong>
                        ${slot.walk} min
                    </strong>

                </div>


                <div class="slot-detail">

                    <span>
                        Area Load
                    </span>

                    <strong>
                        ${slot.occupancy}%
                    </strong>

                </div>


                <div class="slot-detail">

                    <span>
                        Feature
                    </span>

                    <strong>
                        ${slot.feature}
                    </strong>

                </div>

            </div>


            <div class="slot-actions">

                <button
                    class="btn btn-outline"
                    type="button"
                    onclick="showRoute('${slot.id}')">

                    View Route

                </button>


                <button
                    class="btn btn-primary"
                    type="button"
                    onclick="reserveParking('${slot.id}')">

                    Select

                </button>

            </div>

        `;


        container.appendChild(card);

    });

}


/* =========================================================
   FILTER PARKING
========================================================= */

function filterParking() {

    const vehicle =
        document
            .getElementById(
                "vehicleType"
            )
            .value;


    const accessibility =
        document
            .getElementById(
                "accessibility"
            )
            .value;


    let results =
        parkingData.filter(
            slot => {

                if (
                    vehicle === "EV"
                ) {

                    return (
                        slot.type === "EV"
                    );

                }


                if (
                    vehicle === "Bike"
                ) {

                    return (
                        slot.type === "Bike"
                    );

                }


                if (
                    vehicle === "SUV"
                ) {

                    return (
                        slot.type !== "Bike"
                    );

                }


                return true;

            }
        );


    if (
        accessibility ===
        "accessible"
    ) {

        const accessible =
            results.filter(
                slot =>
                    slot.feature ===
                    "Near Lift"
            );


        if (
            accessible.length > 0
        ) {

            results =
                accessible;

        }

    }


    /*
        If a special vehicle filter
        returns nothing, show available
        general parking rather than leaving
        an empty interface.
    */

    if (
        results.length === 0
    ) {

        results =
            parkingData.filter(
                slot =>
                    slot.status ===
                    "Available"
            );

    }


    return results;

}


/* =========================================================
   SORT ACCORDING TO USER PREFERENCE
========================================================= */

function sortByPreference(
    results
) {

    const preference =
        document
            .getElementById(
                "parkingPreference"
            )
            .value;


    results.sort(
        (a, b) => {

            if (
                preference ===
                "nearest"
            ) {

                return (
                    a.distance -
                    b.distance
                );

            }


            if (
                preference ===
                "lessbusy"
            ) {

                return (
                    a.occupancy -
                    b.occupancy
                );

            }


            if (
                preference ===
                "easy"
            ) {

                const aValue =
                    a.feature ===
                    "Easy Exit"
                        ? 0
                        : 1;


                const bValue =
                    b.feature ===
                    "Easy Exit"
                        ? 0
                        : 1;


                return (
                    aValue -
                    bValue
                );

            }


            return (

                (
                    a.occupancy +
                    a.distance / 10
                )

                -

                (
                    b.occupancy +
                    b.distance / 10
                )

            );

        }
    );


    return results;

}


/* =========================================================
   FIND BEST PARKING
========================================================= */

function findBestParking() {

    let results =
        filterParking();


    results =
        sortByPreference(results);


    currentResults =
        results;


    currentRecommendation =
        results[0] || null;


    if (
        currentRecommendation
    ) {

        updateRecommendation(
            currentRecommendation
        );


        renderParkingCards(
            currentResults,
            currentRecommendation.id
        );


        showToast(
            "Parking found",
            `Your suggested space is ${currentRecommendation.id}.`
        );

    }

}


/* =========================================================
   RECOMMENDATION UI
========================================================= */

function updateRecommendation(
    slot
) {

    if (!slot) {
        return;
    }


    document
        .getElementById(
            "recommendedSlot"
        )
        .textContent =
        `${slot.id} · ${slot.floor}`;


    document
        .getElementById(
            "recommendedDistance"
        )
        .textContent =
        `${slot.distance} m`;


    document
        .getElementById(
            "recommendedWalk"
        )
        .textContent =
        `${slot.walk} min`;


    document
        .getElementById(
            "recommendedArea"
        )
        .textContent =
        `Zone ${slot.zone}`;


    document
        .getElementById(
            "recommendTitle"
        )
        .textContent =
        `Best match: ${slot.id}`;


    document
        .getElementById(
            "recommendDescription"
        )
        .textContent =
        `${slot.feature}. Approx. ${slot.walk} minutes from the mall entrance.`;

}


/* =========================================================
   SORT DISPLAYED RESULTS
========================================================= */

function initializeParkingSorting() {

    document
        .getElementById(
            "sortSpaces"
        )
        .addEventListener(
            "change",
            function() {

                const sorted =
                    [...currentResults];


                if (
                    this.value ===
                    "distance"
                ) {

                    sorted.sort(
                        (a,b) =>
                            a.distance -
                            b.distance
                    );

                }


                else if (
                    this.value ===
                    "floor"
                ) {

                    sorted.sort(
                        (a,b) =>
                            a.floor.localeCompare(
                                b.floor
                            )
                    );

                }


                else if (
                    currentRecommendation
                ) {

                    sorted.sort(
                        (a,b) => {

                            if (
                                a.id ===
                                currentRecommendation.id
                            ) {

                                return -1;

                            }


                            if (
                                b.id ===
                                currentRecommendation.id
                            ) {

                                return 1;

                            }


                            return (
                                a.distance -
                                b.distance
                            );

                        }
                    );

                }


                renderParkingCards(
                    sorted,
                    currentRecommendation
                        ? currentRecommendation.id
                        : null
                );

            }
        );

}


/* =========================================================
   RESERVE PARKING
========================================================= */

function reserveParking(
    slotId
) {

    const user =
        getSavedUser();


    if (!user) {

        showToast(
            "Registration required",
            "Please register your vehicle before selecting a space."
        );


        openRegistrationModal();

        return;

    }


    const slot =
        parkingData.find(
            item =>
                item.id === slotId
        );


    if (!slot) {
        return;
    }


    localStorage.setItem(
        "parknexaBooking",
        JSON.stringify(slot)
    );


    updateBookingInterface();


    showToast(
        "Parking selected",
        `${slot.id} on ${slot.floor} is ready for your visit.`
    );


    scrollToSection(
        "my-parking"
    );

}


/* =========================================================
   BOOKING DISPLAY
========================================================= */

function updateBookingInterface() {

    const savedBooking =
        localStorage.getItem(
            "parknexaBooking"
        );


    if (!savedBooking) {
        return;
    }


    try {

        const booking =
            JSON.parse(
                savedBooking
            );


        document
            .getElementById(
                "bookingSlot"
            )
            .textContent =
            `${booking.id} · ${booking.floor} · Zone ${booking.zone}`;


        document
            .getElementById(
                "bookingDetails"
            )
            .textContent =
            `${booking.distance} m away · Estimated walk ${booking.walk} min · ${booking.feature}`;

    }

    catch (error) {

        localStorage.removeItem(
            "parknexaBooking"
        );

    }

}


/* =========================================================
   ROUTE
========================================================= */

function showRoute(
    slotId
) {

    const slot =
        parkingData.find(
            item =>
                item.id === slotId
        );


    if (!slot) {
        return;
    }


    const entry =
        document
            .getElementById(
                "entryGate"
            )
            .value;


    showToast(
        "Route ready",
        `${entry} Entrance → ${slot.floor} → Zone ${slot.zone} → Space ${slot.id}`
    );


    scrollToSection(
        "mall-map"
    );

}


/* =========================================================
   PARKING INITIALIZATION
========================================================= */

function initializeParking() {

    document
        .getElementById(
            "findParkingBtn"
        )
        .addEventListener(
            "click",
            findBestParking
        );


    initializeParkingSorting();


    renderParkingCards(
        parkingData.slice(0,6)
    );


    updateBookingInterface();

}