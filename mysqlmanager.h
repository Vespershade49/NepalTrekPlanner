#ifndef MYSQLMANAGER_H
#define MYSQLMANAGER_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>
#include <QCryptographicHash>
#include <QDateTime>
#include <vector>
#include <utility>
#include "trekroute.h"
#include "guide.h"
#include "permit.h"
#include "bookingdetail.h"
#include "review.h"

using namespace std;

// Simple pair of (user_id, name) for populating "book as" dropdowns
struct UserRow {
    int userId;
    string name;
    UserRow() {}
    UserRow(int id, string n) : userId(id), name(n) {}
};

class MySqlManager {
private:
    QSqlDatabase db;

    string mapDifficulty(string mysqlDifficulty) {
        if (mysqlDifficulty == "Easy")     return "Beginner";
        if (mysqlDifficulty == "Moderate") return "Intermediate";
        if (mysqlDifficulty == "Hard")     return "Expert";
        if (mysqlDifficulty == "Extreme")  return "Expert";
        return "Intermediate";
    }

    // Passwords are hashed with SHA-256 - we never store or compare
    // the raw text. Case matters, so no trimming/lowercasing here.
    QString hashText(QString text) {
        QByteArray hash = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256);
        return QString::fromUtf8(hash.toHex());
    }

    // Security answers are forgiving about case/whitespace ("Kathmandu"
    // and "kathmandu " both work) since people don't always remember
    // exactly how they typed something months later.
    QString hashAnswer(QString text) {
        QByteArray hash = QCryptographicHash::hash(
            text.trimmed().toLower().toUtf8(), QCryptographicHash::Sha256);
        return QString::fromUtf8(hash.toHex());
    }

public:
    MySqlManager() {
        if (QSqlDatabase::contains("qt_sql_default_connection")) {
            db = QSqlDatabase::database("qt_sql_default_connection");
        } else {
            db = QSqlDatabase::addDatabase("QMYSQL");
            db.setHostName("127.0.0.1");
            db.setUserName("root");
            db.setPassword("");
            db.setDatabaseName("nepal_trek_planner");
        }
    }

    bool connect() {
        if (!db.isOpen()) {
            if (!db.open()) {
                qDebug() << "MySQL Connection Error:" << db.lastError().text();
                return false;
            }
        }
        return true;
    }

    // TREKS
    std::vector<TrekRoute> fetchTrekRoutes() {
        std::vector<TrekRoute> routes;
        if (!connect()) return routes;

        QSqlQuery query(
            "SELECT trek_id, name, duration_days, difficulty_level, max_altitude, base_cost FROM Treks"
            );

        while (query.next()) {
            int id            = query.value(0).toInt();
            string name       = query.value(1).toString().toStdString();
            int days          = query.value(2).toInt();
            string diff       = query.value(3).toString().toStdString();
            double altitude   = query.value(4).toDouble();
            double cost       = query.value(5).toDouble();

            vector<string> checkpoints = {"Start Node", "End Node"};
            TrekRoute r(id, name, days, mapDifficulty(diff), altitude, cost,
                        "Autumn/Spring", true, checkpoints);
            routes.push_back(r);
        }
        return routes;
    }

    // USERS
    std::vector<UserRow> fetchUsers() {
        std::vector<UserRow> users;
        if (!connect()) return users;

        QSqlQuery query("SELECT user_id, name FROM Users");
        while (query.next())
            users.push_back(UserRow(query.value(0).toInt(), query.value(1).toString().toStdString()));
        return users;
    }

    // GUIDES - everyone (used for reference display)
    std::vector<Guide> fetchGuides() {
        std::vector<Guide> guides;
        if (!connect()) return guides;

        QSqlQuery query(
            "SELECT guide_id, name, license_number, languages, experience_years, rating, contact, daily_rate FROM Guides"
            );
        while (query.next()) {
            guides.push_back(Guide(
                query.value(0).toInt(),
                query.value(1).toString().toStdString(),
                query.value(2).toString().toStdString(),
                query.value(3).toString().toStdString(),
                query.value(4).toInt(),
                query.value(5).toDouble(),
                query.value(6).toString().toStdString(),
                query.value(7).toDouble()
                ));
        }
        return guides;
    }

    // GUIDES - only those free of any active (Pending/Confirmed) booking
    // right now. This is what powers the "Assign Guide" dropdown so we
    // can't double-book a guide.
    std::vector<Guide> fetchAvailableGuides() {
        std::vector<Guide> guides;
        if (!connect()) return guides;

        QSqlQuery query(
            "SELECT guide_id, name, license_number, languages, experience_years, rating, contact, daily_rate "
            "FROM Guides g WHERE g.guide_id NOT IN ("
            "  SELECT bg.guide_id FROM Booking_Guides bg "
            "  JOIN Bookings b ON bg.booking_id = b.booking_id "
            "  WHERE b.status IN ('Pending','Confirmed'))"
            );
        while (query.next()) {
            guides.push_back(Guide(
                query.value(0).toInt(),
                query.value(1).toString().toStdString(),
                query.value(2).toString().toStdString(),
                query.value(3).toString().toStdString(),
                query.value(4).toInt(),
                query.value(5).toDouble(),
                query.value(6).toString().toStdString(),
                query.value(7).toDouble()
                ));
        }
        return guides;
    }

    // PERMITS
    std::vector<Permit> fetchPermits(int trekId) {
        std::vector<Permit> permits;
        if (!connect()) return permits;

        QSqlQuery query;
        query.prepare("SELECT permit_id, trek_id, permit_name, cost, issuing_authority FROM Permits WHERE trek_id = :tid");
        query.bindValue(":tid", trekId);
        query.exec();

        while (query.next()) {
            permits.push_back(Permit(
                query.value(0).toInt(),
                query.value(1).toInt(),
                query.value(2).toString().toStdString(),
                query.value(3).toDouble(),
                query.value(4).toString().toStdString()
                ));
        }
        return permits;
    }

    double fetchTotalPermitCost(int trekId) {
        if (!connect()) return 0;
        QSqlQuery query;
        query.prepare("SELECT fn_total_permit_cost(:tid)");
        query.bindValue(":tid", trekId);
        query.exec();
        if (query.next())
            return query.value(0).toDouble();
        return 0;
    }

    // BOOKINGS
    std::vector<BookingDetail> fetchBookingDetails() {
        std::vector<BookingDetail> bookings;
        if (!connect()) return bookings;

        QSqlQuery query("SELECT booking_id, user_name, trek_name, region, difficulty_level, "
                        "start_date, num_people, total_cost, status, assigned_guides FROM View_Booking_Details");
        while (query.next()) {
            bookings.push_back(BookingDetail(
                query.value(0).toInt(),
                query.value(1).toString().toStdString(),
                query.value(2).toString().toStdString(),
                query.value(3).toString().toStdString(),
                query.value(4).toString().toStdString(),
                query.value(5).toString().toStdString(),
                query.value(6).toInt(),
                query.value(7).toDouble(),
                query.value(8).toString().toStdString(),
                query.value(9).toString().toStdString()
                ));
        }
        return bookings;
    }

    // Only the bookings belonging to one user - powers "My Bookings"
    // so a user isn't staring at everyone else's trips.
    std::vector<BookingDetail> fetchBookingDetailsForUser(int userId) {
        std::vector<BookingDetail> bookings;
        if (!connect()) return bookings;

        QSqlQuery query;
        query.prepare(
            "SELECT v.booking_id, v.user_name, v.trek_name, v.region, v.difficulty_level, "
            "v.start_date, v.num_people, v.total_cost, v.status, v.assigned_guides "
            "FROM View_Booking_Details v "
            "JOIN Bookings b ON v.booking_id = b.booking_id "
            "WHERE b.user_id = :uid"
            );
        query.bindValue(":uid", userId);
        query.exec();

        while (query.next()) {
            bookings.push_back(BookingDetail(
                query.value(0).toInt(),
                query.value(1).toString().toStdString(),
                query.value(2).toString().toStdString(),
                query.value(3).toString().toStdString(),
                query.value(4).toString().toStdString(),
                query.value(5).toString().toStdString(),
                query.value(6).toInt(),
                query.value(7).toDouble(),
                query.value(8).toString().toStdString(),
                query.value(9).toString().toStdString()
                ));
        }
        return bookings;
    }

    pair<int, double> createBooking(int userId, int trekId, QString startDate, int numPeople) {
        if (!connect()) {
            qDebug() << "createBooking: not connected to database";
            return make_pair(-1, 0.0);
        }

        QSqlQuery query;
        query.prepare("CALL sp_create_booking(?, ?, ?, ?)");
        query.addBindValue(userId);
        query.addBindValue(trekId);
        query.addBindValue(startDate);
        query.addBindValue(numPeople);

        if (!query.exec()) {
            qDebug() << "createBooking: exec() failed -" << query.lastError().text();
            return make_pair(-1, 0.0);
        }

        int newId = -1;
        double cost = 0.0;
        bool gotRow = false;

        while (true) {
            if (query.isActive() && query.isSelect()) {
                if (query.next()) {
                    QVariant vId = query.value(0);
                    QVariant vCost = query.value(1);
                    if (!vId.isNull()) newId = vId.toInt();
                    if (!vCost.isNull()) cost = vCost.toDouble();
                    gotRow = true;
                }
                while (query.next()) {}
                break;
            }
            if (!query.nextResult())
                break;
        }

        while (query.nextResult()) {}
        query.finish();

        if (gotRow) {
            qDebug() << "createBooking: success - id=" << newId << "cost=" << cost;
            return make_pair(newId, cost);
        }

        QString tempConnName;
        static int tempConnCounter = 0;
        tempConnName = QString("temp_mysql_conn_%1").arg(++tempConnCounter);

        {
            QSqlDatabase tempDb = QSqlDatabase::addDatabase("QMYSQL", tempConnName);
            tempDb.setHostName(db.hostName());
            tempDb.setUserName(db.userName());
            tempDb.setPassword(db.password());
            tempDb.setDatabaseName(db.databaseName());

            if (!tempDb.open()) {
                qDebug() << "createBooking: fallback temp connection failed -" << tempDb.lastError().text();
                QSqlDatabase::removeDatabase(tempConnName);
                return make_pair(-1, 0.0);
            }

            {
                QSqlQuery idQuery(tempDb);
                if (!idQuery.exec("SELECT LAST_INSERT_ID()")) {
                    qDebug() << "createBooking: LAST_INSERT_ID query failed -" << idQuery.lastError().text();
                } else if (idQuery.next()) {
                    newId = idQuery.value(0).toInt();
                }
            }

            {
                QSqlQuery costQuery(tempDb);
                costQuery.prepare("SELECT fn_calculate_total_cost(?, ?)");
                costQuery.addBindValue(trekId);
                costQuery.addBindValue(numPeople);
                if (!costQuery.exec()) {
                    qDebug() << "createBooking: cost query failed -" << costQuery.lastError().text();
                } else if (costQuery.next()) {
                    cost = costQuery.value(0).toDouble();
                }
            }

            tempDb.close();
        }
        QSqlDatabase::removeDatabase(tempConnName);

        qDebug() << "createBooking: fallback - id=" << newId << "cost=" << cost;
        return make_pair(newId, cost);
    }

    // Same as createBooking, but takes an explicit total cost instead
    // of recalculating trek-base-price * people. This is what lets a
    // booking reflect what the Budget Calculator actually worked out
    // (entry fees, accommodation, food, transport included).
    pair<int, double> createBookingWithCost(int userId, int trekId, QString startDate,
                                            int numPeople, double totalCost) {
        if (!connect()) return make_pair(-1, 0.0);

        QSqlQuery query;
        query.prepare("CALL sp_create_booking_with_cost(?, ?, ?, ?, ?)");
        query.addBindValue(userId);
        query.addBindValue(trekId);
        query.addBindValue(startDate);
        query.addBindValue(numPeople);
        query.addBindValue(totalCost);

        if (!query.exec()) {
            qDebug() << "createBookingWithCost: exec() failed -" << query.lastError().text();
            return make_pair(-1, 0.0);
        }

        int newId = -1;
        double cost = totalCost;
        bool gotRow = false;

        while (true) {
            if (query.isActive() && query.isSelect()) {
                if (query.next()) {
                    QVariant vId = query.value(0);
                    if (!vId.isNull()) newId = vId.toInt();
                    gotRow = true;
                }
                while (query.next()) {}
                break;
            }
            if (!query.nextResult())
                break;
        }
        while (query.nextResult()) {}
        query.finish();

        if (gotRow) return make_pair(newId, cost);

        QString tempConnName = QString("temp_mysql_conn_wc_%1").arg(QDateTime::currentMSecsSinceEpoch());
        {
            QSqlDatabase tempDb = QSqlDatabase::addDatabase("QMYSQL", tempConnName);
            tempDb.setHostName(db.hostName());
            tempDb.setUserName(db.userName());
            tempDb.setPassword(db.password());
            tempDb.setDatabaseName(db.databaseName());

            if (tempDb.open()) {
                QSqlQuery idQuery(tempDb);
                if (idQuery.exec("SELECT LAST_INSERT_ID()") && idQuery.next())
                    newId = idQuery.value(0).toInt();
                tempDb.close();
            }
        }
        QSqlDatabase::removeDatabase(tempConnName);

        return make_pair(newId, cost);
    }

    bool confirmBooking(int bookingId) {
        if (!connect()) return false;
        QSqlQuery query;
        query.prepare("CALL sp_confirm_booking(:bid)");
        query.bindValue(":bid", bookingId);
        return query.exec();
    }

    bool cancelBooking(int bookingId) {
        if (!connect()) return false;
        QSqlQuery query;
        query.prepare("CALL sp_cancel_booking(:bid)");
        query.bindValue(":bid", bookingId);
        return query.exec();
    }

    // Assign a guide, respecting the double-booking lock in
    // sp_assign_guide. Fills `message` with a user-facing explanation
    // either way, and `feeAdded` with the NPR amount (daily_rate *
    // trek duration) that was added onto the booking's total_cost -
    // 0 if the assignment failed.
    bool assignGuide(int bookingId, int guideId, QString &message, double &feeAdded) {
        feeAdded = 0.0;
        if (!connect()) {
            message = "Not connected to the database.";
            return false;
        }

        QSqlQuery query;
        query.prepare("CALL sp_assign_guide(?, ?, @success, @message, @fee_added)");
        query.addBindValue(bookingId);
        query.addBindValue(guideId);

        if (!query.exec()) {
            message = query.lastError().text();
            return false;
        }
        while (query.nextResult()) {}

        QSqlQuery result;
        result.exec("SELECT @success, @message, @fee_added");
        if (result.next()) {
            bool success = result.value(0).toBool();
            message = result.value(1).toString();
            feeAdded = result.value(2).toDouble();
            return success;
        }

        message = "Could not confirm whether the guide was assigned.";
        return false;
    }

    // REVIEWS
    std::vector<Review> fetchReviews(int trekId) {
        std::vector<Review> reviews;
        if (!connect()) return reviews;

        QSqlQuery query;
        query.prepare(
            "SELECT r.review_id, u.name, t.name, r.rating, r.comment, r.reviewed_at "
            "FROM Reviews r "
            "JOIN Users u ON r.user_id = u.user_id "
            "JOIN Treks t ON r.trek_id = t.trek_id "
            "WHERE r.trek_id = :tid"
            );
        query.bindValue(":tid", trekId);
        query.exec();

        while (query.next()) {
            reviews.push_back(Review(
                query.value(0).toInt(),
                query.value(1).toString().toStdString(),
                query.value(2).toString().toStdString(),
                query.value(3).toInt(),
                query.value(4).toString().toStdString(),
                query.value(5).toString().toStdString()
                ));
        }
        return reviews;
    }

    bool addReview(int userId, int trekId, int rating, QString comment, QString &errorMessage) {
        if (!connect()) {
            errorMessage = "Not connected to database.";
            return false;
        }

        QSqlQuery query;
        query.prepare("INSERT INTO Reviews (user_id, trek_id, rating, comment) VALUES (:uid, :tid, :rating, :comment)");
        query.bindValue(":uid", userId);
        query.bindValue(":tid", trekId);
        query.bindValue(":rating", rating);
        query.bindValue(":comment", comment);

        if (!query.exec()) {
            QString dbError = query.lastError().text();
            if (dbError.contains("only review treks they have booked", Qt::CaseInsensitive))
                errorMessage = "You can only review a trek after you've booked it.";
            else
                errorMessage = dbError;
            return false;
        }
        return true;
    }

    // ── LOGIN / SIGN UP ─────────────────────────────────────────

    // Accepts either an email or a username in `identifier`.
    UserRow authenticateUser(QString identifier, QString password) {
        if (!connect()) return UserRow(-1, "");

        QSqlQuery query;
        query.prepare(
            "SELECT user_id, name FROM Users "
            "WHERE (email = :id OR name = :id) AND password_hash = :password"
            );
        query.bindValue(":id", identifier);
        query.bindValue(":password", hashText(password));
        query.exec();

        if (query.next())
            return UserRow(query.value(0).toInt(), query.value(1).toString().toStdString());

        return UserRow(-1, "");
    }

    // Creates the account, including the security question set right
    // at signup - the simplest possible flow, no extra step needed.
    int registerUser(QString name, QString email, QString password,
                     QString securityQuestion, QString securityAnswer,
                     QString &errorMessage) {
        if (!connect()) {
            errorMessage = "Not connected to database.";
            return -1;
        }

        QSqlQuery query;
        query.prepare(
            "INSERT INTO Users (name, email, password_hash, security_question, security_answer_hash) "
            "VALUES (:name, :email, :password, :question, :answerHash)"
            );
        query.bindValue(":name", name);
        query.bindValue(":email", email);
        query.bindValue(":password", hashText(password));
        query.bindValue(":question", securityQuestion);
        query.bindValue(":answerHash", hashAnswer(securityAnswer));

        if (!query.exec()) {
            errorMessage = query.lastError().text();
            return -1;
        }
        return query.lastInsertId().toInt();
    }

    // ── PROFILE EDITING ─────────────────────────────────────────

    struct UserProfileData {
        QString name, email, nationality, contact;
    };

    UserProfileData fetchUserProfile(int userId) {
        UserProfileData data;
        if (!connect()) return data;
        QSqlQuery query;
        query.prepare("SELECT name, email, nationality, contact FROM Users WHERE user_id = :uid");
        query.bindValue(":uid", userId);
        query.exec();
        if (query.next()) {
            data.name = query.value(0).toString();
            data.email = query.value(1).toString();
            data.nationality = query.value(2).toString();
            data.contact = query.value(3).toString();
        }
        return data;
    }

    // Updates name/nationality/contact, and optionally the password
    // (only if both currentPassword and newPassword are given, and
    // currentPassword actually matches what's on file).
    bool updateUserProfile(int userId, QString name, QString nationality, QString contact,
                           QString currentPassword, QString newPassword, QString &errorMessage) {
        if (!connect()) {
            errorMessage = "Not connected to database.";
            return false;
        }

        if (!newPassword.isEmpty()) {
            QSqlQuery check;
            check.prepare("SELECT password_hash FROM Users WHERE user_id = :uid");
            check.bindValue(":uid", userId);
            check.exec();
            if (!check.next() || check.value(0).toString() != hashText(currentPassword)) {
                errorMessage = "Current password is incorrect.";
                return false;
            }
        }

        QSqlQuery query;
        if (!newPassword.isEmpty()) {
            query.prepare(
                "UPDATE Users SET name = :name, nationality = :nat, contact = :contact, "
                "password_hash = :password WHERE user_id = :uid"
                );
            query.bindValue(":password", hashText(newPassword));
        } else {
            query.prepare(
                "UPDATE Users SET name = :name, nationality = :nat, contact = :contact "
                "WHERE user_id = :uid"
                );
        }
        query.bindValue(":name", name);
        query.bindValue(":nat", nationality);
        query.bindValue(":contact", contact);
        query.bindValue(":uid", userId);

        if (!query.exec()) {
            errorMessage = query.lastError().text();
            return false;
        }
        return true;
    }

    // ── FORGOT PASSWORD (security question, no email needed) ───

    QString getSecurityQuestion(QString email) {
        if (!connect()) return "";
        QSqlQuery query;
        query.prepare("SELECT security_question FROM Users WHERE email = :email");
        query.bindValue(":email", email);
        query.exec();
        if (query.next())
            return query.value(0).toString();
        return "";
    }

    bool resetPasswordWithSecurityAnswer(QString email, QString answer,
                                         QString newPassword, QString &errorMessage) {
        if (!connect()) {
            errorMessage = "Not connected to database.";
            return false;
        }

        QSqlQuery check;
        check.prepare("SELECT security_answer_hash FROM Users WHERE email = :email");
        check.bindValue(":email", email);
        check.exec();

        if (!check.next()) {
            errorMessage = "No account found with that email.";
            return false;
        }

        QString storedHash = check.value(0).toString();
        if (storedHash.isEmpty() || storedHash != hashAnswer(answer)) {
            errorMessage = "That answer doesn't match what we have on file.";
            return false;
        }

        QSqlQuery update;
        update.prepare("UPDATE Users SET password_hash = :password WHERE email = :email");
        update.bindValue(":password", hashText(newPassword));
        update.bindValue(":email", email);

        if (!update.exec()) {
            errorMessage = update.lastError().text();
            return false;
        }
        return true;
    }
};

#endif // MYSQLMANAGER_H