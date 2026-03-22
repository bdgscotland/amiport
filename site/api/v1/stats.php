<?php
/**
 * Stats endpoint — aggregated download/vote/request statistics
 *
 * GET /api/v1/stats.php
 */

require_once dirname(__DIR__, 2) . '/db.php';

header('Cache-Control: public, max-age=300');

$pdo = amiport_db();
if ($pdo === null) {
    amiport_json(['ok' => false, 'error' => 'Stats unavailable'], 500);
}

try {
    // Total downloads
    $totalDownloads = (int) $pdo->query('SELECT COUNT(*) FROM downloads')->fetchColumn();

    // Total packages (from JSON files, not DB)
    $dataDir = dirname(__DIR__, 2) . '/data/packages';
    $totalPackages = count(glob($dataDir . '/*.json'));

    // Total votes
    $totalVotes = (int) $pdo->query('SELECT COUNT(*) FROM votes')->fetchColumn();

    // Total port requests
    $totalRequests = (int) $pdo->query('SELECT COUNT(*) FROM port_requests')->fetchColumn();

    // Popular packages (top 10 by downloads)
    $popular = $pdo->query(
        'SELECT d.package_name as name,
                COUNT(*) as downloads,
                COALESCE(SUM(CASE WHEN v.vote = 1 THEN 1 ELSE 0 END), 0) as votes_up,
                COALESCE(SUM(CASE WHEN v.vote = -1 THEN 1 ELSE 0 END), 0) as votes_down
         FROM downloads d
         LEFT JOIN votes v ON d.package_name = v.package_name
         GROUP BY d.package_name
         ORDER BY downloads DESC
         LIMIT 10'
    )->fetchAll();

    // Cast types
    foreach ($popular as &$p) {
        $p['downloads'] = (int) $p['downloads'];
        $p['votes_up'] = (int) $p['votes_up'];
        $p['votes_down'] = (int) $p['votes_down'];
    }
    unset($p);

    // Recent download counts by package (7d, 30d)
    $recentDownloads = [];
    foreach (['7' => '7d', '30' => '30d'] as $days => $label) {
        $stmt = $pdo->prepare(
            "SELECT package_name as name, COUNT(*) as count
             FROM downloads
             WHERE downloaded_at >= DATE_SUB(NOW(), INTERVAL ? DAY)
             GROUP BY package_name
             ORDER BY count DESC"
        );
        $stmt->execute([$days]);
        foreach ($stmt->fetchAll() as $row) {
            $recentDownloads[] = [
                'name' => $row['name'],
                'count' => (int) $row['count'],
                'period' => $label,
            ];
        }
    }

    // Trends (total downloads in each window)
    $trends = [];
    foreach (['7' => '7d', '30' => '30d', '90' => '90d'] as $days => $label) {
        $stmt = $pdo->prepare(
            "SELECT COUNT(*) FROM downloads WHERE downloaded_at >= DATE_SUB(NOW(), INTERVAL ? DAY)"
        );
        $stmt->execute([$days]);
        $trends[$label] = (int) $stmt->fetchColumn();
    }

    amiport_json([
        'total_downloads' => $totalDownloads,
        'total_packages' => $totalPackages,
        'total_votes' => $totalVotes,
        'total_requests' => $totalRequests,
        'popular' => $popular,
        'recent_downloads' => $recentDownloads,
        'trends' => $trends,
    ]);
} catch (PDOException $e) {
    error_log('amiport stats query failed: ' . $e->getMessage());
    amiport_json(['ok' => false, 'error' => 'Stats unavailable'], 500);
}
