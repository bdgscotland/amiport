<?php
/**
 * Database connection singleton and .env loader for amiport.
 *
 * Usage:
 *   require_once __DIR__ . '/db.php';
 *   $pdo = amiport_db();
 *   $salt = amiport_env('IP_SALT');
 */

/**
 * Load .env file using a simple line parser.
 * NOT parse_ini_file() — that corrupts bcrypt hashes containing $ characters.
 */
function amiport_load_env(): array
{
    static $env = null;
    if ($env !== null) {
        return $env;
    }

    $env = [];
    $path = __DIR__ . '/.env';
    if (!file_exists($path)) {
        return $env;
    }

    foreach (file($path, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES) as $line) {
        $line = trim($line);
        if ($line === '' || $line[0] === '#') {
            continue;
        }
        $parts = explode('=', $line, 2);
        if (count($parts) === 2) {
            $env[trim($parts[0])] = trim($parts[1]);
        }
    }

    return $env;
}

/**
 * Get an environment variable from .env.
 */
function amiport_env(string $key, string $default = ''): string
{
    $env = amiport_load_env();
    return $env[$key] ?? $default;
}

/**
 * Get PDO database connection (singleton).
 * Returns null if database is not configured or unreachable.
 */
function amiport_db(): ?PDO
{
    static $pdo = null;
    static $attempted = false;

    if ($pdo !== null) {
        return $pdo;
    }
    if ($attempted) {
        return null;
    }
    $attempted = true;

    $host = amiport_env('DB_HOST');
    $name = amiport_env('DB_NAME');
    $user = amiport_env('DB_USER');
    $pass = amiport_env('DB_PASS');

    if ($host === '' || $name === '' || $user === '' || $pass === '') {
        return null;
    }

    try {
        $dsn = "mysql:host=$host;dbname=$name;charset=utf8mb4";
        $pdo = new PDO($dsn, $user, $pass, [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            PDO::ATTR_EMULATE_PREPARES => false,
        ]);
        return $pdo;
    } catch (PDOException $e) {
        error_log('amiport DB connection failed: ' . $e->getMessage());
        $pdo = null;
        return null;
    }
}

/**
 * Hash an IP address for privacy-preserving dedup.
 */
function amiport_ip_hash(): string
{
    $ip = $_SERVER['REMOTE_ADDR'] ?? '0.0.0.0';
    $salt = amiport_env('IP_SALT', 'amiport-default-salt');
    return hash('sha256', $ip . $salt);
}

/**
 * Send a JSON response and exit.
 */
function amiport_json(array $data, int $status = 200): void
{
    http_response_code($status);
    header('Content-Type: application/json; charset=utf-8');
    header('Access-Control-Allow-Origin: https://amiport.platesteel.net');
    header('Access-Control-Allow-Methods: GET, POST');
    echo json_encode($data, JSON_UNESCAPED_SLASHES);
    exit;
}

// Configure session security settings
ini_set('session.cookie_httponly', '1');
ini_set('session.cookie_samesite', 'Strict');
ini_set('session.gc_maxlifetime', '3600');

/**
 * Generate or retrieve a CSRF token for the current session.
 */
function amiport_csrf_token(): string
{
    if (session_status() !== PHP_SESSION_ACTIVE) {
        session_start();
    }
    if (empty($_SESSION['csrf_token'])) {
        $_SESSION['csrf_token'] = bin2hex(random_bytes(32));
    }
    return $_SESSION['csrf_token'];
}

/**
 * Validate a CSRF token against the session token.
 */
function amiport_csrf_check(string $token): bool
{
    if (session_status() !== PHP_SESSION_ACTIVE) {
        return false;
    }
    return isset($_SESSION['csrf_token']) && hash_equals($_SESSION['csrf_token'], $token);
}
