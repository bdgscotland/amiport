<?php
/**
 * Catalog endpoint — serves the porting tech tree catalog
 *
 * GET /api/v1/catalog.php                              — Full catalog
 * GET /api/v1/catalog.php?category=cli                 — Filter by category
 * GET /api/v1/catalog.php?tier=ready                   — Filter by readiness tier
 * GET /api/v1/catalog.php?profile=a1200_accel          — Filter by hardware profile fit
 */

require_once dirname(__DIR__, 2) . '/db.php';

header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: https://amiport.platesteel.net');
header('Access-Control-Allow-Methods: GET');
header('Cache-Control: public, max-age=60');

$catalogPath = dirname(__DIR__, 2) . '/data/catalog.json';

if (!file_exists($catalogPath)) {
    http_response_code(404);
    echo json_encode(['error' => 'Catalog not found']);
    exit;
}

$raw = file_get_contents($catalogPath);
if ($raw === false) {
    http_response_code(500);
    echo json_encode(['error' => 'Failed to read catalog']);
    exit;
}

$catalog = json_decode($raw, true);
if ($catalog === null) {
    http_response_code(500);
    echo json_encode(['error' => 'Invalid catalog JSON']);
    exit;
}

// Apply filters
$category = isset($_GET['category']) ? preg_replace('/[^a-z_]/', '', $_GET['category']) : null;
$tier = isset($_GET['tier']) ? preg_replace('/[^a-z]/', '', $_GET['tier']) : null;
$profile = isset($_GET['profile']) ? preg_replace('/[^a-z0-9_]/', '', $_GET['profile']) : null;

if ($category || $tier || $profile) {
    $filtered = [];
    foreach ($catalog['candidates'] as $c) {
        if ($category && ($c['amiport_category'] ?? '') !== $category) continue;
        if ($tier && ($c['readiness']['tier'] ?? '') !== $tier) continue;
        if ($profile) {
            $fit = $c['hardware_fit'][$profile] ?? 'unknown';
            if (!in_array($fit, ['fits', 'tight'])) continue;
        }
        $filtered[] = $c;
    }
    $catalog['candidates'] = $filtered;
}

// Add summary stats
$tiers = ['ready' => 0, 'feasible' => 0, 'blocked' => 0, 'infeasible' => 0, 'unanalyzed' => 0];
foreach ($catalog['candidates'] as $c) {
    $t = $c['readiness']['tier'] ?? 'unanalyzed';
    if (isset($tiers[$t])) $tiers[$t]++;
    else $tiers['unanalyzed']++;
}

$shimTotal = count($catalog['shim_functions'] ?? []);
$shimImpl = 0;
foreach ($catalog['shim_functions'] ?? [] as $s) {
    if (($s['status'] ?? '') === 'implemented') $shimImpl++;
}

$catalog['summary'] = [
    'candidates' => count($catalog['candidates']),
    'ported' => count($catalog['ported'] ?? []),
    'tiers' => $tiers,
    'shim_coverage' => $shimTotal > 0 ? round($shimImpl / $shimTotal * 100) : 0,
    'shim_implemented' => $shimImpl,
    'shim_total' => $shimTotal,
];

// Load community vote counts
$voteMap = [];
$pdo = amiport_db();
if ($pdo !== null) {
    try {
        $stmt = $pdo->query(
            'SELECT candidate_slug,
                SUM(CASE WHEN vote = 1 THEN 1 ELSE 0 END) as up,
                SUM(CASE WHEN vote = -1 THEN 1 ELSE 0 END) as dn
             FROM catalog_votes GROUP BY candidate_slug'
        );
        while ($row = $stmt->fetch()) {
            $voteMap[$row['candidate_slug']] = [
                'up' => (int) $row['up'],
                'down' => (int) $row['dn'],
                'score' => (int) $row['up'] - (int) $row['dn'],
            ];
        }
    } catch (PDOException $e) {
        error_log('amiport catalog votes query failed: ' . $e->getMessage());
    }
}

// Add community_votes to each candidate
for ($i = 0; $i < count($catalog['candidates']); $i++) {
    $slug = $catalog['candidates'][$i]['id'] ?? '';
    $catalog['candidates'][$i]['community_votes'] = $voteMap[$slug] ?? ['up' => 0, 'down' => 0, 'score' => 0];
}

// Build shim unlock opportunities — show ALL unimplemented shims, not just those with unlocks
$unlocks = [];
foreach ($catalog['shim_functions'] ?? [] as $name => $s) {
    if (($s['status'] ?? '') !== 'implemented') {
        $count = $s['unlocks_count'] ?? count($s['unlocks'] ?? []);
        $unlocks[] = ['name' => $name, 'tier' => $s['tier'] ?? 0, 'unlocks' => $count, 'complexity' => $s['complexity'] ?? '?'];
    }
}
usort($unlocks, function($a, $b) { return $b['unlocks'] - $a['unlocks']; });
$catalog['summary']['top_unlocks'] = $unlocks;

echo json_encode($catalog, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
