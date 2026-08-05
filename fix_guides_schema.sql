-- ============================================================
-- NEPAL TREK PLANNER — SCHEMA FIX (non-destructive)
-- Brings an existing/outdated database up to date with the
-- current app code, WITHOUT dropping or wiping any existing
-- Users, Bookings, Reviews, etc.
--
-- Fixes two issues:
--   1. Guides table missing the daily_rate column
--      -> caused fetchGuides()/fetchAvailableGuides() to fail
--         silently and show an empty guides table.
--   2. sp_assign_guide stored procedure was the old 4-parameter
--      version -> caused "Incorrect number of arguments... expected
--      4, got 5" when assigning a guide.
--
-- Safe to run multiple times.
-- ============================================================

USE nepal_trek_planner;

-- ------------------------------------------------------------
-- 1. Add daily_rate column to Guides (if missing)
-- ------------------------------------------------------------

SET @col_exists := (
    SELECT COUNT(*) FROM information_schema.COLUMNS
    WHERE TABLE_SCHEMA = 'nepal_trek_planner'
      AND TABLE_NAME = 'Guides'
      AND COLUMN_NAME = 'daily_rate'
);

SET @add_col_sql := IF(
    @col_exists = 0,
    'ALTER TABLE Guides ADD COLUMN daily_rate DECIMAL(10,2) NOT NULL DEFAULT 1500.00',
    'SELECT "daily_rate column already exists, skipping" AS info'
);

PREPARE stmt FROM @add_col_sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- Backfill the correct per-guide rates (harmless to re-run;
-- only touches rows matching each license_number).
UPDATE Guides SET daily_rate = 2000.00 WHERE license_number = 'NTB-1021';
UPDATE Guides SET daily_rate = 3000.00 WHERE license_number = 'NTB-2045';
UPDATE Guides SET daily_rate = 3200.00 WHERE license_number = 'NTB-3067';
UPDATE Guides SET daily_rate = 1500.00 WHERE license_number = 'NTB-4089';
UPDATE Guides SET daily_rate = 2200.00 WHERE license_number = 'NTB-5102';

-- ------------------------------------------------------------
-- 2. Recreate sp_assign_guide as the current 5-param version
-- ------------------------------------------------------------

DROP PROCEDURE IF EXISTS sp_assign_guide;

DELIMITER //

-- Assign a guide, but block it if that guide is already on another
-- active (Pending/Confirmed) booking (guide-locking). On success,
-- the guide's fee (daily_rate * trek's duration_days) is added onto
-- Bookings.total_cost, on top of whatever the Budget Calculator
-- already worked out.
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

DELIMITER ;

-- ------------------------------------------------------------
-- 3. Verification (just prints results, changes nothing)
-- ------------------------------------------------------------

SELECT 'Guides table columns:' AS check_1;
DESCRIBE Guides;

SELECT 'Guides data:' AS check_2;
SELECT guide_id, name, license_number, daily_rate FROM Guides;

SELECT 'sp_assign_guide parameter count (should be 5):' AS check_3;
SELECT COUNT(*) AS param_count
FROM information_schema.PARAMETERS
WHERE SPECIFIC_NAME = 'sp_assign_guide'
  AND SPECIFIC_SCHEMA = 'nepal_trek_planner';
