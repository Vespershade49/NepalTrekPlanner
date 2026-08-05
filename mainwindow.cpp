#include "mainwindow.h"
#include "data.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QStringList>
#include <QDateTime>
#include <QGroupBox>
#include <QScrollArea>
#include <QDialog>

MainWindow::MainWindow(int userId, QString userName, QString userEmail, QWidget *parent)
    : QMainWindow(parent), currentUserId(userId), currentUserName(userName), currentUserEmail(userEmail)
{
    setWindowTitle("Nepal Tourism & Trek Planner");
    resize(1200, 820);

    loadAllData(backend);
    loadExistingTripFromDatabase();

    // ── Central layout: a persistent "Your Trip" bar sits above the tabs
    //    so whatever the user has picked stays visible no matter which
    //    tab they're on. ─────────────────────────────────────────────
    QWidget *central = new QWidget(this);
    QVBoxLayout *centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(6);

    tripBar = new QFrame;
    tripBar->setStyleSheet(
        "QFrame { background-color: #23342f; border-bottom: 2px solid #2f7d6c; }"
        );
    QHBoxLayout *tripBarLayout = new QHBoxLayout(tripBar);
    tripBarLayout->setContentsMargins(16, 14, 16, 14);

    tripBarLabel = new QLabel("No trip planned yet - pick a trek in Budget Calculator to get started.");
    tripBarLabel->setStyleSheet("color: #cfe8e0; font-weight: bold;");

    tripBarLayout->addWidget(tripBarLabel, 1);

    tabs = new QTabWidget;
    tabs->addTab(setupSpotsTab(),     "Explore");
    tabs->addTab(setupBudgetTab(),    "Budget Calculator");
    tabs->addTab(setupRouteTab(),     "Route Finder");
    tabs->addTab(setupEmergencyTab(), "Emergency Info");
    tabs->addTab(setupBookingsTab(),  "My Bookings");
    tabs->addTab(setupProfileTab(),   "Profile");

    centralLayout->addWidget(tripBar);
    centralLayout->addWidget(tabs);
    setCentralWidget(central);

    // Now that tripBarLabel, profileTripLabel, etc. all exist, refresh
    // them to reflect whatever loadExistingTripFromDatabase() found -
    // otherwise a real, already-confirmed booking would sit in the
    // database while every part of the UI still claimed "No trip
    // planned yet."
    updateTripBar();
    applyTripToBookingTab(); // sync Bookings/Reviews trek combos right away too, in case a trip already exists

    populateSpotsTable(backend.touristSpots);

    // Whenever the Route Finder or My Bookings tab is opened, quietly
    // pre-fill it based on whatever trek is currently selected.
    connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 2) applyTripToRouteTab();
        if (index == 4) applyTripToBookingTab();
        if (index == 5) refreshProfileTripSummary();
    });
}

MainWindow::~MainWindow() {}

// ══════════════════════════════════════════════════════════════
// TAB 7: Profile - account info, password, current trip, logout
// ══════════════════════════════════════════════════════════════
QWidget* MainWindow::setupProfileTab()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);

    // ── Account info ────────────────────────────────────────────
    QGroupBox *infoGroup = new QGroupBox("Account Information");
    QFormLayout *infoForm = new QFormLayout(infoGroup);

    MySqlManager::UserProfileData data = mysqlManager.fetchUserProfile(currentUserId);

    profileNameEdit = new QLineEdit(data.name.isEmpty() ? currentUserName : data.name);
    profileEmailLabel = new QLabel(data.email.isEmpty() ? currentUserEmail : data.email);
    profileEmailLabel->setStyleSheet("color: #888891;");
    profileNationalityEdit = new QLineEdit(data.nationality);
    profileNationalityEdit->setPlaceholderText("e.g. Nepal");
    profileContactEdit = new QLineEdit(data.contact);
    profileContactEdit->setPlaceholderText("Phone number");

    infoForm->addRow("Name:", profileNameEdit);
    infoForm->addRow("Email:", profileEmailLabel);
    infoForm->addRow("Nationality:", profileNationalityEdit);
    infoForm->addRow("Contact:", profileContactEdit);

    saveProfileBtn = new QPushButton("Save Changes");
    saveProfileBtn->setMaximumWidth(180);
    profileStatusLabel = new QLabel("");
    profileStatusLabel->setWordWrap(true);
    QHBoxLayout *saveProfileRow = new QHBoxLayout;
    saveProfileRow->addWidget(saveProfileBtn);
    saveProfileRow->addStretch();
    infoForm->addRow(saveProfileRow);
    infoForm->addRow(profileStatusLabel);

    connect(saveProfileBtn, &QPushButton::clicked, this, &MainWindow::onSaveProfile);

    // ── Change password ─────────────────────────────────────────
    QGroupBox *passwordGroup = new QGroupBox("Change Password");
    QFormLayout *passwordForm = new QFormLayout(passwordGroup);

    currentPasswordEdit = new QLineEdit;
    currentPasswordEdit->setEchoMode(QLineEdit::Password);
    newPasswordEdit = new QLineEdit;
    newPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit = new QLineEdit;
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);

    passwordForm->addRow("Current password:", currentPasswordEdit);
    passwordForm->addRow("New password:", newPasswordEdit);
    passwordForm->addRow("Confirm new password:", confirmPasswordEdit);

    changePasswordBtn = new QPushButton("Update Password");
    changePasswordBtn->setMaximumWidth(180);
    passwordStatusLabel = new QLabel("");
    passwordStatusLabel->setWordWrap(true);
    QHBoxLayout *changePasswordRow = new QHBoxLayout;
    changePasswordRow->addWidget(changePasswordBtn);
    changePasswordRow->addStretch();
    passwordForm->addRow(changePasswordRow);
    passwordForm->addRow(passwordStatusLabel);

    connect(changePasswordBtn, &QPushButton::clicked, this, &MainWindow::onChangePassword);

    // ── Current trip summary ────────────────────────────────────
    QGroupBox *tripGroup = new QGroupBox("Your Current Trip");
    QVBoxLayout *tripLayout = new QVBoxLayout(tripGroup);
    profileTripLabel = new QLabel("No trip planned yet.");
    profileTripLabel->setWordWrap(true);
    tripLayout->addWidget(profileTripLabel);

    // ── Log out ──────────────────────────────────────────────────
    logoutBtn = new QPushButton("Log Out");
    logoutBtn->setStyleSheet("background-color: #7a3f3f;");
    logoutBtn->setMaximumWidth(180);
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogout);

    layout->addWidget(infoGroup);
    layout->addWidget(passwordGroup);
    layout->addWidget(tripGroup);
    layout->addStretch();
    QHBoxLayout *logoutRow = new QHBoxLayout;
    logoutRow->addWidget(logoutBtn);
    logoutRow->addStretch();
    layout->addLayout(logoutRow);

    // Wrapped in a scroll area so Logout is always reachable, even on
    // a smaller screen or with a long trip summary.
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(page);
    return scroll;
}

void MainWindow::onSaveProfile()
{
    QString errorMessage;
    // Empty current/new password here means "don't touch the password" -
    // that's handled separately by the Change Password section below.
    bool ok = mysqlManager.updateUserProfile(
        currentUserId, profileNameEdit->text(), profileNationalityEdit->text(),
        profileContactEdit->text(), "", "", errorMessage);

    if (!ok) {
        profileStatusLabel->setStyleSheet("color: #e07a5f;");
        profileStatusLabel->setText(errorMessage);
        return;
    }

    currentUserName = profileNameEdit->text();
    bookingAsLabel->setText(currentUserName);
    reviewAsLabel->setText(currentUserName);
    profileStatusLabel->setStyleSheet("color: #4fb8a2;");
    profileStatusLabel->setText("Saved.");
}

void MainWindow::onChangePassword()
{
    if (currentPasswordEdit->text().isEmpty() || newPasswordEdit->text().isEmpty()) {
        passwordStatusLabel->setStyleSheet("color: #e07a5f;");
        passwordStatusLabel->setText("Enter your current and new password.");
        return;
    }
    if (newPasswordEdit->text() != confirmPasswordEdit->text()) {
        passwordStatusLabel->setStyleSheet("color: #e07a5f;");
        passwordStatusLabel->setText("New passwords don't match.");
        return;
    }

    QString errorMessage;
    bool ok = mysqlManager.updateUserProfile(
        currentUserId, profileNameEdit->text(), profileNationalityEdit->text(),
        profileContactEdit->text(), currentPasswordEdit->text(), newPasswordEdit->text(),
        errorMessage);

    if (!ok) {
        passwordStatusLabel->setStyleSheet("color: #e07a5f;");
        passwordStatusLabel->setText(errorMessage);
        return;
    }

    currentPasswordEdit->clear();
    newPasswordEdit->clear();
    confirmPasswordEdit->clear();
    passwordStatusLabel->setStyleSheet("color: #4fb8a2;");
    passwordStatusLabel->setText("Password updated.");
}

void MainWindow::refreshProfileTripSummary()
{
    if (!profileTripLabel) return;

    if (!currentTrip.hasTrek) {
        profileTripLabel->setText("No trip planned yet - pick a trek in Budget Calculator to get started.");
        return;
    }

    QString status = currentTrip.isBooked
                         ? "<span style='color:#4fb8a2;'>&#10003; Booked</span>"
                         : "<span style='color:#e0b05f;'>Draft - not booked yet</span>";

    QString text = QString("<b>%1</b> &middot; %2<br>%3 day(s), %4 people")
                       .arg(QString::fromStdString(currentTrip.trek.name))
                       .arg(status)
                       .arg(currentTrip.days)
                       .arg(currentTrip.people);

    if (currentTrip.hasCalculatedCost) {
        text += QString("<br><b>Estimated total: %1 NPR</b>").arg(currentTrip.totalCost, 0, 'f', 0);
        if (currentTrip.guideFee > 0)
            text += QString(" <span style='color:#888891;'>(includes %1 NPR guide fee)</span>")
                        .arg(currentTrip.guideFee, 0, 'f', 0);
    }

    profileTripLabel->setText(text);
}

void MainWindow::onLogout()
{
    int choice = QMessageBox::question(this, "Log Out", "Log out of your account?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (choice != QMessageBox::Yes) return;

    emit logoutRequested();
    close();
}

// ══════════════════════════════════════════════════════════════
// Trip-linking helpers - this is what ties the tabs together
// ══════════════════════════════════════════════════════════════
// Bookings are permanent (stored in MySQL); currentTrip is not - it's
// just in-memory state for whatever's being actively planned this
// session. Without this, a user who already has a real Confirmed
// booking would see "No trip planned yet" everywhere the moment they
// log back in, which looks like the app forgot about a booking that's
// still sitting right there in the database.
//
// Note: the Bookings table doesn't store which tourist spots were
// selected or how many days were planned for - only start_date,
// num_people, and the final total_cost. So this reconstructs a
// reasonable approximation (days defaults to the trek's standard
// length) rather than the exact original selections, which aren't
// persisted anywhere.
void MainWindow::loadExistingTripFromDatabase()
{
    std::vector<BookingDetail> bookings = mysqlManager.fetchBookingDetailsForUser(currentUserId);

    for (const BookingDetail &b : bookings) {
        if (b.status != "Pending" && b.status != "Confirmed") continue;

        for (const TrekRoute &t : backend.trekRoutes) {
            if (t.name == b.trekName) {
                currentTrip.hasTrek = true;
                currentTrip.trek = t;
                currentTrip.days = t.daysRequired > 0 ? t.daysRequired : 7;
                currentTrip.people = b.numPeople;
                currentTrip.hasCalculatedCost = true;
                currentTrip.totalCost = b.totalCost;
                currentTrip.isBooked = true;
                return; // most relevant one found - stop looking
            }
        }
    }
}

void MainWindow::updateTripBar()
{
    if (!currentTrip.hasTrek) {
        tripBarLabel->setText("No trip planned yet - pick a trek in Budget Calculator to get started.");
        return;
    }

    QString status = currentTrip.isBooked
                         ? "<span style='color:#4fb8a2;'>&#10003; Booked</span>"
                         : "<span style='color:#e0b05f;'>Draft - not booked yet</span>";

    QString text = QString("Your Trip: %1  ·  %2 day(s)  ·  %3 people  ·  %4")
                       .arg(QString::fromStdString(currentTrip.trek.name))
                       .arg(currentTrip.days)
                       .arg(currentTrip.people)
                       .arg(status);

    if (currentTrip.hasCalculatedCost) {
        text += QString("  ·  ~%1 NPR").arg(currentTrip.totalCost, 0, 'f', 0);
        if (currentTrip.guideFee > 0)
            text += QString(" (incl. %1 NPR guide fee)").arg(currentTrip.guideFee, 0, 'f', 0);
    }

    tripBarLabel->setText(text);
    refreshProfileTripSummary();
}

void MainWindow::applyTripToBudgetTab()
{
    if (!currentTrip.hasTrek) return;

    int index = budgetTrekCombo->findText(QString::fromStdString(currentTrip.trek.name));
    if (index >= 0)
        budgetTrekCombo->setCurrentIndex(index);

    budgetDaysSpin->setValue(currentTrip.days);
    numPeopleSpin->setValue(currentTrip.people);
}

void MainWindow::applyTripToBookingTab()
{
    if (!currentTrip.hasTrek) return;

    int index = bookingTrekCombo->findText(QString::fromStdString(currentTrip.trek.name));
    if (index >= 0)
        bookingTrekCombo->setCurrentIndex(index);

    bookingPeopleSpin->setValue(currentTrip.people);

    // The Reviews trek dropdown otherwise silently sits on whichever
    // trek happens to be first in the list - if that's not the one
    // you actually booked, "Submit Review" fails the "only review
    // treks you've booked" rule with no obvious reason why. Default it
    // to the trip you're actually tracking instead.
    int reviewIndex = reviewsTrekCombo->findText(QString::fromStdString(currentTrip.trek.name));
    if (reviewIndex >= 0)
        reviewsTrekCombo->setCurrentIndex(reviewIndex);
}

// ══════════════════════════════════════════════════════════════
// TAB 1: Explore Tourist Spots
// ══════════════════════════════════════════════════════════════
QWidget* MainWindow::setupSpotsTab()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);

    QHBoxLayout *filterRow = new QHBoxLayout;

    categoryCombo = new QComboBox;
    categoryCombo->addItems({"All", "Adventure", "Cultural", "Religious", "Nature"});

    nameSearchEdit = new QLineEdit;
    nameSearchEdit->setPlaceholderText("Search by name...");

    searchSpotsBtn = new QPushButton("Search");

    filterRow->addWidget(new QLabel("Category:"));
    filterRow->addWidget(categoryCombo);
    filterRow->addWidget(nameSearchEdit);
    filterRow->addWidget(searchSpotsBtn);

    spotsTable = new QTableWidget;
    styleTable(spotsTable);
    spotsTable->setColumnCount(6);
    spotsTable->setHorizontalHeaderLabels(
        {"Name", "Location", "Category", "Best Time", "Entry Fee (NPR)", "Description"});
    spotsTable->horizontalHeader()->setStretchLastSection(true);
    spotsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    spotsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    layout->addLayout(filterRow);
    layout->addWidget(spotsTable);

    connect(searchSpotsBtn, &QPushButton::clicked, this, &MainWindow::onSearchSpots);

    return page;
}

void MainWindow::populateSpotsTable(const std::vector<TouristSpot>& spots)
{
    spotsTable->setRowCount((int)spots.size());
    for (int i = 0; i < (int)spots.size(); i++) {
        const TouristSpot &s = spots[i];
        spotsTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(s.name)));
        spotsTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(s.location)));
        spotsTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(s.category)));
        spotsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(s.bestTime)));
        spotsTable->setItem(i, 4, new QTableWidgetItem(QString::number(s.entryFee, 'f', 0)));
        spotsTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(s.description)));
    }
    spotsTable->resizeColumnsToContents();
}

void MainWindow::onSearchSpots()
{
    QString wantedCategory = categoryCombo->currentText();
    QString typedName = nameSearchEdit->text().toLower();

    std::vector<TouristSpot> results;

    for (const TouristSpot &spot : backend.touristSpots) {
        bool categoryMatches = (wantedCategory == "All")
        || (QString::fromStdString(spot.category) == wantedCategory);

        bool nameMatches = typedName.isEmpty()
                           || QString::fromStdString(spot.name).toLower().contains(typedName);

        if (categoryMatches && nameMatches)
            results.push_back(spot);
    }

    populateSpotsTable(results);
}

// ══════════════════════════════════════════════════════════════
// TAB 3: Budget Calculator
// ══════════════════════════════════════════════════════════════

QWidget* MainWindow::setupBudgetTab()
{
    QWidget *page = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(page);

    QWidget *left = new QWidget;
    QFormLayout *form = new QFormLayout(left);

    budgetTrekCombo = new QComboBox;
    for (const TrekRoute &t : backend.trekRoutes)
        budgetTrekCombo->addItem(QString::fromStdString(t.name));

    budgetDaysSpin = new QSpinBox;
    budgetDaysSpin->setRange(1, 60);
    budgetDaysSpin->setValue(7);

    accommodationCombo = new QComboBox;
    accommodationCombo->addItems({"budget", "standard", "luxury"});

    numPeopleSpin = new QSpinBox;
    numPeopleSpin->setRange(1, 20);
    numPeopleSpin->setValue(1);

    budgetLimitSpin = new QDoubleSpinBox;
    budgetLimitSpin->setRange(0, 2000000);
    budgetLimitSpin->setSingleStep(5000);
    budgetLimitSpin->setValue(0);
    budgetLimitSpin->setSuffix(" NPR");

    calculateBudgetBtn = new QPushButton("Calculate Total Cost");
    saveItineraryBtn = new QPushButton("Save Itinerary");
    loadItinerariesBtn = new QPushButton("View Saved Itineraries");
    proceedToBookingBtn = new QPushButton("Book This Trip \u2192");
    proceedToBookingBtn->setEnabled(false);

    form->addRow("Trek route:", budgetTrekCombo);
    form->addRow("Number of days:", budgetDaysSpin);
    form->addRow("Accommodation type:", accommodationCombo);
    form->addRow("Number of people:", numPeopleSpin);
    form->addRow("Your budget (optional):", budgetLimitSpin);
    form->addRow(calculateBudgetBtn);
    form->addRow(saveItineraryBtn);
    form->addRow(loadItinerariesBtn);
    form->addRow(proceedToBookingBtn);

    budgetResultText = new QTextEdit;
    budgetResultText->setReadOnly(true);
    budgetResultText->setPlaceholderText("Cost breakdown will appear here.");

    layout->addWidget(left, 2);
    layout->addWidget(budgetResultText, 1);

    connect(calculateBudgetBtn, &QPushButton::clicked, this, &MainWindow::onCalculateBudget);
    connect(saveItineraryBtn, &QPushButton::clicked, this, &MainWindow::onSaveItinerary);
    connect(loadItinerariesBtn, &QPushButton::clicked, this, &MainWindow::onLoadItineraries);
    connect(proceedToBookingBtn, &QPushButton::clicked, this, &MainWindow::onProceedToBooking);

    return page;
}

void MainWindow::onCalculateBudget()
{
    if (backend.trekRoutes.empty()) {
        QMessageBox::warning(this, "No Data", "No trek routes are loaded.");
        return;
    }

    int trekIndex = budgetTrekCombo->currentIndex();
    TrekRoute selectedTrek = backend.trekRoutes[trekIndex];

    BudgetCalculator::CostBreakdown b = backend.calculateBudget(
        selectedTrek,
        budgetDaysSpin->value(),
        accommodationCombo->currentText().toStdString(),
        numPeopleSpin->value()
        );

    // This is the moment the trip becomes "real" - remember it so
    // Save Itinerary and My Bookings can both use it. A fresh
    // calculation is always a new draft, not yet an actual booking -
    // isBooked only flips true once onCreateBooking() actually
    // succeeds (or on startup, if this matches an existing DB booking).
    // A fresh calculation also means any guide fee from a previous
    // trip no longer applies - that only gets added back once a guide
    // is actually assigned to this trip's real booking.
    currentTrip.hasTrek = true;
    currentTrip.trek = selectedTrek;
    currentTrip.days = budgetDaysSpin->value();
    currentTrip.people = numPeopleSpin->value();
    currentTrip.accommodation = accommodationCombo->currentText().toStdString();
    currentTrip.hasCalculatedCost = true;
    currentTrip.totalCost = b.totalNPR;
    currentTrip.guideFee = 0;
    currentTrip.budgetLimit = budgetLimitSpin->value();
    currentTrip.isBooked = false;

    bool overBudget = currentTrip.budgetLimit > 0 && currentTrip.totalCost > currentTrip.budgetLimit;
    proceedToBookingBtn->setEnabled(!overBudget);
    proceedToBookingBtn->setToolTip(overBudget
                                        ? "This trip is over your entered budget - adjust it and recalculate to book."
                                        : "");
    updateTripBar();

    // Built as HTML so the actual TOTAL stands out clearly instead of
    // looking like just another line item (that's what was confusing
    // about "Trek cost" before - it's only one component, not the total).
    QString html;
    html += QString("<h3 style='color:#4fb8a2;'>%1</h3>").arg(QString::fromStdString(selectedTrek.name));
    html += "<table style='width:100%; font-size:13px;'>";
    html += QString("<tr><td>Trek base cost</td><td align='right'>%1 NPR</td></tr>").arg(b.trekCost, 0, 'f', 0);
    html += QString("<tr><td>Accommodation</td><td align='right'>%1 NPR</td></tr>").arg(b.accommodation, 0, 'f', 0);
    html += QString("<tr><td>Food</td><td align='right'>%1 NPR</td></tr>").arg(b.food, 0, 'f', 0);
    html += QString("<tr><td>Transport</td><td align='right'>%1 NPR</td></tr>").arg(b.transport, 0, 'f', 0);
    html += "</table><hr>";
    html += QString("<p style='font-size:18px; font-weight:bold; color:#4fb8a2;'>Total: %1 NPR</p>")
                .arg(b.totalNPR, 0, 'f', 0);
    html += QString("<p style='color:#a8a8b0;'>(~%1 USD)</p>").arg(b.totalUSD, 0, 'f', 2);

    // Budget check - only runs if the user actually entered a limit
    // (0 means "no limit", so it's skipped entirely).
    double budgetLimit = budgetLimitSpin->value();
    if (budgetLimit > 0) {
        html += "<hr>";
        if (b.totalNPR <= budgetLimit) {
            double spare = budgetLimit - b.totalNPR;
            html += QString("<p style='color:#4fb8a2; font-weight:bold;'>"
                            "\u2713 Within your %1 NPR budget - %2 NPR to spare.</p>")
                        .arg(budgetLimit, 0, 'f', 0)
                        .arg(spare, 0, 'f', 0);
        } else {
            double over = b.totalNPR - budgetLimit;
            html += QString("<p style='color:#e07a5f; font-weight:bold;'>"
                            "\u2717 This exceeds your %1 NPR budget by %2 NPR.</p>")
                        .arg(budgetLimit, 0, 'f', 0)
                        .arg(over, 0, 'f', 0);

            // Suggest other treks that would actually fit, using the
            // same days/accommodation/people.
            std::vector<std::pair<TrekRoute, double>> affordable;
            for (const TrekRoute &t : backend.trekRoutes) {
                if (t.trekId == selectedTrek.trekId) continue;
                double cost = backend.calculateBudget(
                                         t, budgetDaysSpin->value(),
                                         accommodationCombo->currentText().toStdString(),
                                         numPeopleSpin->value()
                                         ).totalNPR;
                if (cost <= budgetLimit)
                    affordable.push_back({t, cost});
            }
            std::sort(affordable.begin(), affordable.end(),
                      [](const auto &a, const auto &b) { return a.second < b.second; });

            if (!affordable.empty()) {
                html += "<p style='color:#888891; font-size:11px;'>Treks that would likely fit "
                        "(same days/accommodation/people):</p>";
                html += "<ul style='font-size:12px; color:#cfcfd6;'>";
                for (const auto &pair : affordable) {
                    html += QString("<li>%1 - about %2 NPR</li>")
                    .arg(QString::fromStdString(pair.first.name))
                        .arg(pair.second, 0, 'f', 0);
                }
                html += "</ul>";
            } else {
                html += "<p style='color:#888891; font-size:11px;'>No other trek fits this budget "
                        "at this length/group size either - try fewer days, fewer people, or a "
                        "cheaper accommodation type.</p>";
            }
        }
    }

    budgetResultText->setHtml(html);
}

void MainWindow::onProceedToBooking()
{
    if (!currentTrip.hasCalculatedCost) {
        QMessageBox::information(this, "Calculate First",
                                 "Calculate your trip's cost first, then you can book it.");
        return;
    }

    // Hard stop: if a budget limit was set and this trip goes over it,
    // don't let the user book it at all. They need to actually bring
    // the trip under budget (fewer days/people, cheaper accommodation,
    // a different trek) and recalculate, rather than just clicking
    // through a warning.
    if (currentTrip.budgetLimit > 0 && currentTrip.totalCost > currentTrip.budgetLimit) {
        double over = currentTrip.totalCost - currentTrip.budgetLimit;
        QMessageBox::warning(this, "Over Budget",
                             QString("This trip costs %1 NPR, which is %2 NPR over your %3 NPR budget.\n\n"
                                     "You can't book it as-is - try fewer days, fewer people, a cheaper "
                                     "accommodation type, or a different trek, then recalculate.")
                                 .arg(currentTrip.totalCost, 0, 'f', 0)
                                 .arg(over, 0, 'f', 0)
                                 .arg(currentTrip.budgetLimit, 0, 'f', 0));
        return;
    }

    applyTripToBookingTab();
    tabs->setCurrentIndex(4); // My Bookings tab
}

// ══════════════════════════════════════════════════════════════
// TAB 4: Route Optimizer (Dijkstra)
// ══════════════════════════════════════════════════════════════
QWidget* MainWindow::setupRouteTab()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);

    QFormLayout *form = new QFormLayout;

    startCityCombo = new QComboBox;
    destCityCombo = new QComboBox;

    std::vector<string> cities = routeOptimizer.getAllCities();
    QStringList cityList;
    for (const string &c : cities)
        cityList << QString::fromStdString(c);
    cityList.sort();

    startCityCombo->addItems(cityList);
    destCityCombo->addItems(cityList);

    // Kathmandu is the starting point for practically every trek in
    // Nepal, so default it there - the user only ever needs to change
    // "From" if they're genuinely starting somewhere else.
    int kathmanduIndex = startCityCombo->findText("Kathmandu");
    if (kathmanduIndex >= 0) startCityCombo->setCurrentIndex(kathmanduIndex);

    findRouteBtn = new QPushButton("Find Cheapest Route");

    routeBudgetSpin = new QDoubleSpinBox;
    routeBudgetSpin->setRange(0, 1000000);
    routeBudgetSpin->setSingleStep(500);
    routeBudgetSpin->setValue(0);
    routeBudgetSpin->setSuffix(" NPR");
    routeBudgetSpin->setSpecialValueText("No limit");

    form->addRow("From:", startCityCombo);
    form->addRow("To:", destCityCombo);
    form->addRow("Max budget (optional):", routeBudgetSpin);
    form->addRow(findRouteBtn);

    routeResultText = new QTextEdit;
    routeResultText->setReadOnly(true);
    routeResultText->setPlaceholderText("Route details will appear here.");

    layout->addLayout(form);
    layout->addWidget(routeResultText);

    connect(findRouteBtn, &QPushButton::clicked, this, &MainWindow::onFindRoute);

    return page;
}

// Maps a trek's name to its actual endpoint in the RouteOptimizer's
// city graph, so the "To" field fills itself in once a trek is chosen
// instead of the user having to guess which city corresponds to it.
void MainWindow::applyTripToRouteTab()
{
    if (!currentTrip.hasTrek) return;

    QString name = QString::fromStdString(currentTrip.trek.name);
    QString destination;

    if (name.contains("Everest", Qt::CaseInsensitive))
        destination = "EBC";
    else if (name.contains("Annapurna", Qt::CaseInsensitive) && name.contains("Circuit", Qt::CaseInsensitive))
        destination = "Muktinath";
    else if (name.contains("Poon Hill", Qt::CaseInsensitive))
        destination = "Poon Hill";
    else if (name.contains("Langtang", Qt::CaseInsensitive))
        destination = "Kyanjin Gompa";
    else if (name.contains("Mustang", Qt::CaseInsensitive))
        destination = "Lo Manthang";

    if (destination.isEmpty()) return; // no mapping for this trek yet - leave as-is

    int index = destCityCombo->findText(destination);
    if (index >= 0) {
        destCityCombo->setCurrentIndex(index);
        // Calculate right away instead of leaving "Route details will
        // appear here" showing until the user clicks the button - the
        // destination is already known at this point.
        onFindRoute();
    }
}

void MainWindow::onFindRoute()
{
    string start = startCityCombo->currentText().toStdString();
    string dest = destCityCombo->currentText().toStdString();

    // routeBudgetSpin's special value (0, displayed as "No limit")
    // means "don't constrain the search" - pass -1 through in that case.
    double budget = (routeBudgetSpin->value() <= 0) ? -1 : routeBudgetSpin->value();

    RouteResult result = routeOptimizer.findCheapestRoute(start, dest, budget);

    if (!result.found) {
        if (result.budgetLimited)
            routeResultText->setText(
                QString("A route exists, but every option costs more than your %1 NPR budget. "
                        "Try raising the budget or picking a nearer destination.")
                    .arg(routeBudgetSpin->value(), 0, 'f', 0));
        else
            routeResultText->setText("No route found between these two locations.");
        return;
    }

    QString pathStr;
    for (size_t i = 0; i < result.path.size(); i++) {
        pathStr += QString::fromStdString(result.path[i]);
        if (i != result.path.size() - 1)
            pathStr += "  ->  ";
    }

    QString output;
    output += "Route:\n" + pathStr + "\n\n";
    output += QString("Total transport cost: %1 NPR\n").arg(result.totalCost, 0, 'f', 0);
    output += QString("Total distance:       %1 km\n").arg(result.totalDistance, 0, 'f', 0);

    // AMS / altitude-safety warnings - flags any single hop along the
    // route that gains more than RouteOptimizer::AMS_SAFE_ASCENT_METERS.
    if (!result.amsWarnings.empty()) {
        output += "\n\u26a0 Altitude sickness (AMS) risk:\n";
        for (const AmsWarning &w : result.amsWarnings) {
            output += QString("  %1 -> %2: +%3 m in one stage (over the %4 m/day safe limit)\n")
            .arg(QString::fromStdString(w.fromCity))
                .arg(QString::fromStdString(w.toCity))
                .arg(w.ascentMeters, 0, 'f', 0)
                .arg(RouteOptimizer::AMS_SAFE_ASCENT_METERS, 0, 'f', 0);
        }
        output += "Consider adding an acclimatization day before this stage.\n";
    }

    routeResultText->setText(output);
}

// ══════════════════════════════════════════════════════════════
// TAB 5: Emergency Info
// ══════════════════════════════════════════════════════════════
QWidget* MainWindow::setupEmergencyTab()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);

    QHBoxLayout *row = new QHBoxLayout;
    regionCombo = new QComboBox;
    for (const string &r : backend.getAllEmergencyRegions())
        regionCombo->addItem(QString::fromStdString(r));

    showEmergencyBtn = new QPushButton("Show Emergency Info");

    row->addWidget(new QLabel("Region:"));
    row->addWidget(regionCombo);
    row->addWidget(showEmergencyBtn);

    emergencyResultText = new QTextEdit;
    emergencyResultText->setReadOnly(true);
    emergencyResultText->setPlaceholderText("Emergency contact details will appear here.");

    layout->addLayout(row);
    layout->addWidget(emergencyResultText);

    connect(showEmergencyBtn, &QPushButton::clicked, this, &MainWindow::onShowEmergency);

    return page;
}

void MainWindow::onShowEmergency()
{
    string region = regionCombo->currentText().toStdString();
    EmergencyInfo info = backend.getEmergencyInfo(region);

    if (info.contactName.empty()) {
        emergencyResultText->setText("No emergency info found for this region.");
        return;
    }

    QString output;
    output += "Region:   " + QString::fromStdString(info.region) + "\n";
    output += "Contact:  " + QString::fromStdString(info.contactName) + "\n";
    output += "Phone:    " + QString::fromStdString(info.phoneNumber) + "\n";
    output += "Hospital: " + QString::fromStdString(info.hospitalName) + "\n\n";
    output += "Altitude sickness warning:\n" + QString::fromStdString(info.altitudeSicknessWarning);

    emergencyResultText->setText(output);
}

// ══════════════════════════════════════════════════════════════
// Save / Load Itinerary
// ══════════════════════════════════════════════════════════════
void MainWindow::onSaveItinerary()
{
    if (!currentTrip.hasCalculatedCost) {
        QMessageBox::warning(this, "Nothing to Save",
                             "Calculate a budget first, then save it.");
        return;
    }

    string timestamp = QDateTime::currentDateTime()
                           .toString("yyyy-MM-dd hh:mm:ss").toStdString();

    Itinerary itinerary = backend.createItinerary(
        currentUserId,
        currentTrip.trek,
        {},
        currentTrip.totalCost,
        timestamp
        );

    QString text;
    text += QString("Itinerary #%1 - %2\n").arg(itinerary.itineraryId).arg(currentUserName);
    text += QString("Total Cost: %1 NPR\n").arg(itinerary.totalCost, 0, 'f', 0);
    text += QString("Created: %1\n\n").arg(QString::fromStdString(itinerary.createdDate));
    text += "Trek:\n";
    text += QString("  - %1 (%2 days)\n").arg(QString::fromStdString(currentTrip.trek.name)).arg(currentTrip.days);
    text += "\nFull Cost Breakdown:\n" + budgetResultText->toPlainText();

    bool ok = backend.saveItinerary(text.toStdString(), timestamp);

    if (ok)
        QMessageBox::information(this, "Saved",
                                 QString("Itinerary #%1 saved.").arg(itinerary.itineraryId));
    else
        QMessageBox::critical(this, "Error", "Could not save the itinerary.");
}

void MainWindow::onLoadItineraries()
{
    string content = backend.loadItineraries();

    if (content.empty()) {
        QMessageBox::information(this, "No Saved Itineraries",
                                 "No itineraries found yet. Save one first!");
        return;
    }

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Saved Itineraries");
    dialog->resize(600, 500);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    QTextEdit *textBox = new QTextEdit;
    textBox->setReadOnly(true);
    textBox->setText(QString::fromStdString(content));
    layout->addWidget(textBox);

    QPushButton *closeBtn = new QPushButton("Close");
    layout->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    dialog->exec();
}

// ══════════════════════════════════════════════════════════════
// TAB 6: My Bookings
// ══════════════════════════════════════════════════════════════
QWidget* MainWindow::setupBookingsTab()
{
    QWidget *page = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(page);

    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    QWidget *inner = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(inner);

    // ── Group A: Book a trip ────────────────────────────────────
    QGroupBox *bookingGroup = new QGroupBox("Book a trip");
    QFormLayout *bookingForm = new QFormLayout(bookingGroup);

    bookingAsLabel = new QLabel(currentUserName);
    bookingAsLabel->setStyleSheet("color: #4fb8a2; font-weight: bold;");

    bookingTrekCombo = new QComboBox;
    for (const TrekRoute &t : backend.trekRoutes)
        bookingTrekCombo->addItem(QString::fromStdString(t.name), t.trekId);

    bookingDateEdit = new QDateEdit(QDate::currentDate());
    bookingDateEdit->setCalendarPopup(true);

    bookingPeopleSpin = new QSpinBox;
    bookingPeopleSpin->setRange(1, 20);
    bookingPeopleSpin->setValue(1);
    // Locked on purpose: total cost is calculated in the Budget
    // Calculator for a specific number of people. Editable here too,
    // it could drift from what the displayed total actually covers.
    // It's set from currentTrip.people in applyTripToBookingTab().
    bookingPeopleSpin->setReadOnly(true);
    bookingPeopleSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    bookingPeopleSpin->setFocusPolicy(Qt::NoFocus);

    createBookingBtn = new QPushButton("Confirm Booking");
    bookingResultLabel = new QLabel("");
    bookingResultLabel->setWordWrap(true);

    bookingForm->addRow("Booking as:", bookingAsLabel);
    bookingForm->addRow("Trek:", bookingTrekCombo);
    bookingForm->addRow("Start date:", bookingDateEdit);
    bookingForm->addRow("Number of people:", bookingPeopleSpin);
    bookingForm->addRow(createBookingBtn);
    bookingForm->addRow(bookingResultLabel);

    connect(createBookingBtn, &QPushButton::clicked, this, &MainWindow::onCreateBooking);

    // ── Group B: My bookings ────────────────────────────────────
    QGroupBox *bookingsListGroup = new QGroupBox("My bookings");
    QVBoxLayout *bookingsListLayout = new QVBoxLayout(bookingsListGroup);

    bookingsTable = new QTableWidget;
    styleTable(bookingsTable);
    bookingsTable->setMinimumHeight(180);
    bookingsTable->setColumnCount(10);
    bookingsTable->setHorizontalHeaderLabels(
        {"ID", "User", "Trek", "Region", "Difficulty", "Start Date", "People", "Total Cost", "Status", "Guides"});
    bookingsTable->horizontalHeader()->setStretchLastSection(true);
    bookingsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    bookingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QHBoxLayout *bookingActionsRow = new QHBoxLayout;
    refreshBookingsBtn = new QPushButton("Refresh");
    confirmBookingBtn = new QPushButton("Confirm Selected");
    cancelBookingBtn = new QPushButton("Cancel Selected");
    assignGuideCombo = new QComboBox;
    assignGuideBtn = new QPushButton("Assign Guide to Selected");

    bookingActionsRow->addWidget(refreshBookingsBtn);
    bookingActionsRow->addWidget(confirmBookingBtn);
    bookingActionsRow->addWidget(cancelBookingBtn);
    bookingActionsRow->addWidget(new QLabel("Available guide:"));
    bookingActionsRow->addWidget(assignGuideCombo);
    bookingActionsRow->addWidget(assignGuideBtn);

    bookingsListLayout->addWidget(bookingsTable);
    bookingsListLayout->addLayout(bookingActionsRow);

    connect(refreshBookingsBtn, &QPushButton::clicked, this, &MainWindow::onRefreshBookings);
    connect(confirmBookingBtn, &QPushButton::clicked, this, &MainWindow::onConfirmBooking);
    connect(cancelBookingBtn, &QPushButton::clicked, this, &MainWindow::onCancelBooking);
    connect(assignGuideBtn, &QPushButton::clicked, this, &MainWindow::onAssignGuide);

    // ── Group C: Guides (reference table) ───────────────────────
    QGroupBox *guidesGroup = new QGroupBox("Our Guides");
    QVBoxLayout *guidesLayout = new QVBoxLayout(guidesGroup);
    guidesTable = new QTableWidget;
    styleTable(guidesTable);
    guidesTable->setMinimumHeight(140);
    guidesTable->setColumnCount(7);
    guidesTable->setHorizontalHeaderLabels(
        {"Name", "License", "Languages", "Experience (yrs)", "Rating", "Contact", "Daily Rate (NPR)"});
    guidesTable->horizontalHeader()->setStretchLastSection(true);
    guidesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    guidesLayout->addWidget(guidesTable);

    // ── Group D: Reviews ─────────────────────────────────────────
    QGroupBox *reviewsGroup = new QGroupBox("Reviews");
    QVBoxLayout *reviewsLayout = new QVBoxLayout(reviewsGroup);

    QHBoxLayout *reviewTrekRow = new QHBoxLayout;
    reviewsTrekCombo = new QComboBox;
    for (const TrekRoute &t : backend.trekRoutes)
        reviewsTrekCombo->addItem(QString::fromStdString(t.name), t.trekId);
    loadReviewsBtn = new QPushButton("Load Reviews");
    reviewTrekRow->addWidget(new QLabel("Trek:"));
    reviewTrekRow->addWidget(reviewsTrekCombo);
    reviewTrekRow->addWidget(loadReviewsBtn);

    reviewsTable = new QTableWidget;
    styleTable(reviewsTable);
    reviewsTable->setMinimumHeight(140);
    reviewsTable->setColumnCount(4);
    reviewsTable->setHorizontalHeaderLabels({"User", "Rating", "Comment", "Date"});
    reviewsTable->horizontalHeader()->setStretchLastSection(true);
    reviewsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QFormLayout *addReviewForm = new QFormLayout;
    reviewAsLabel = new QLabel(currentUserName);
    reviewAsLabel->setStyleSheet("color: #4fb8a2; font-weight: bold;");
    reviewRatingSpin = new QSpinBox;
    reviewRatingSpin->setRange(1, 5);
    reviewRatingSpin->setValue(5);
    reviewCommentEdit = new QLineEdit;
    reviewCommentEdit->setPlaceholderText("Write a short comment...");
    submitReviewBtn = new QPushButton("Submit Review");

    addReviewForm->addRow("Reviewing as:", reviewAsLabel);
    addReviewForm->addRow("Rating:", reviewRatingSpin);
    addReviewForm->addRow("Comment:", reviewCommentEdit);
    addReviewForm->addRow(submitReviewBtn);

    reviewsLayout->addLayout(reviewTrekRow);
    reviewsLayout->addWidget(reviewsTable);
    reviewsLayout->addLayout(addReviewForm);

    connect(loadReviewsBtn, &QPushButton::clicked, this, &MainWindow::onLoadReviews);
    connect(submitReviewBtn, &QPushButton::clicked, this, &MainWindow::onSubmitReview);

    layout->addWidget(bookingGroup);
    layout->addWidget(bookingsListGroup);
    layout->addWidget(guidesGroup);
    layout->addWidget(reviewsGroup);

    scroll->setWidget(inner);
    outer->addWidget(scroll);

    populateBookingsTable(mysqlManager.fetchBookingDetailsForUser(currentUserId));
    populateGuidesTable(mysqlManager.fetchGuides());
    refreshAvailableGuideCombo();

    return page;
}

void MainWindow::populateBookingsTable(const std::vector<BookingDetail>& bookings)
{
    bookingsTable->setRowCount((int)bookings.size());
    for (int i = 0; i < (int)bookings.size(); i++) {
        const BookingDetail &b = bookings[i];
        bookingsTable->setItem(i, 0, new QTableWidgetItem(QString::number(b.bookingId)));
        bookingsTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(b.userName)));
        bookingsTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(b.trekName)));
        bookingsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(b.region)));
        bookingsTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(b.difficultyLevel)));
        bookingsTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(b.startDate)));
        bookingsTable->setItem(i, 6, new QTableWidgetItem(QString::number(b.numPeople)));
        bookingsTable->setItem(i, 7, new QTableWidgetItem(QString::number(b.totalCost, 'f', 2)));
        bookingsTable->setItem(i, 8, new QTableWidgetItem(QString::fromStdString(b.status)));
        bookingsTable->setItem(i, 9, new QTableWidgetItem(QString::fromStdString(b.assignedGuides)));
    }
    bookingsTable->resizeColumnsToContents();
}

void MainWindow::populateGuidesTable(const std::vector<Guide>& guides)
{
    guidesTable->setRowCount((int)guides.size());
    for (int i = 0; i < (int)guides.size(); i++) {
        const Guide &g = guides[i];
        guidesTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(g.name)));
        guidesTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(g.licenseNumber)));
        guidesTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(g.languages)));
        guidesTable->setItem(i, 3, new QTableWidgetItem(QString::number(g.experienceYears)));
        guidesTable->setItem(i, 4, new QTableWidgetItem(QString::number(g.rating, 'f', 2)));
        guidesTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(g.contact)));
        guidesTable->setItem(i, 6, new QTableWidgetItem(QString::number(g.dailyRate, 'f', 0)));
    }
    guidesTable->resizeColumnsToContents();
}

void MainWindow::populateReviewsTable(const std::vector<Review>& reviews)
{
    reviewsTable->setRowCount((int)reviews.size());
    for (int i = 0; i < (int)reviews.size(); i++) {
        const Review &r = reviews[i];
        reviewsTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(r.userName)));
        reviewsTable->setItem(i, 1, new QTableWidgetItem(QString::number(r.rating)));
        reviewsTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(r.comment)));
        reviewsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(r.reviewedAt)));
    }
    reviewsTable->resizeColumnsToContents();
}

// Only shows guides who aren't currently tied to another active
// booking - this is the actual fix for the double-booking problem.
void MainWindow::refreshAvailableGuideCombo()
{
    assignGuideCombo->clear();
    std::vector<Guide> available = mysqlManager.fetchAvailableGuides();

    if (available.empty()) {
        assignGuideCombo->addItem("No guides currently available", -1);
        assignGuideCombo->setEnabled(false);
        return;
    }

    assignGuideCombo->setEnabled(true);
    for (const Guide &g : available)
        assignGuideCombo->addItem(
            QString::fromStdString(g.name) + QString(" (%1 NPR/day)").arg(g.dailyRate, 0, 'f', 0),
            g.guideId);
}

void MainWindow::styleTable(QTableWidget *table)
{
    table->setStyleSheet(
        "QTableWidget::item { padding: 8px; }"
        "QHeaderView::section { padding: 8px; font-weight: bold; }"
        );
    table->verticalHeader()->setDefaultSectionSize(34);
    table->setAlternatingRowColors(true);
}

void MainWindow::onCreateBooking()
{
    int trekId = bookingTrekCombo->currentData().toInt();
    QString trekName = bookingTrekCombo->currentText();
    QString date = bookingDateEdit->date().toString("yyyy-MM-dd");
    int people = bookingPeopleSpin->value();

    // Guard against duplicate bookings: if the user already has a
    // Pending or Confirmed booking for this exact trek, warn them
    // before silently stacking up another one - there was previously
    // nothing stopping repeated clicks from creating identical
    // bookings over and over.
    std::vector<BookingDetail> existing = mysqlManager.fetchBookingDetailsForUser(currentUserId);
    for (const BookingDetail &b : existing) {
        bool sameTrek = QString::fromStdString(b.trekName) == trekName;
        bool stillActive = (b.status == "Pending" || b.status == "Confirmed");
        if (sameTrek && stillActive) {
            int choice = QMessageBox::question(
                this, "Already Booked",
                QString("You already have a %1 booking for \"%2\" (starting %3). "
                        "Book another one anyway?")
                    .arg(QString::fromStdString(b.status))
                    .arg(trekName)
                    .arg(QString::fromStdString(b.startDate)),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (choice != QMessageBox::Yes)
                return;
            break; // one warning is enough even if there are multiple matches
        }
    }

    // If the Budget Calculator has already worked out a real total for
    // this exact trek (entry fees + accommodation + food + transport,
    // not just the trek's base price), use that instead of silently
    // recalculating a different, smaller number behind the scenes.
    bool useCalculatedCost = currentTrip.hasCalculatedCost
                             && currentTrip.trek.trekId == trekId;

    // Same hard budget stop as "Book This Trip" in the Budget
    // Calculator - repeated here in case someone reaches this tab
    // directly (e.g. tab-clicking instead of "Book This Trip") rather
    // than going through onProceedToBooking().
    if (useCalculatedCost && currentTrip.budgetLimit > 0
        && currentTrip.totalCost > currentTrip.budgetLimit) {
        double over = currentTrip.totalCost - currentTrip.budgetLimit;
        QMessageBox::warning(this, "Over Budget",
                             QString("This trip costs %1 NPR, which is %2 NPR over your %3 NPR budget.\n\n"
                                     "Go back to the Budget Calculator, bring it under budget, and "
                                     "recalculate before booking.")
                                 .arg(currentTrip.totalCost, 0, 'f', 0)
                                 .arg(over, 0, 'f', 0)
                                 .arg(currentTrip.budgetLimit, 0, 'f', 0));
        return;
    }

    pair<int, double> result = useCalculatedCost
                                   ? mysqlManager.createBookingWithCost(currentUserId, trekId, date, people, currentTrip.totalCost)
                                   : mysqlManager.createBooking(currentUserId, trekId, date, people);

    if (result.first == -1) {
        bookingResultLabel->setStyleSheet("color: #e07a5f;");
        bookingResultLabel->setText("Booking failed - please try again in a moment.");
        QMessageBox::warning(this, "Booking Failed", "Could not create the booking. Is the database reachable?");
    } else {
        bookingResultLabel->setStyleSheet("color: #4fb8a2;");
        bookingResultLabel->setText(
            QString("Booked! Trip cost: %1 NPR").arg(result.second, 0, 'f', 2));
        populateBookingsTable(mysqlManager.fetchBookingDetailsForUser(currentUserId));

        // This trip is now an actual saved booking, not just a draft -
        // reflect that everywhere the trip status shows up.
        if (trekId == currentTrip.trek.trekId) {
            currentTrip.isBooked = true;
            updateTripBar();
        }
    }
}

void MainWindow::onRefreshBookings()
{
    populateBookingsTable(mysqlManager.fetchBookingDetailsForUser(currentUserId));
    refreshAvailableGuideCombo();
}

void MainWindow::onConfirmBooking()
{
    int row = bookingsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "No Selection", "Select a booking first.");
        return;
    }
    int bookingId = bookingsTable->item(row, 0)->text().toInt();
    bool ok = mysqlManager.confirmBooking(bookingId);
    if (ok) {
        populateBookingsTable(mysqlManager.fetchBookingDetailsForUser(currentUserId));
        QMessageBox::information(this, "Confirmed", QString("Booking #%1 confirmed.").arg(bookingId));
    } else {
        QMessageBox::warning(this, "Failed", "Could not confirm booking.");
    }
}

void MainWindow::onCancelBooking()
{
    int row = bookingsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "No Selection", "Select a booking first.");
        return;
    }
    int bookingId = bookingsTable->item(row, 0)->text().toInt();

    int choice = QMessageBox::question(this, "Cancel Booking",
                                       QString("Cancel booking #%1? This cannot be undone.").arg(bookingId),
                                       QMessageBox::Yes | QMessageBox::No);

    if (choice != QMessageBox::Yes)
        return;

    bool ok = mysqlManager.cancelBooking(bookingId);
    if (ok) {
        populateBookingsTable(mysqlManager.fetchBookingDetailsForUser(currentUserId));
        refreshAvailableGuideCombo(); // cancelling frees up any guide that was on this booking
        QMessageBox::information(this, "Cancelled", QString("Booking #%1 cancelled.").arg(bookingId));
    } else {
        QMessageBox::warning(this, "Failed", "Could not cancel booking.");
    }
}

void MainWindow::onAssignGuide()
{
    int row = bookingsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "No Selection", "Select a booking first.");
        return;
    }

    int guideId = assignGuideCombo->currentData().toInt();
    if (guideId < 0) {
        QMessageBox::information(this, "No Guides Available", "There are no free guides to assign right now.");
        return;
    }

    int bookingId = bookingsTable->item(row, 0)->text().toInt();
    QString bookingTrekName = bookingsTable->item(row, 2)->text();

    QString message;
    double feeAdded = 0.0;
    bool ok = mysqlManager.assignGuide(bookingId, guideId, message, feeAdded);

    if (ok) {
        populateBookingsTable(mysqlManager.fetchBookingDetailsForUser(currentUserId));
        refreshAvailableGuideCombo(); // that guide is no longer free

        // Roll the fee into the trip that's actively shown in the trip
        // bar / Profile summary too, but only if this booking is the
        // same trek that trip is tracking - otherwise it'd misattribute
        // one trek's guide fee onto a totally different trip.
        if (bookingTrekName == QString::fromStdString(currentTrip.trek.name)) {
            currentTrip.totalCost += feeAdded;
            currentTrip.guideFee += feeAdded;
            updateTripBar();
        }

        QMessageBox::information(this, "Guide Assigned",
                                 QString("%1\n\nGuide fee added: %2 NPR (their daily rate \u00d7 the trek's duration). "
                                         "The booking's total cost has been updated.")
                                     .arg(message)
                                     .arg(feeAdded, 0, 'f', 0));
    } else {
        // This is the guide-locking rule in action - e.g. the guide
        // just got taken by someone else's booking in the meantime.
        QMessageBox::warning(this, "Could Not Assign Guide", message);
        refreshAvailableGuideCombo();
    }
}

void MainWindow::onLoadReviews()
{
    int trekId = reviewsTrekCombo->currentData().toInt();
    populateReviewsTable(mysqlManager.fetchReviews(trekId));
}

void MainWindow::onSubmitReview()
{
    int trekId = reviewsTrekCombo->currentData().toInt();
    int rating = reviewRatingSpin->value();
    QString comment = reviewCommentEdit->text();

    QString errorMessage;
    bool ok = mysqlManager.addReview(currentUserId, trekId, rating, comment, errorMessage);

    if (ok) {
        QMessageBox::information(this, "Review Added", "Review submitted successfully.");
        populateReviewsTable(mysqlManager.fetchReviews(trekId));
        reviewCommentEdit->clear();
    } else {
        QMessageBox::warning(this, "Review Blocked", errorMessage);
    }
}