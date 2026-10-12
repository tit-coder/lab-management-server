-- =============================================================================
-- Lab Management Server - Database Initialization Schema
-- Database: lab_management
-- Supported RDBMS: MariaDB 10.4+ / MySQL 8.0+
-- =============================================================================

-- 1. Create Database safely
CREATE DATABASE IF NOT EXISTS `lab_management`
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE `lab_management`;

-- =============================================================================
-- 2. Create Application User (Security Best Practice: DO NOT use root in app)
-- Note: Run the following commands as MariaDB administrator (root):
--
-- CREATE USER IF NOT EXISTS 'lab_user'@'localhost' IDENTIFIED BY 'lab_password_change_me';
-- CREATE USER IF NOT EXISTS 'lab_user'@'127.0.0.1' IDENTIFIED BY 'lab_password_change_me';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON lab_management.* TO 'lab_user'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON lab_management.* TO 'lab_user'@'127.0.0.1';
-- FLUSH PRIVILEGES;
-- =============================================================================

-- Disable foreign key checks during schema creation
SET FOREIGN_KEY_CHECKS = 0;

-- 3. Table: devices
-- Stores lab computer equipment and network peripherals
CREATE TABLE IF NOT EXISTS `devices` (
    `id` INT AUTO_INCREMENT PRIMARY KEY,
    `device_code` VARCHAR(50) NOT NULL UNIQUE COMMENT 'Unique identifier code, e.g., PC-LAB101-01',
    `device_name` VARCHAR(100) NOT NULL COMMENT 'Display name of equipment',
    `device_type` VARCHAR(50) NOT NULL COMMENT 'Type: PC, Monitor, Keyboard, Mouse, Network',
    `room` VARCHAR(50) NOT NULL COMMENT 'Lab room location, e.g., LAB101, LAB102',
    `status` ENUM('Active', 'Broken', 'Repairing') NOT NULL DEFAULT 'Active' COMMENT 'Current operating state',
    `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Registration timestamp',
    INDEX `idx_devices_room` (`room`),
    INDEX `idx_devices_status` (`status`),
    INDEX `idx_devices_type` (`device_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 4. Table: maintenance_logs
-- Records equipment issues, repair progress, and maintenance history
CREATE TABLE IF NOT EXISTS `maintenance_logs` (
    `id` INT AUTO_INCREMENT PRIMARY KEY,
    `device_id` INT NOT NULL COMMENT 'Foreign key referencing devices.id',
    `issue_description` TEXT NOT NULL COMMENT 'Detailed breakdown of the problem',
    `status` ENUM('Pending', 'InProgress', 'Completed') NOT NULL DEFAULT 'Pending' COMMENT 'Issue resolution progress',
    `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Log reporting timestamp',
    `completed_at` TIMESTAMP NULL DEFAULT NULL COMMENT 'Resolution completion timestamp',
    INDEX `idx_maintenance_device` (`device_id`),
    INDEX `idx_maintenance_status` (`status`),
    CONSTRAINT `fk_maintenance_device`
        FOREIGN KEY (`device_id`)
        REFERENCES `devices` (`id`)
        ON DELETE CASCADE
        ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Enable foreign key checks back
SET FOREIGN_KEY_CHECKS = 1;

-- =============================================================================
-- 5. Sample Data for Testing and Verification
-- =============================================================================

-- Sample devices across multiple rooms and types
INSERT INTO `devices` (`id`, `device_code`, `device_name`, `device_type`, `room`, `status`, `created_at`) VALUES
(1, 'PC-LAB101-01', 'Dell OptiPlex 7090 MT', 'PC', 'LAB101', 'Active', '2026-09-01 08:00:00'),
(2, 'MON-LAB101-01', 'Dell UltraSharp U2422H 24"', 'Monitor', 'LAB101', 'Active', '2026-09-01 08:05:00'),
(3, 'KB-LAB101-01', 'Logitech K120 USB Keyboard', 'Keyboard', 'LAB101', 'Active', '2026-09-01 08:10:00'),
(4, 'MS-LAB101-01', 'Logitech B100 Optical Mouse', 'Mouse', 'LAB101', 'Broken', '2026-09-01 08:12:00'),
(5, 'PC-LAB101-02', 'Dell OptiPlex 7090 MT', 'PC', 'LAB101', 'Repairing', '2026-09-01 08:15:00'),
(6, 'MON-LAB101-02', 'Dell UltraSharp U2422H 24"', 'Monitor', 'LAB101', 'Active', '2026-09-01 08:20:00'),
(7, 'SW-LAB101-01', 'Cisco Catalyst 2960 24-Port Switch', 'Network', 'LAB101', 'Active', '2026-09-01 07:30:00'),
(8, 'PC-LAB102-01', 'HP ProDesk 600 G6 Microtower', 'PC', 'LAB102', 'Active', '2026-09-02 09:00:00'),
(9, 'MON-LAB102-01', 'HP E24 G4 FHD Monitor', 'Monitor', 'LAB102', 'Broken', '2026-09-02 09:05:00'),
(10, 'RT-LAB102-01', 'MikroTik Cloud Router RB4011', 'Network', 'LAB102', 'Active', '2026-09-02 07:45:00'),
(11, 'PC-LAB102-02', 'HP ProDesk 600 G6 Microtower', 'PC', 'LAB102', 'Repairing', '2026-09-02 09:10:00'),
(12, 'KB-LAB102-01', 'HP 125 Wired Keyboard', 'Keyboard', 'LAB102', 'Active', '2026-09-02 09:15:00')
ON DUPLICATE KEY UPDATE `device_name` = VALUES(`device_name`);

-- Sample maintenance logs
INSERT INTO `maintenance_logs` (`id`, `device_id`, `issue_description`, `status`, `created_at`, `completed_at`) VALUES
(1, 4, 'Chuột bị liệt nút bấm chuột trái và đứt dây cáp USB ngầm.', 'Pending', '2026-10-01 10:15:00', NULL),
(2, 5, 'Máy tính tự tắt sau 15 phút hoạt động, quạt tản nhiệt CPU kêu to và quá nhiệt.', 'InProgress', '2026-10-02 14:30:00', NULL),
(3, 9, 'Màn hình xuất hiện sọc dọc màu xanh lá cây và chập chờn nguồn.', 'Pending', '2026-10-05 09:00:00', NULL),
(4, 11, 'Máy tính không nhận ổ cứng SSD, lỗi khởi động No Boot Device Found.', 'InProgress', '2026-10-08 11:20:00', NULL),
(5, 1, 'Bảo trì định kỳ: vệ sinh bụi bẩn, tra keo tản nhiệt và cập nhật BIOS.', 'Completed', '2026-09-15 08:30:00', '2026-09-15 11:00:00')
ON DUPLICATE KEY UPDATE `issue_description` = VALUES(`issue_description`);
