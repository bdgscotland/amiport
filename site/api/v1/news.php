<?php
/**
 * news.php -- public JSON proxy for site/data/news.json
 *
 * The /data/ directory is blocked by site/.htaccess. Browsers fetch news
 * via this endpoint instead. Server-side consumers (feed.php, activity.php)
 * read site/data/news.json directly from disk -- they do not use this
 * endpoint.
 */

header('Content-Type: application/json; charset=UTF-8');
header('Cache-Control: public, max-age=300');
header('Access-Control-Allow-Origin: https://amiport.platesteel.net');

$newsFile = dirname(__DIR__, 2) . '/data/news.json';

if (!file_exists($newsFile)) {
    http_response_code(404);
    echo json_encode(['error' => 'news not found']);
    exit;
}

$raw = file_get_contents($newsFile);
if ($raw === false) {
    http_response_code(500);
    echo json_encode(['error' => 'failed to read news']);
    exit;
}

// Validate it parses before echoing -- fail closed on a corrupt file.
$decoded = json_decode($raw, true);
if ($decoded === null && json_last_error() !== JSON_ERROR_NONE) {
    http_response_code(500);
    echo json_encode(['error' => 'news feed is malformed']);
    exit;
}

echo $raw;
