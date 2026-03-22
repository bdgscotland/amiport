-- amiport MySQL schema
-- Run once: mysql -u USER -p DB < schema.sql

CREATE TABLE IF NOT EXISTS downloads (
    id INT AUTO_INCREMENT PRIMARY KEY,
    package_name VARCHAR(64) NOT NULL,
    ip_hash VARCHAR(64) NOT NULL,
    user_agent VARCHAR(255),
    downloaded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_package (package_name),
    INDEX idx_date (downloaded_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS votes (
    id INT AUTO_INCREMENT PRIMARY KEY,
    package_name VARCHAR(64) NOT NULL,
    ip_hash VARCHAR(64) NOT NULL,
    vote TINYINT NOT NULL,
    voted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uq_vote (package_name, ip_hash),
    INDEX idx_package (package_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS login_attempts (
    id INT AUTO_INCREMENT PRIMARY KEY,
    ip_hash VARCHAR(64) NOT NULL,
    attempted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_ip_time (ip_hash, attempted_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS port_requests (
    id INT AUTO_INCREMENT PRIMARY KEY,
    tool_name VARCHAR(128) NOT NULL,
    tool_url VARCHAR(512),
    ip_hash VARCHAR(64) NOT NULL,
    status ENUM('pending', 'accepted', 'rejected', 'completed') DEFAULT 'pending',
    admin_notes TEXT,
    requested_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
