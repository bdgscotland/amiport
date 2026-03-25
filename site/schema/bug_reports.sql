-- Bug reports table for amiport
-- Run on Dreamhost MySQL after deploying report-bug.php

CREATE TABLE IF NOT EXISTS bug_reports (
    id         INT AUTO_INCREMENT PRIMARY KEY,
    package    VARCHAR(100) NOT NULL,
    description TEXT NOT NULL,
    command    VARCHAR(500) DEFAULT NULL,
    setup      VARCHAR(200) DEFAULT NULL,
    email      VARCHAR(200) DEFAULT NULL,
    github_url VARCHAR(512) DEFAULT NULL,
    ip_hash    CHAR(64) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_package (package),
    INDEX idx_ip_created (ip_hash, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
