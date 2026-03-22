<?php
/**
 * Port request endpoint — submit a tool request
 *
 * POST /api/v1/request.php
 * Body: {"tool_name": "less", "tool_why": "...", "tool_setup": "...", "tool_url": "...", "website": ""}
 */

require_once dirname(__DIR__, 2) . '/db.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    amiport_json(['ok' => false, 'error' => 'Method not allowed'], 405);
}

$input = json_decode(file_get_contents('php://input'), true);
if (!is_array($input)) {
    amiport_json(['ok' => false, 'error' => 'Invalid JSON'], 400);
}

// Honeypot check: if the hidden "website" field is filled, it's a bot
$honeypot = $input['website'] ?? '';
if ($honeypot !== '') {
    // Fool the bot: return success but don't insert
    amiport_json(['ok' => true, 'message' => 'Request submitted!']);
}

$toolName = trim($input['tool_name'] ?? '');
$toolWhy = trim($input['tool_why'] ?? '');
$toolSetup = trim($input['tool_setup'] ?? '');
$toolUrl = trim($input['tool_url'] ?? '');

if ($toolName === '') {
    amiport_json(['ok' => false, 'error' => 'Tool name is required'], 400);
}

// Truncate to limits
$toolName = substr($toolName, 0, 100);
$toolWhy = substr($toolWhy, 0, 500);
$toolSetup = substr($toolSetup, 0, 200);
$toolUrl = substr($toolUrl, 0, 512);

$pdo = amiport_db();
if ($pdo === null) {
    amiport_json(['ok' => false, 'error' => 'Could not submit request'], 500);
}

try {
    $stmt = $pdo->prepare(
        'INSERT INTO port_requests (tool_name, tool_url, tool_why, tool_setup, ip_hash) VALUES (?, ?, ?, ?, ?)'
    );
    $stmt->execute([$toolName, $toolUrl ?: null, $toolWhy ?: null, $toolSetup ?: null, amiport_ip_hash()]);

    amiport_json(['ok' => true, 'message' => "Request submitted! We'll review it soon."]);
} catch (PDOException $e) {
    // If new columns don't exist yet, fall back to original schema
    if (strpos($e->getMessage(), 'Unknown column') !== false) {
        try {
            $stmt = $pdo->prepare(
                'INSERT INTO port_requests (tool_name, tool_url, ip_hash) VALUES (?, ?, ?)'
            );
            $stmt->execute([$toolName, $toolUrl ?: null, amiport_ip_hash()]);
            amiport_json(['ok' => true, 'message' => "Request submitted! We'll review it soon."]);
        } catch (PDOException $e2) {
            error_log('amiport request failed: ' . $e2->getMessage());
            amiport_json(['ok' => false, 'error' => 'Could not submit request'], 500);
        }
    } else {
        error_log('amiport request failed: ' . $e->getMessage());
        amiport_json(['ok' => false, 'error' => 'Could not submit request'], 500);
    }
}
