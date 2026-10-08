/* =========================================================
   PARKNEXA
   app.js
========================================================= */


/* =========================================================
   TOAST
========================================================= */

let toastTimer = null;


function showToast(
    title,
    message
) {

    const toast =
        document.getElementById(
            "toast"
        );


    const toastTitle =
        document.getElementById(
            "toastTitle"
        );


    const toastMessage =
        document.getElementById(
            "toastMessage"
        );


    toastTitle.textContent =
        title;


    toastMessage.textContent =
        message;


    toast.classList.add(
        "show"
    );


    clearTimeout(
        toastTimer
    );


    toastTimer =
        setTimeout(
            function() {

                toast.classList.remove(
                    "show"
                );

            },
            3500
        );

}


/* =========================================================
   MOBILE NAVIGATION
========================================================= */

function initializeNavigation() {

    const mobileMenuBtn =
        document.getElementById(
            "mobileMenuBtn"
        );


    const mainNav =
        document.getElementById(
            "mainNav"
        );


    mobileMenuBtn.addEventListener(
        "click",
        function() {

            const isOpen =
                mainNav.classList.toggle(
                    "open"
                );


            mobileMenuBtn.setAttribute(
                "aria-expanded",
                isOpen
                    ? "true"
                    : "false"
            );

        }
    );


    document
        .querySelectorAll(
            ".nav-links a"
        )
        .forEach(
            link => {

                link.addEventListener(
                    "click",
                    function() {

                        mainNav.classList.remove(
                            "open"
                        );


                        mobileMenuBtn.setAttribute(
                            "aria-expanded",
                            "false"
                        );

                    }
                );

            }
        );

}


/* =========================================================
   SCROLL HELPER
========================================================= */

function scrollToSection(
    id
) {

    const section =
        document.getElementById(
            id
        );


    if (!section) {
        return;
    }


    section.scrollIntoView({
        behavior: "smooth"
    });

}


/* =========================================================
   UPDATE DASHBOARD NUMBERS
========================================================= */

function updateParkingSummary() {

    const totalSpaces = 360;

    const availableSpaces = 148;

    const occupiedSpaces =
        totalSpaces -
        availableSpaces;

    const percentage =
        Math.round(
            (
                occupiedSpaces /
                totalSpaces
            ) * 100
        );


    document
        .getElementById(
            "heroAvailable"
        )
        .textContent =
        availableSpaces;


    document
        .getElementById(
            "availableCount"
        )
        .textContent =
        availableSpaces;


    document
        .getElementById(
            "occupiedCount"
        )
        .textContent =
        occupiedSpaces;


    document
        .getElementById(
            "occupancyPercent"
        )
        .textContent =
        `${percentage}%`;

}


/* =========================================================
   INITIALIZE APPLICATION
========================================================= */

document.addEventListener(
    "DOMContentLoaded",
    function() {

        initializeNavigation();

        initializeAuth();

        initializeParking();

        initializeMap();

        updateParkingSummary();

    }
);