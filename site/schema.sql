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
    tool_why VARCHAR(500),
    tool_setup VARCHAR(200),
    ip_hash VARCHAR(64) NOT NULL,
    status ENUM('pending', 'accepted', 'rejected', 'completed') DEFAULT 'pending',
    admin_notes TEXT,
    requested_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS catalog_votes (
    id INT AUTO_INCREMENT PRIMARY KEY,
    candidate_slug VARCHAR(50) NOT NULL,
    ip_hash VARCHAR(64) NOT NULL,
    vote TINYINT NOT NULL DEFAULT 1,
    voted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uq_catalog_vote (candidate_slug, ip_hash),
    INDEX idx_slug (candidate_slug)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS milestones (
    id INT AUTO_INCREMENT PRIMARY KEY,
    package_name VARCHAR(50) NOT NULL,
    milestone_value INT NOT NULL,
    reached_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uq_milestone (package_name, milestone_value)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS bug_reports (
    id INT AUTO_INCREMENT PRIMARY KEY,
    package_name VARCHAR(64) NOT NULL,
    description TEXT NOT NULL,
    command VARCHAR(512),
    setup VARCHAR(200),
    email VARCHAR(255),
    ip_hash VARCHAR(64) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_package (package_name),
    INDEX idx_date (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Migration: add tool_why and tool_setup to existing port_requests table
-- ALTER TABLE port_requests ADD COLUMN tool_why VARCHAR(500) AFTER tool_url;
-- ALTER TABLE port_requests ADD COLUMN tool_setup VARCHAR(200) AFTER tool_why;

-- Migration: add pipeline_status column alongside old status (reversible)
-- ALTER TABLE port_requests ADD COLUMN pipeline_status ENUM('requested','evaluating','in_progress','testing','shipped','declined') NOT NULL DEFAULT 'requested' AFTER status;
-- ALTER TABLE port_requests ADD COLUMN status_updated_at DATETIME DEFAULT NULL AFTER pipeline_status;
-- UPDATE port_requests SET pipeline_status = 'requested' WHERE status = 'pending';
-- UPDATE port_requests SET pipeline_status = 'evaluating' WHERE status = 'accepted';
-- UPDATE port_requests SET pipeline_status = 'shipped' WHERE status = 'completed';
-- UPDATE port_requests SET pipeline_status = 'declined' WHERE status = 'rejected';
