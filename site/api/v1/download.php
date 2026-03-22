<?php
/**
 * Download endpoint — serve LHA file + increment counter
 *
 * GET /api/v1/download.php?name=grep
 *
 * Loads all package metadata via glob (no user input in file paths),
 * matches the requested name, then serves the corresponding LHA file.
 * Increments a per-package download counter using flock().
 */

// Validate name parameter
if (!isset($_GET['name'])) {
    http_response_code(400);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode(['error' => 'Missing name parameter']);
    exit;
}

$requestedName = (string) $_GET['name'];

if (!preg_match('/^[a-z0-9_-]+$/', $requestedName)) {
    http_response_code(404);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode(['error' => 'Package not found']);
    exit;
}

$siteRoot   = dirname(__DIR__, 2);
$dataDir    = $siteRoot . '/data/packages';
$pkgDir     = $siteRoot . '/packages';
$counterDir = $siteRoot . '/data/counters';

// Load ALL package metadata via glob — no user input in file paths
$found = null;
$files = glob($dataDir . '/*.json');

if ($files) {
    foreach ($files as $file) {
        $data = file_get_contents($file);
        if ($data === false) {
            continue;
        }
        $meta = json_decode($data, true);
        if (!is_array($meta)) {
            continue;
        }
        // Match by name — compare user input against trusted data
        if (isset($meta['name']) && $meta['name'] === $requestedName) {
            $found = $meta;
            break;
        }
    }
}

if ($found === null || !isset($found['version']) || !is_string($found['version'])) {
    http_response_code(404);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode(['error' => 'Package not found']);
    exit;
}

// Use trusted data from the package metadata (not user input) for paths
$pkgName = $found['name'];
$version = $found['version'];

// Validate version format from trusted data
if (!preg_match('/^[0-9][0-9a-z._-]*$/', $version)) {
    http_response_code(500);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode(['error' => 'Invalid version format']);
    exit;
}

// Construct LHA path from trusted metadata
$lhaFile = $pkgDir . '/' . $pkgName . '-' . $version . '.lha';

// Resolve to real path and verify it's within the packages directory
$realPath = realpath($lhaFile);
$realPkgDir = realpath($pkgDir);

if ($realPath === false || $realPkgDir === false || strpos($realPath, $realPkgDir) !== 0) {
    http_response_code(404);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode(['error' => 'Package file not found']);
    exit;
}

// Track download in MySQL (best-effort — file still serves if DB is down)
require_once dirname(__DIR__, 2) . '/db.php';
try {
    $pdo = amiport_db();
    if ($pdo !== null) {
        $stmt = $pdo->prepare(
            'INSERT INTO downloads (package_name, ip_hash, user_agent) VALUES (?, ?, ?)'
        );
        $stmt->execute([
            $pkgName,
            amiport_ip_hash(),
            substr($_SERVER['HTTP_USER_AGENT'] ?? '', 0, 255)
        ]);
    }
} catch (PDOException $e) {
    error_log('amiport download tracking failed: ' . $e->getMessage());
}

// Serve the LHA file
$fileSize = filesize($realPath);
header('Content-Type: application/octet-stream');
header('Content-Disposition: attachment; filename="' . $pkgName . '-' . $version . '.lha"');
if ($fileSize !== false) {
    header('Content-Length: ' . $fileSize);
}
header('Cache-Control: public, max-age=86400');

readfile($realPath);
exit;
