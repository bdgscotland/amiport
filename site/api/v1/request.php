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

// --- Pipeline status update (admin only) ---
if (($input['action'] ?? '') === 'update_status') {
    session_start();
    if (!isset($_SESSION['admin']) || $_SESSION['admin'] !== true) {
        amiport_json(['ok' => false, 'error' => 'Unauthorized'], 403);
    }
    if (!amiport_csrf_check($input['csrf_token'] ?? '')) {
        amiport_json(['ok' => false, 'error' => 'Invalid CSRF token'], 403);
    }

    $requestId = (int) ($input['request_id'] ?? 0);
    $pipelineStatus = $input['pipeline_status'] ?? '';
    $validPipelineStatuses = ['requested', 'evaluating', 'in_progress', 'testing', 'shipped', 'declined'];

    if ($requestId <= 0) {
        amiport_json(['ok' => false, 'error' => 'Invalid request ID'], 400);
    }
    if (!in_array($pipelineStatus, $validPipelineStatuses, true)) {
        amiport_json(['ok' => false, 'error' => 'Invalid pipeline status'], 400);
    }

    $pdo = amiport_db();
    if ($pdo === null) {
        amiport_json(['ok' => false, 'error' => 'Database unavailable'], 500);
    }

    try {
        $stmt = $pdo->prepare(
            'UPDATE port_requests SET pipeline_status = ?, status_updated_at = NOW() WHERE id = ?'
        );
        $stmt->execute([$pipelineStatus, $requestId]);

        // Fetch the updated timestamp
        $stmt2 = $pdo->prepare('SELECT status_updated_at FROM port_requests WHERE id = ?');
        $stmt2->execute([$requestId]);
        $updatedAt = $stmt2->fetchColumn();

        amiport_json([
            'ok' => true,
            'pipeline_status' => $pipelineStatus,
            'status_updated_at' => $updatedAt ?: date('Y-m-d H:i:s'),
        ]);
    } catch (PDOException $e) {
        // Gracefully handle missing column
        if (strpos($e->getMessage(), 'Unknown column') !== false) {
            error_log('amiport: pipeline_status column missing from port_requests table');
            amiport_json(['ok' => false, 'error' => 'Pipeline status column not yet created in database'], 500);
        } else {
            error_log('amiport pipeline status update failed: ' . $e->getMessage());
            amiport_json(['ok' => false, 'error' => 'Could not update status'], 500);
        }
    }
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
