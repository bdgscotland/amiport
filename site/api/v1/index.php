<?php
/**
 * API health/info endpoint
 * GET /api/v1/
 */

header('Content-Type: application/json; charset=utf-8');
// CORS: allow same-site JS to call API (amiget CLI doesn't use CORS)
header('Access-Control-Allow-Origin: https://amiport.platesteel.net');
header('Access-Control-Allow-Methods: GET');
header('Cache-Control: no-cache');

echo json_encode([
    'name'    => 'amiport',
    'version' => 1,
    'status'  => 'ok',
    'endpoints' => [
        'packages' => '/api/v1/packages.json',
        'package'  => '/api/v1/packages.php?name={name}',
        'download' => '/api/v1/download.php?name={name}',
    ],
], JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
