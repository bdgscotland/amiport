<?php
/**
 * Vote endpoint — thumbs up/down per package
 *
 * POST /api/v1/vote.php
 * Body: {"package": "grep", "vote": 1}  (1 = up, -1 = down)
 */

require_once dirname(__DIR__, 2) . '/db.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    amiport_json(['ok' => false, 'error' => 'Method not allowed'], 405);
}

$input = json_decode(file_get_contents('php://input'), true);
if (!is_array($input)) {
    amiport_json(['ok' => false, 'error' => 'Invalid JSON'], 400);
}

$packageName = $input['package'] ?? '';
$vote = $input['vote'] ?? null;

if (!is_string($packageName) || !preg_match('/^[a-z0-9_-]+$/', $packageName)) {
    amiport_json(['ok' => false, 'error' => 'Invalid package name'], 400);
}

if ($vote !== 1 && $vote !== -1) {
    amiport_json(['ok' => false, 'error' => 'Invalid vote value (must be 1 or -1)'], 400);
}

$pdo = amiport_db();
if ($pdo === null) {
    amiport_json(['ok' => false, 'error' => 'Could not save vote'], 500);
}

try {
    // UPSERT: insert or update existing vote
    $stmt = $pdo->prepare(
        'INSERT INTO votes (package_name, ip_hash, vote, voted_at)
         VALUES (?, ?, ?, NOW())
         ON DUPLICATE KEY UPDATE vote = VALUES(vote), voted_at = NOW()'
    );
    $stmt->execute([$packageName, amiport_ip_hash(), $vote]);

    // Return updated tally
    $stmt = $pdo->prepare(
        'SELECT
            SUM(CASE WHEN vote = 1 THEN 1 ELSE 0 END) as votes_up,
            SUM(CASE WHEN vote = -1 THEN 1 ELSE 0 END) as votes_down
         FROM votes WHERE package_name = ?'
    );
    $stmt->execute([$packageName]);
    $row = $stmt->fetch();

    $up = (int) ($row['votes_up'] ?? 0);
    $down = (int) ($row['votes_down'] ?? 0);

    amiport_json([
        'ok' => true,
        'votes_up' => $up,
        'votes_down' => $down,
        'score' => $up - $down,
    ]);
} catch (PDOException $e) {
    error_log('amiport vote failed: ' . $e->getMessage());
    amiport_json(['ok' => false, 'error' => 'Could not save vote'], 500);
}
