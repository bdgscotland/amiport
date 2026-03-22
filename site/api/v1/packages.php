<?php
/**
 * Package list / single package endpoint
 *
 * GET /api/v1/packages.php           — Full package manifest
 * GET /api/v1/packages.php?name=grep — Single package detail
 */

header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: https://amiport.platesteel.net');
header('Access-Control-Allow-Methods: GET');
header('Cache-Control: public, max-age=300');

$dataDir = dirname(__DIR__, 2) . '/data/packages';

/**
 * Sanitize package data: extract only known fields with strict types.
 */
function sanitize_package(array $raw): array
{
    $pkg = [];
    $stringFields = ['name', 'version', 'description', 'category', 'source',
                     'license', 'download', 'aminet', 'sha256'];
    foreach ($stringFields as $field) {
        if (isset($raw[$field]) && is_string($raw[$field])) {
            $pkg[$field] = $raw[$field];
        }
    }
    $intFields = ['size', 'stack'];
    foreach ($intFields as $field) {
        if (isset($raw[$field]) && is_numeric($raw[$field])) {
            $pkg[$field] = (int) $raw[$field];
        }
    }
    if (isset($raw['requires']) && is_array($raw['requires'])) {
        $pkg['requires'] = array_values(array_filter($raw['requires'], 'is_string'));
    } else {
        $pkg['requires'] = [];
    }
    return $pkg;
}

// --- Load ALL packages from disk (no user input in file paths) ---
$packages = [];
$files = glob($dataDir . '/*.json');

if ($files) {
    foreach ($files as $file) {
        $data = file_get_contents($file);
        if ($data === false) {
            continue;
        }
        $raw = json_decode($data, true);
        if (!is_array($raw)) {
            continue;
        }
        $packages[] = sanitize_package($raw);
    }
}

usort($packages, function (array $a, array $b): int {
    return strcasecmp($a['name'] ?? '', $b['name'] ?? '');
});

// --- Augment with MySQL download counts and vote scores ---
require_once dirname(__DIR__, 2) . '/db.php';
$stats = [];
try {
    $pdo = amiport_db();
    if ($pdo !== null) {
        // Download counts
        $rows = $pdo->query(
            'SELECT package_name, COUNT(*) as downloads FROM downloads GROUP BY package_name'
        )->fetchAll();
        foreach ($rows as $row) {
            $stats[$row['package_name']]['downloads'] = (int) $row['downloads'];
        }
        // Vote scores
        $rows = $pdo->query(
            'SELECT package_name, SUM(CASE WHEN vote=1 THEN 1 ELSE 0 END) as up, SUM(CASE WHEN vote=-1 THEN 1 ELSE 0 END) as down FROM votes GROUP BY package_name'
        )->fetchAll();
        foreach ($rows as $row) {
            $stats[$row['package_name']]['votes_up'] = (int) $row['up'];
            $stats[$row['package_name']]['votes_down'] = (int) $row['down'];
            $stats[$row['package_name']]['vote_score'] = (int) $row['up'] - (int) $row['down'];
        }
    }
} catch (PDOException $e) {
    error_log('amiport packages stats query failed: ' . $e->getMessage());
}

// Merge stats into packages
foreach ($packages as &$pkg) {
    $name = $pkg['name'] ?? '';
    $pkg['downloads'] = $stats[$name]['downloads'] ?? 0;
    $pkg['votes_up'] = $stats[$name]['votes_up'] ?? 0;
    $pkg['votes_down'] = $stats[$name]['votes_down'] ?? 0;
    $pkg['vote_score'] = $stats[$name]['vote_score'] ?? 0;
}
unset($pkg);

// --- Single package request: filter from already-loaded list ---
if (isset($_GET['name'])) {
    $requestedName = (string) $_GET['name'];

    // Validate: alphanumeric, hyphens, underscores only
    if (!preg_match('/^[a-z0-9_-]+$/', $requestedName)) {
        http_response_code(404);
        echo json_encode(['error' => 'Package not found']);
        exit;
    }

    // Find in the already-loaded (untainted) packages array
    $found = null;
    foreach ($packages as $pkg) {
        if (isset($pkg['name']) && $pkg['name'] === $requestedName) {
            $found = $pkg;
            break;
        }
    }

    if ($found === null) {
        http_response_code(404);
        echo json_encode(['error' => 'Package not found']);
        exit;
    }

    echo json_encode($found, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
    exit;
}

// --- Full manifest ---
echo json_encode([
    'version'   => 1,
    'generated' => gmdate('Y-m-d\TH:i:s\Z'),
    'base_url'  => 'http://amiport.platesteel.net',
    'packages'  => $packages,
], JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
