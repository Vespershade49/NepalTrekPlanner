#ifndef DATA_H
#define DATA_H

#include "backend.h"
#include "mysqlmanager.h"

// Call this once at app startup to fill the backend with data.
void loadAllData(Backend& backend) {

    // ═══════════════════════════════════════════════════════════
    // TOURIST SPOTS
    // These stay hardcoded - your MySQL schema doesn't have a
    // tourist spots table, only Treks/Users/Guides/Bookings/etc.
    // ═══════════════════════════════════════════════════════════

    backend.addTouristSpot(TouristSpot(
        1, "Pashupatinath Temple", "Kathmandu", "Religious",
        "October to March", 1000.0,
        "One of the most sacred Hindu temples in the world, located on the banks of the Bagmati River."
        ));

    backend.addTouristSpot(TouristSpot(
        2, "Boudhanath Stupa", "Kathmandu", "Cultural",
        "October to April", 400.0,
        "One of the largest spherical stupas in Nepal and a major center of Tibetan Buddhism."
        ));

    backend.addTouristSpot(TouristSpot(
        3, "Phewa Lake", "Pokhara", "Nature",
        "September to November", 0.0,
        "A beautiful freshwater lake in Pokhara with stunning reflections of the Annapurna range."
        ));

    backend.addTouristSpot(TouristSpot(
        4, "Chitwan National Park", "Chitwan", "Nature",
        "October to March", 1500.0,
        "UNESCO World Heritage Site famous for one-horned rhinoceros, Bengal tigers, and elephant safaris."
        ));

    backend.addTouristSpot(TouristSpot(
        5, "Lumbini", "Rupandehi", "Religious",
        "November to February", 0.0,
        "Birthplace of Lord Buddha and a UNESCO World Heritage Site visited by pilgrims worldwide."
        ));

    backend.addTouristSpot(TouristSpot(
        6, "Everest Base Camp", "Solukhumbu", "Adventure",
        "March to May, September to November", 3000.0,
        "The base camp of the world's highest mountain at 5364m, a bucket-list destination for trekkers."
        ));

    backend.addTouristSpot(TouristSpot(
        7, "Annapurna Base Camp", "Kaski", "Adventure",
        "March to May, October to December", 2000.0,
        "Surrounded by towering Himalayan peaks including Annapurna I at 8091m."
        ));

    backend.addTouristSpot(TouristSpot(
        8, "Langtang Valley", "Rasuwa", "Nature",
        "March to May, October to November", 2000.0,
        "Known as the valley of glaciers, offering stunning views and rich Tamang culture."
        ));

    backend.addTouristSpot(TouristSpot(
        9, "Upper Mustang", "Mustang", "Cultural",
        "May to October", 50000.0,
        "A restricted demilitarized zone with ancient Tibetan culture, caves, and desert landscapes."
        ));

    backend.addTouristSpot(TouristSpot(
        10, "Rara Lake", "Mugu", "Nature",
        "April to June, September to November", 2000.0,
        "Nepal's largest lake situated at 2990m, known for its crystal-clear water and remote beauty."
        ));

    // ═══════════════════════════════════════════════════════════
    // EMERGENCY INFO - stays hardcoded too
    // ═══════════════════════════════════════════════════════════

    backend.addEmergencyInfo(EmergencyInfo(
        "Kathmandu", "Nepal Police", "100",
        "Tribhuvan University Teaching Hospital",
        "Altitude is low here. No altitude sickness risk."
        ));

    backend.addEmergencyInfo(EmergencyInfo(
        "Everest Region", "Himalayan Rescue Association", "01-4440292",
        "Pheriche Aid Post (4240m)",
        "High risk above 3500m. Ascend slowly, drink water, descend immediately if symptoms appear."
        ));

    backend.addEmergencyInfo(EmergencyInfo(
        "Annapurna Region", "Tourist Police Pokhara", "061-465117",
        "Manang Aid Post (3500m)",
        "Risk above 3000m. Take acclimatization days at Manang before Thorong La pass."
        ));

    backend.addEmergencyInfo(EmergencyInfo(
        "Langtang Region", "Langtang National Park Office", "01-4225489",
        "Kyanjin Clinic (3870m)",
        "Moderate risk. Watch for headaches and nausea above 3500m."
        ));

    backend.addEmergencyInfo(EmergencyInfo(
        "Mustang Region", "Tourist Police Jomsom", "069-440107",
        "Jomsom Hospital",
        "Risk present. Desert altitude of 3800m can cause dehydration and AMS."
        ));

    // ═══════════════════════════════════════════════════════════
    // TREK ROUTES - try MySQL first, fall back to hardcoded backup
    // if the database isn't running or the table is empty.
    // This is what keeps the app from opening with zero treks.
    // ═══════════════════════════════════════════════════════════

    MySqlManager dbManager;
    std::vector<TrekRoute> liveTreks = dbManager.fetchTrekRoutes();

    if (!liveTreks.empty()) {
        // MySQL connected successfully - use the live data
        for (const auto& route : liveTreks)
            backend.addTrekRoute(route);
    } else {
        // MySQL not available - use backup data so the app still works
        backend.addTrekRoute(TrekRoute(
            1, "Everest Base Camp Trek", 14, "Expert", 5364.0, 80000.0,
            "March-May, Sep-Nov", true,
            {"Lukla", "Namche Bazaar", "Tengboche", "Dingboche", "Lobuche", "Gorak Shep", "EBC"}
            ));

        backend.addTrekRoute(TrekRoute(
            2, "Annapurna Circuit Trek", 18, "Intermediate", 5416.0, 60000.0,
            "Oct-Nov, Mar-Apr", true,
            {"Besisahar", "Chame", "Pisang", "Manang", "Thorong La Pass", "Muktinath", "Pokhara"}
            ));

        backend.addTrekRoute(TrekRoute(
            3, "Poon Hill Trek", 4, "Beginner", 3210.0, 20000.0,
            "Oct-Dec, Feb-May", true,
            {"Nayapul", "Tikhedhunga", "Ghorepani", "Poon Hill", "Tadapani", "Ghandruk"}
            ));

        backend.addTrekRoute(TrekRoute(
            4, "Langtang Valley Trek", 7, "Intermediate", 3870.0, 35000.0,
            "Mar-May, Oct-Nov", false,
            {"Syabrubesi", "Lama Hotel", "Langtang Village", "Kyanjin Gompa", "Tsergo Ri"}
            ));

        backend.addTrekRoute(TrekRoute(
            5, "Ghorepani Poon Hill Trek", 5, "Beginner", 3210.0, 25000.0,
            "All year except monsoon", true,
            {"Pokhara", "Nayapul", "Hile", "Ghorepani", "Poon Hill", "Tadapani", "Chomrong"}
            ));
    }
}

#endif // DATA_H