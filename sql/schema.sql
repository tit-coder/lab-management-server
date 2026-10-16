-- ====================================================================
-- Do an: Thiet ke va xay dung Web Server bang C++ ket noi co so du lieu MariaDB
--        tren Ubuntu Linux de quan ly thiet bi phong may
-- File: sql/schema.sql
-- ====================================================================

CREATE DATABASE IF NOT EXISTS lab_management
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE lab_management;

-- 1. Bang quan ly thiet bi phong may (devices)
CREATE TABLE IF NOT EXISTS devices (
    id INT AUTO_INCREMENT PRIMARY KEY,
    device_code VARCHAR(30) NOT NULL UNIQUE,
    device_name VARCHAR(100) NOT NULL,
    device_type VARCHAR(50) NOT NULL,
    room VARCHAR(50) NOT NULL,
    status ENUM('Active', 'Broken', 'Repairing')
        NOT NULL DEFAULT 'Active',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_device_status (status),
    INDEX idx_device_room (room)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 2. Bang nhat ky bao tri / xu ly su co (maintenance_logs)
CREATE TABLE IF NOT EXISTS maintenance_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    device_id INT NOT NULL,
    description TEXT NOT NULL,
    status ENUM('Pending', 'InProgress', 'Completed')
        NOT NULL DEFAULT 'Pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    completed_at DATETIME NULL,

    CONSTRAINT fk_maintenance_device
        FOREIGN KEY (device_id)
        REFERENCES devices(id)
        ON DELETE RESTRICT,
    INDEX idx_maintenance_status (status),
    INDEX idx_maintenance_device_id (device_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 3. Du lieu mau ban dau (Idempotent seed data su dung INSERT IGNORE de khong pha huy du lieu hien co)
INSERT IGNORE INTO devices (id, device_code, device_name, device_type, room, status) VALUES
(1, 'PC-001', 'Computer 01', 'Computer', 'Room A', 'Active'),
(2, 'PC-002', 'Computer 02', 'Computer', 'Room A', 'Broken'),
(3, 'PC-003', 'Computer 03', 'Computer', 'Room A', 'Repairing'),
(4, 'MON-001', 'Monitor 01', 'Monitor', 'Room A', 'Active');

INSERT IGNORE INTO maintenance_logs (id, device_id, description, status) VALUES
(1, 2, 'Keyboard is not working', 'Pending');
