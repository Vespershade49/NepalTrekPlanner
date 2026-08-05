-- ============================================================
-- NEPAL TOURIST TREK PLANNER — DATABASE
-- Consolidated version: original schema + security-question
-- password recovery + guide-locking, no email dependency.
-- ============================================================

DROP DATABASE IF EXISTS nepal_trek_planner;
CREATE DATABASE nepal_trek_planner;
USE nepal_trek_planner;

-- ============================================================
-- 1. TABLES
-- ============================================================

CREATE TABLE Users (
    user_id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    nationality VARCHAR(50),
    contact VARCHAR(20),
    security_question VARCHAR(255),
    security_answer_hash VARCHAR(255),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE Treks (
    trek_id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    region VARCHAR(50),
    difficulty_level ENUM('Easy', 'Moderate', 'Hard', 'Extreme') NOT NULL DEFAULT 'Moderate',
    duration_days INT NOT NULL,
    base_cost DECIMAL(10,2) NOT NULL,
    max_altitude INT,
    description TEXT
);

CREATE TABLE Guides (
    guide_id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    license_number VARCHAR(50) UNIQUE,
    languages VARCHAR(100),
    experience_years INT,
    rating DECIMAL(3,2) CHECK (rating BETWEEN 0 AND 5),
    contact VARCHAR(20),
    daily_rate DECIMAL(10,2) NOT NULL DEFAULT 1500.00 -- NPR/day charged when this guide is assigned to a booking
);

CREATE TABLE Bookings (
    booking_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    trek_id INT NOT NULL,
    start_date DATE NOT NULL,
    num_people INT DEFAULT 1 CHECK (num_people > 0),
    total_cost DECIMAL(10,2),
    status ENUM('Pending', 'Confirmed', 'Cancelled') DEFAULT 'Pending',
    booked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES Users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (trek_id) REFERENCES Treks(trek_id) ON DELETE RESTRICT
);

CREATE TABLE Booking_Guides (
    id INT AUTO_INCREMENT PRIMARY KEY,
    booking_id INT NOT NULL,
    guide_id INT NOT NULL,
    FOREIGN KEY (booking_id) REFERENCES Bookings(booking_id) ON DELETE CASCADE,
    FOREIGN KEY (guide_id) REFERENCES Guides(guide_id) ON DELETE RESTRICT,
    UNIQUE KEY unique_booking_guide (booking_id, guide_id)
);

CREATE TABLE Reviews (
    review_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,
    trek_id INT,
    rating INT CHECK (rating BETWEEN 1 AND 5),
    comment TEXT,
    reviewed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES Users(user_id) ON DELETE SET NULL,
    FOREIGN KEY (trek_id) REFERENCES Treks(trek_id) ON DELETE CASCADE
);

CREATE TABLE Permits (
    permit_id INT AUTO_INCREMENT PRIMARY KEY,
    trek_id INT,
    permit_name VARCHAR(100),
    cost DECIMAL(10,2),
    issuing_authority VARCHAR(100),
    FOREIGN KEY (trek_id) REFERENCES Treks(trek_id) ON DELETE CASCADE
);

-- ============================================================
-- 2. SAMPLE DATA
-- ============================================================

INSERT INTO Treks (name, region, difficulty_level, duration_days, base_cost, max_altitude, description) VALUES
('Everest Base Camp', 'Khumbu', 'Hard', 14, 1200.00, 5364, 'Classic trek to the base of Mount Everest.'),
('Annapurna Circuit', 'Annapurna', 'Moderate', 18, 1000.00, 5416, 'A scenic circuit around the Annapurna massif.'),
('Poon Hill', 'Annapurna', 'Easy', 4, 300.00, 3210, 'Short trek famous for sunrise views.'),
('Langtang Valley', 'Langtang', 'Moderate', 7, 550.00, 3870, 'Valley trek close to Kathmandu with Tibetan-influenced culture.'),
('Upper Mustang', 'Mustang', 'Hard', 12, 1800.00, 3840, 'Restricted-area trek through the former Kingdom of Lo.'),
('Manaslu Circuit', 'Gorkha', 'Extreme', 16, 1600.00, 5106, 'Remote circuit around the eighth highest mountain in the world.');

INSERT INTO Guides (name, license_number, languages, experience_years, rating, contact, daily_rate) VALUES
('Ram Tamang', 'NTB-1021', 'Nepali, English', 8, 4.80, '9801111111', 2000.00),
('Sita Sherpa', 'NTB-2045', 'Nepali, English, Japanese', 12, 4.95, '9802222222', 3000.00),
('Dawa Sherpa', 'NTB-3067', 'Nepali, English, Mandarin', 15, 4.90, '9803333333', 3200.00),
('Kamal Rai', 'NTB-4089', 'Nepali, English, Hindi', 5, 4.50, '9804444444', 1500.00),
('Nima Gurung', 'NTB-5102', 'Nepali, English, German', 10, 4.70, '9805555555', 2200.00);

INSERT INTO Permits (trek_id, permit_name, cost, issuing_authority) VALUES
(1, 'Sagarmatha National Park Entry', 30.00, 'Dept of National Parks'),
(1, 'TIMS Card', 20.00, 'Nepal Tourism Board'),
(2, 'Annapurna Conservation Permit', 30.00, 'ACAP'),
(2, 'TIMS Card', 20.00, 'Nepal Tourism Board'),
(3, 'Annapurna Conservation Permit', 30.00, 'ACAP'),
(4, 'Langtang National Park Entry', 30.00, 'Dept of National Parks'),
(5, 'Upper Mustang Restricted Area Permit', 500.00, 'Dept of Immigration'),
(5, 'Annapurna Conservation Permit', 30.00, 'ACAP'),
(6, 'Manaslu Restricted Area Permit', 100.00, 'Dept of Immigration'),
(6, 'Manaslu Conservation Area Permit', 30.00, 'MCAP');

-- No sample Users/Bookings/Reviews on purpose - sign up fresh in the
-- app so your account's password is hashed by the current code.

-- ============================================================
-- 3. VIEWS
-- ============================================================

CREATE OR REPLACE VIEW View_Booking_Details AS
SELECT
    b.booking_id,
    u.name AS user_name,
    u.email,
    t.name AS trek_name,
    t.region,
    t.difficulty_level,
    b.start_date,
    b.num_people,
    b.total_cost,
    b.status,
    GROUP_CONCAT(g.name SEPARATOR ', ') AS assigned_guides
FROM Bookings b
JOIN Users u ON b.user_id = u.user_id
JOIN Treks t ON b.trek_id = t.trek_id
LEFT JOIN Booking_Guides bg ON b.booking_id = bg.booking_id
LEFT JOIN Guides g ON bg.guide_id = g.guide_id
GROUP BY b.booking_id;

CREATE OR REPLACE VIEW View_Trek_Summary AS
SELECT
    t.trek_id,
    t.name,
    t.region,
    t.difficulty_level,
    t.duration_days,
    t.base_cost,
    COALESCE(ROUND(AVG(r.rating), 2), 0) AS avg_rating,
    COUNT(DISTINCT r.review_id) AS num_reviews,
    COALESCE((SELECT SUM(p.cost) FROM Permits p WHERE p.trek_id = t.trek_id), 0) AS total_permit_cost
FROM Treks t
LEFT JOIN Reviews r ON t.trek_id = r.trek_id
GROUP BY t.trek_id;

CREATE OR REPLACE VIEW View_Guide_Workload AS
SELECT
    g.guide_id,
    g.name,
    g.rating,
    COUNT(bg.booking_id) AS total_assignments
FROM Guides g
LEFT JOIN Booking_Guides bg ON g.guide_id = bg.guide_id
GROUP BY g.guide_id;

-- Guides who are currently free (not on any Pending/Confirmed booking)
CREATE OR REPLACE VIEW View_Available_Guides AS
SELECT g.*
FROM Guides g
WHERE g.guide_id NOT IN (
    SELECT bg.guide_id
    FROM Booking_Guides bg
    JOIN Bookings b ON bg.booking_id = b.booking_id
    WHERE b.status IN ('Pending', 'Confirmed')
);

-- ============================================================
-- 4. FUNCTIONS
-- ============================================================

DELIMITER //

CREATE FUNCTION fn_calculate_total_cost(p_trek_id INT, p_num_people INT)
RETURNS DECIMAL(10,2)
DETERMINISTIC
BEGIN
    DECLARE v_base_cost DECIMAL(10,2);
    SELECT base_cost INTO v_base_cost FROM Treks WHERE trek_id = p_trek_id;
    RETURN v_base_cost * p_num_people;
END //

CREATE FUNCTION fn_total_permit_cost(p_trek_id INT)
RETURNS DECIMAL(10,2)
DETERMINISTIC
BEGIN
    DECLARE v_total DECIMAL(10,2);
    SELECT COALESCE(SUM(cost), 0) INTO v_total FROM Permits WHERE trek_id = p_trek_id;
    RETURN v_total;
END //

DELIMITER ;

-- ============================================================
-- 5. STORED PROCEDURES
-- ============================================================

DELIMITER //

CREATE PROCEDURE sp_create_booking(
    IN p_user_id INT,
    IN p_trek_id INT,
    IN p_start_date DATE,
    IN p_num_people INT
)
BEGIN
    DECLARE v_total_cost DECIMAL(10,2);
    SET v_total_cost = fn_calculate_total_cost(p_trek_id, p_num_people);

    INSERT INTO Bookings (user_id, trek_id, start_date, num_people, total_cost, status)
    VALUES (p_user_id, p_trek_id, p_start_date, p_num_people, v_total_cost, 'Pending');

    SELECT LAST_INSERT_ID() AS new_booking_id, v_total_cost AS calculated_cost;
END //

-- Create a booking with an explicit total cost (used when the Budget
-- Calculator has already worked out the real total, including entry
-- fees, accommodation, food, and transport - not just the trek's
-- base price).
CREATE PROCEDURE sp_create_booking_with_cost(
    IN p_user_id INT,
    IN p_trek_id INT,
    IN p_start_date DATE,
    IN p_num_people INT,
    IN p_total_cost DECIMAL(10,2)
)
BEGIN
    INSERT INTO Bookings (user_id, trek_id, start_date, num_people, total_cost, status)
    VALUES (p_user_id, p_trek_id, p_start_date, p_num_people, p_total_cost, 'Pending');

    SELECT LAST_INSERT_ID() AS new_booking_id, p_total_cost AS calculated_cost;
END //

CREATE PROCEDURE sp_confirm_booking(IN p_booking_id INT)
BEGIN
    UPDATE Bookings
    SET status = 'Confirmed'
    WHERE booking_id = p_booking_id AND status = 'Pending';
END //

CREATE PROCEDURE sp_cancel_booking(IN p_booking_id INT)
BEGIN
    UPDATE Bookings
    SET status = 'Cancelled'
    WHERE booking_id = p_booking_id AND status != 'Cancelled';
END //

-- Assign a guide, but block it if that guide is already on another
-- active (Pending/Confirmed) booking. This is the guide-locking fix -
-- a guide can't be double-booked onto two active trips at once.
--
-- On a successful assignment, the guide's fee (daily_rate * the
-- trek's duration_days) is added straight onto Bookings.total_cost -
-- a guide wasn't free before, and now their cost actually shows up in
-- what the trekker owes, on top of whatever the Budget Calculator
-- already worked out for trek/accommodation/food/transport.
CREATE PROCEDURE sp_assign_guide(
    IN p_booking_id INT,
    IN p_guide_id INT,
    OUT p_success BOOLEAN,
    OUT p_message VARCHAR(255),
    OUT p_fee_added DECIMAL(10,2)
)
BEGIN
    DECLARE conflict_count INT DEFAULT 0;
    DECLARE v_daily_rate DECIMAL(10,2);
    DECLARE v_duration_days INT;

    SET p_fee_added = 0;

    SELECT COUNT(*) INTO conflict_count
    FROM Booking_Guides bg
    JOIN Bookings b ON bg.booking_id = b.booking_id
    WHERE bg.guide_id = p_guide_id
      AND b.status IN ('Pending', 'Confirmed')
      AND bg.booking_id != p_booking_id;

    IF conflict_count > 0 THEN
        SET p_success = FALSE;
        SET p_message = 'This guide is already assigned to another active trip.';

    ELSEIF EXISTS (
        SELECT 1 FROM Booking_Guides
        WHERE booking_id = p_booking_id AND guide_id = p_guide_id
    ) THEN
        SET p_success = FALSE;
        SET p_message = 'This guide is already assigned to this booking.';

    ELSE
        INSERT INTO Booking_Guides (booking_id, guide_id) VALUES (p_booking_id, p_guide_id);

        SELECT g.daily_rate, t.duration_days
        INTO v_daily_rate, v_duration_days
        FROM Bookings b
        JOIN Treks t ON b.trek_id = t.trek_id
        JOIN Guides g ON g.guide_id = p_guide_id
        WHERE b.booking_id = p_booking_id;

        SET p_fee_added = v_daily_rate * v_duration_days;

        UPDATE Bookings
        SET total_cost = total_cost + p_fee_added
        WHERE booking_id = p_booking_id;

        SET p_success = TRUE;
        SET p_message = 'Guide assigned successfully.';
    END IF;
END //

CREATE PROCEDURE sp_get_user_bookings(IN p_user_id INT)
BEGIN
    SELECT * FROM View_Booking_Details WHERE user_name = (
        SELECT name FROM Users WHERE user_id = p_user_id
    );
END //

DELIMITER ;

-- ============================================================
-- 6. TRIGGERS
-- ============================================================

DELIMITER //

CREATE TRIGGER trg_bookings_before_insert
BEFORE INSERT ON Bookings
FOR EACH ROW
BEGIN
    IF NEW.total_cost IS NULL THEN
        SET NEW.total_cost = fn_calculate_total_cost(NEW.trek_id, NEW.num_people);
    END IF;
END //

CREATE TRIGGER trg_reviews_before_insert
BEFORE INSERT ON Reviews
FOR EACH ROW
BEGIN
    IF NEW.user_id IS NOT NULL AND NOT EXISTS (
        SELECT 1 FROM Bookings
        WHERE user_id = NEW.user_id AND trek_id = NEW.trek_id
    ) THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'User can only review treks they have booked.';
    END IF;
END //

DELIMITER ;

-- ============================================================
-- 7. NON-DESTRUCTIVE MIGRATION (guide daily_rate + fee-on-assign)
-- ============================================================
-- Running the whole script above drops and recreates the database,
-- which wipes any real users/bookings you already have. If you don't
-- want that, run ONLY the block below against your existing database
-- instead of the full script - it adds the new column and replaces
-- sp_assign_guide with the fee-charging version, without touching any
-- existing rows.
--
-- ALTER TABLE Guides ADD COLUMN daily_rate DECIMAL(10,2) NOT NULL DEFAULT 1500.00;
-- UPDATE Guides SET daily_rate = 2000.00 WHERE license_number = 'NTB-1021';
-- UPDATE Guides SET daily_rate = 3000.00 WHERE license_number = 'NTB-2045';
-- UPDATE Guides SET daily_rate = 3200.00 WHERE license_number = 'NTB-3067';
-- UPDATE Guides SET daily_rate = 1500.00 WHERE license_number = 'NTB-4089';
-- UPDATE Guides SET daily_rate = 2200.00 WHERE license_number = 'NTB-5102';
--
-- DROP PROCEDURE IF EXISTS sp_assign_guide;
-- DELIMITER //
-- (then paste the sp_assign_guide CREATE PROCEDURE block from section 5 above)
-- DELIMITER ;
