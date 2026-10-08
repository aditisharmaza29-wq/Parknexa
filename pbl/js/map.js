/* =========================================================
   PARKNEXA
   map.js
========================================================= */


/* =========================================================
   MAP FLOOR BUTTONS
========================================================= */

function initializeMap() {

    document
        .querySelectorAll(
            ".map-floor-btn"
        )
        .forEach(
            button => {

                button.addEventListener(
                    "click",
                    function() {

                        document
                            .querySelectorAll(
                                ".map-floor-btn"
                            )
                            .forEach(
                                btn =>
                                    btn.classList.remove(
                                        "active"
                                    )
                            );


                        this.classList.add(
                            "active"
                        );


                        const floor =
                            this.dataset.floor;


                        showToast(
                            `Parking ${floor}`,
                            `Showing parking information for ${floor}.`
                        );

                    }
                );

            }
        );


    document
        .querySelectorAll(
            ".floor-pill"
        )
        .forEach(
            button => {

                button.addEventListener(
                    "click",
                    function() {

                        document
                            .querySelectorAll(
                                ".floor-pill"
                            )
                            .forEach(
                                btn =>
                                    btn.classList.remove(
                                        "active"
                                    )
                            );


                        this.classList.add(
                            "active"
                        );

                    }
                );

            }
        );

}