<?php
/**
 * Catalog vote endpoint — thumbs up/down per candidate
 *
 * POST /api/v1/catalog-vote.php
 * Body: {"slug": "less", "vote": 1}  (1 = up, -1 = down)
 */

require_once dirname(__DIR__, 2) . '/db.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    amiport_json(['ok' => false, 'error' => 'Method not allowed'], 405);
}

$input = json_decode(file_get_contents('php://input'), true);
if (!is_array($input)) {
    amiport_json(['ok' => false, 'error' => 'Invalid JSON'], 400);
}

$slug = $input['slug'] ?? '';
$vote = $input['vote'] ?? null;

if (!is_string($slug) || !preg_match('/^[a-z0-9_-]+$/', $slug)) {
    amiport_json(['ok' => false, 'error' => 'Invalid slug'], 400);
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
        'INSERT INTO catalog_votes (candidate_slug, ip_hash, vote, voted_at)
         VALUES (?, ?, ?, NOW())
         ON DUPLICATE KEY UPDATE vote = VALUES(vote), voted_at = NOW()'
    );
    $stmt->execute([$slug, amiport_ip_hash(), $vote]);

    // Return updated tally
    $stmt = $pdo->prepare(
        'SELECT
            SUM(CASE WHEN vote = 1 THEN 1 ELSE 0 END) as votes_up,
            SUM(CASE WHEN vote = -1 THEN 1 ELSE 0 END) as votes_down
         FROM catalog_votes WHERE candidate_slug = ?'
    );
    $stmt->execute([$slug]);
    $row = $stmt->fetch();

    $up = (int) ($row['votes_up'] ?? 0);
    $down = (int) ($row['votes_down'] ?? 0);

    amiport_json([
        'ok' => true,
        'votes_up' => $up,
        'votes_down' => $down,
        'score' => $up - $down,
        'your_vote' => $vote,
    ]);
} catch (PDOException $e) {
    error_log('amiport catalog vote failed: ' . $e->getMessage());
    amiport_json(['ok' => false, 'error' => 'Could not save vote'], 500);
}
