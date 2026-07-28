#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QDateEdit>
#include <QFrame>
#include <vector>

#include "backend.h"
#include "Routeoptimizer.h"
#include "mysqlmanager.h"

// Holds whatever the user has picked so far, so it can follow them
// from "Plan a Trek" -> "Budget Calculator" -> "My Bookings" instead
// of each tab starting from a blank slate.
struct TripPlan {
    bool hasTrek = false;
    TrekRoute trek;
    std::vector<TouristSpot> spots;
    int days = 7;
    int people = 1;
    string accommodation = "budget";
    bool hasCalculatedCost = false;
    double totalCost = 0;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(int userId, QString userName, QString userEmail, QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void logoutRequested();

private slots:
    void onSaveProfile();
    void onChangePassword();
    void onLogout();
    void onSearchSpots();
    void onFindTrek();
    void onUseTrekForTrip();          // "Use this trek" in Plan a Trek
    void onCalculateBudget();
    void onProceedToBooking();        // "Book This Trip" in Budget Calculator
    void onFindRoute();
    void onShowEmergency();
    void onSaveItinerary();
    void onLoadItineraries();
    void onCreateBooking();
    void onRefreshBookings();
    void onConfirmBooking();
    void onCancelBooking();
    void onAssignGuide();
    void onLoadReviews();
    void onSubmitReview();

private:
    // ── Logged-in user (from LoginDialog) ──────────────────────
    int     currentUserId;
    QString currentUserName;
    QString currentUserEmail;

    void applyTripToRouteTab();       // auto-fills the destination city based on the selected trek
    void refreshProfileTripSummary(); // updates the "Your Trip" section on the Profile tab

    // ── Core backend objects ───────────────────────────────────
    Backend backend;
    RouteOptimizer routeOptimizer;
    MySqlManager mysqlManager;

    // ── The one shared piece of state that links every tab ─────
    TripPlan currentTrip;
    void updateTripBar();             // refreshes the summary bar text
    void applyTripToBudgetTab();      // pushes currentTrip into the Budget tab widgets
    void applyTripToBookingTab();     // pushes currentTrip into the Bookings tab widgets

    QTabWidget *tabs;

    // ---- Persistent "Your Trip" summary bar (sits above the tabs) ----
    QFrame      *tripBar;
    QLabel      *tripBarLabel;

    // ---- Tab 1: Explore Tourist Spots ----
    QComboBox      *categoryCombo;
    QLineEdit      *nameSearchEdit;
    QDoubleSpinBox *maxBudgetSpin;
    QPushButton    *searchSpotsBtn;
    QTableWidget   *spotsTable;

    // ---- Tab 2: Plan a Trek ----
    QComboBox      *fitnessCombo;
    QSpinBox       *daysSpin;
    QDoubleSpinBox *trekBudgetSpin;
    QPushButton    *findTrekBtn;
    QTableWidget   *treksTable;
    QPushButton    *useTrekBtn;

    // ---- Tab 3: Budget Calculator ----
    QComboBox      *budgetTrekCombo;      // pre-selected from Plan a Trek, but still changeable here
    QListWidget    *budgetSpotsList;
    QSpinBox       *budgetDaysSpin;
    QComboBox      *accommodationCombo;
    QSpinBox       *numPeopleSpin;
    QPushButton    *calculateBudgetBtn;
    QTextEdit      *budgetResultText;
    QPushButton    *saveItineraryBtn;
    QPushButton    *loadItinerariesBtn;
    QPushButton    *proceedToBookingBtn;

    // ---- Tab 4: Route Optimizer (Dijkstra) ----
    QComboBox      *startCityCombo;
    QComboBox      *destCityCombo;
    QPushButton    *findRouteBtn;
    QTextEdit      *routeResultText;

    // ---- Tab 5: Emergency Info ----
    QComboBox      *regionCombo;
    QPushButton    *showEmergencyBtn;
    QTextEdit      *emergencyResultText;

    // ---- Tab 6: My Bookings ----
    QLabel         *bookingAsLabel;       // read-only "Booking as: <name>"
    QComboBox      *bookingTrekCombo;
    QDateEdit      *bookingDateEdit;
    QSpinBox       *bookingPeopleSpin;
    QPushButton    *createBookingBtn;
    QLabel         *bookingResultLabel;

    QTableWidget   *bookingsTable;
    QPushButton    *refreshBookingsBtn;
    QPushButton    *confirmBookingBtn;
    QPushButton    *cancelBookingBtn;
    QComboBox      *assignGuideCombo;
    QPushButton    *assignGuideBtn;

    QTableWidget   *guidesTable;

    QComboBox      *reviewsTrekCombo;
    QPushButton    *loadReviewsBtn;
    QTableWidget   *reviewsTable;
    QLabel         *reviewAsLabel;        // read-only "Reviewing as: <name>"
    QSpinBox       *reviewRatingSpin;
    QLineEdit      *reviewCommentEdit;
    QPushButton    *submitReviewBtn;

    // ---- Tab 7: Profile ----
    QLineEdit      *profileNameEdit;
    QLabel         *profileEmailLabel;
    QLineEdit      *profileNationalityEdit;
    QLineEdit      *profileContactEdit;
    QPushButton    *saveProfileBtn;
    QLabel         *profileStatusLabel;

    QLineEdit      *currentPasswordEdit;
    QLineEdit      *newPasswordEdit;
    QLineEdit      *confirmPasswordEdit;
    QPushButton    *changePasswordBtn;
    QLabel         *passwordStatusLabel;

    QLabel         *profileTripLabel;     // shows the current trip, kept in sync via refreshProfileTripSummary()
    QPushButton    *logoutBtn;

    QWidget* setupProfileTab();

    // Build each tab, return the finished page widget
    QWidget* setupSpotsTab();
    QWidget* setupTrekTab();
    QWidget* setupBudgetTab();
    QWidget* setupRouteTab();
    QWidget* setupEmergencyTab();
    QWidget* setupBookingsTab();

    // Helpers
    void populateSpotsTable(const std::vector<TouristSpot>& spots);
    void populateTreksTable(const std::vector<TrekRoute>& treks);
    void populateBookingsTable(const std::vector<BookingDetail>& bookings);
    void populateGuidesTable(const std::vector<Guide>& guides);
    void populateReviewsTable(const std::vector<Review>& reviews);
    void refreshAvailableGuideCombo();     // repopulates assignGuideCombo with only free guides
    void styleTable(QTableWidget *table);
};

#endif // MAINWINDOW_H