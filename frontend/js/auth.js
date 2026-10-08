/* =========================================================
   PARKNEXA
   auth.js
========================================================= */

function getSavedUser() {

    try {

        const user =
            localStorage.getItem("parknexaUser");

        return user
            ? JSON.parse(user)
            : null;

    } catch (error) {

        console.error(
            "Unable to read saved user.",
            error
        );

        return null;
    }
}


/* =========================================================
   OPEN REGISTRATION
========================================================= */

function openRegistrationModal() {

    const modal =
        document.getElementById(
            "registrationModal"
        );

    modal.classList.remove(
        "hidden"
    );

    document.body.classList.add(
        "modal-open"
    );


    setTimeout(() => {

        document
            .getElementById("fullName")
            .focus();

    }, 50);

}


/* =========================================================
   CLOSE REGISTRATION
========================================================= */

function closeRegistrationModal() {

    const modal =
        document.getElementById(
            "registrationModal"
        );

    modal.classList.add(
        "hidden"
    );

    document.body.classList.remove(
        "modal-open"
    );

}


/* =========================================================
   UPDATE USER UI
========================================================= */

function updateUserInterface() {

    const user =
        getSavedUser();


    const profileChip =
        document.getElementById(
            "profileChip"
        );

    const profileName =
        document.getElementById(
            "profileName"
        );

    const profileAvatar =
        document.getElementById(
            "profileAvatar"
        );

    const notRegistered =
        document.getElementById(
            "notRegistered"
        );

    const registeredContent =
        document.getElementById(
            "registeredContent"
        );


    if (!user) {

        profileChip.style.display =
            "none";

        notRegistered
            .classList
            .remove("hidden");

        registeredContent
            .classList
            .add("hidden");

        return;

    }


    profileChip.style.display =
        "flex";


    profileName.textContent =
        user.name.split(" ")[0];


    profileAvatar.textContent =
        user.name
            .charAt(0)
            .toUpperCase();


    notRegistered
        .classList
        .add("hidden");


    registeredContent
        .classList
        .remove("hidden");


    document
        .getElementById("savedName")
        .textContent =
        user.name;


    document
        .getElementById("savedMobile")
        .textContent =
        user.mobile;


    document
        .getElementById("savedVehicle")
        .textContent =
        user.vehicle;


    document
        .getElementById(
            "savedVehicleType"
        )
        .textContent =
        user.vehicleType;


    if (user.vehicleType) {

        document
            .getElementById(
                "vehicleType"
            )
            .value =
            user.vehicleType;

    }


    if (user.preference) {

        document
            .getElementById(
                "parkingPreference"
            )
            .value =
            user.preference;

    }


    if (user.accessible) {

        document
            .getElementById(
                "accessibility"
            )
            .value =
            "accessible";

    }


    if (
        typeof
        updateBookingInterface ===
        "function"
    ) {

        updateBookingInterface();

    }

}


/* =========================================================
   REGISTRATION SUBMIT
========================================================= */

function handleRegistration(
    event
) {

    event.preventDefault();


    const name =
        document
            .getElementById(
                "fullName"
            )
            .value
            .trim();


    const mobile =
        document
            .getElementById(
                "mobile"
            )
            .value
            .trim();


    const vehicle =
        document
            .getElementById(
                "vehicleNumber"
            )
            .value
            .trim()
            .toUpperCase();


    const vehicleType =
        document
            .getElementById(
                "registeredVehicleType"
            )
            .value;


    if (!name || !vehicle || !vehicleType) {

        showToast(
            "Missing details",
            "Please complete all required fields."
        );

        return;
    }


    if (
        !/^[0-9]{10}$/.test(
            mobile
        )
    ) {

        showToast(
            "Invalid mobile number",
            "Please enter a valid 10-digit mobile number."
        );

        return;
    }


    const user = {

        name,

        mobile,

        vehicle,

        vehicleType,

        visitType:
            document
                .getElementById(
                    "visitType"
                )
                .value,

        preference:
            document
                .getElementById(
                    "registrationPreference"
                )
                .value,

        accessible:
            document
                .getElementById(
                    "accessibilityNeed"
                )
                .checked

    };


    localStorage.setItem(
        "parknexaUser",
        JSON.stringify(user)
    );


    updateUserInterface();

    closeRegistrationModal();


    showToast(
        "Registration successful",
        `Welcome to ParkNexa, ${name.split(" ")[0]}.`
    );

}


/* =========================================================
   AUTH INITIALIZATION
========================================================= */

function initializeAuth() {

    document
        .getElementById(
            "registerNavBtn"
        )
        .addEventListener(
            "click",
            openRegistrationModal
        );


    document
        .getElementById(
            "heroRegisterBtn"
        )
        .addEventListener(
            "click",
            openRegistrationModal
        );


    document
        .getElementById(
            "emptyRegisterBtn"
        )
        .addEventListener(
            "click",
            openRegistrationModal
        );


    document
        .getElementById(
            "closeModalBtn"
        )
        .addEventListener(
            "click",
            closeRegistrationModal
        );


    document
        .getElementById(
            "cancelModalBtn"
        )
        .addEventListener(
            "click",
            closeRegistrationModal
        );


    document
        .getElementById(
            "registrationForm"
        )
        .addEventListener(
            "submit",
            handleRegistration
        );


    document
        .getElementById(
            "registrationModal"
        )
        .addEventListener(
            "click",
            function(event) {

                if (
                    event.target ===
                    this
                ) {

                    closeRegistrationModal();

                }

            }
        );


    document.addEventListener(
        "keydown",
        function(event) {

            if (
                event.key === "Escape"
            ) {

                const modal =
                    document.getElementById(
                        "registrationModal"
                    );


                if (
                    !modal
                        .classList
                        .contains("hidden")
                ) {

                    closeRegistrationModal();

                }

            }

        }
    );


    updateUserInterface();

}