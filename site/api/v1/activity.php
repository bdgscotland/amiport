<?php
/**
 * Activity feed endpoint -- recent port publications, requests, milestones
 *
 * GET /api/v1/activity.php
 * GET /api/v1/activity.php?format=html
 */

require_once dirname(__DIR__, 2) . '/db.php';

header('Cache-Control: public, max-age=300');

// Validate format parameter
$validFormats = ['json', 'html'];
$format = 'json';
if (isset($_GET['format'])) {
    $raw = strtolower(trim($_GET['format']));
    if (in_array($raw, $validFormats, true)) {
        $format = $raw;
    }
}

// --- Cache ---

$cacheFile = sys_get_temp_dir() . '/amiport-activity-cache.json';
$cacheTtl = 300; // 5 minutes

if (file_exists($cacheFile) && (time() - filemtime($cacheFile)) < $cacheTtl) {
    $cached = file_get_contents($cacheFile);
    if ($cached !== false) {
        $items = json_decode($cached, true);
        if (is_array($items)) {
            if ($format === 'html') {
                renderHtml($items);
            }
            amiport_json($items);
        }
    }
}

// --- Collect activity items ---

$items = [];

// 1. Published packages from JSON files (last 90 days)
$dataDir = dirname(__DIR__, 2) . '/data/packages';
$cutoff = date('c', strtotime('-90 days'));
$packageFiles = glob($dataDir . '/*.json');
if (is_array($packageFiles)) {
    foreach ($packageFiles as $file) {
        $data = json_decode(file_get_contents($file), true);
        if (!is_array($data)) {
            continue;
        }
        $publishedAt = $data['published_at'] ?? '';
        if ($publishedAt === '' || $publishedAt < $cutoff) {
            continue;
        }
        $name = $data['name'] ?? basename($file, '.json');
        $version = $data['version'] ?? '';
        $title = $name;
        if ($version !== '') {
            $title .= ' ' . $version;
        }
        $title .= ' published';
        $items[] = [
            'type' => 'port_published',
            'title' => $title,
            'timestamp' => $publishedAt,
            'url' => '/packages/?name=' . urlencode($name),
        ];
    }
}

// 2-4. Database-sourced events (graceful degradation)
$pdo = amiport_db();
if ($pdo !== null) {
    try {
        // 2. Recent port requests (last 90 days, exclude test data)
        $stmt = $pdo->prepare(
            'SELECT tool_name, requested_at
             FROM port_requests
             WHERE requested_at >= DATE_SUB(NOW(), INTERVAL 90 DAY)
               AND tool_name NOT LIKE \'%test%\'
               AND tool_name NOT LIKE \'%migration%\'
               AND tool_name NOT LIKE \'%spam%\'
             ORDER BY requested_at DESC
             LIMIT 10'
        );
        $stmt->execute();
        foreach ($stmt->fetchAll() as $row) {
            $items[] = [
                'type' => 'request_submitted',
                'title' => $row['tool_name'] . ' requested by the community',
                'timestamp' => date('c', strtotime($row['requested_at'])),
                'url' => '/catalog.html',
            ];
        }

        // 3. Milestone detection: check download counts against thresholds
        $thresholds = [50, 100, 500, 1000];
        $counts = $pdo->query(
            'SELECT package_name, COUNT(*) as cnt FROM downloads GROUP BY package_name'
        )->fetchAll();

        // Ensure milestones table exists (CREATE TABLE IF NOT EXISTS)
        $pdo->exec(
            'CREATE TABLE IF NOT EXISTS milestones (
                id INT AUTO_INCREMENT PRIMARY KEY,
                package_name VARCHAR(100) NOT NULL,
                threshold INT NOT NULL,
                reached_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
                UNIQUE KEY uq_pkg_threshold (package_name, threshold)
            )'
        );

        foreach ($counts as $row) {
            $pkg = $row['package_name'];
            $cnt = (int) $row['cnt'];
            foreach ($thresholds as $t) {
                if ($cnt >= $t) {
                    // INSERT IGNORE: unique key prevents duplicates
                    $ins = $pdo->prepare(
                        'INSERT IGNORE INTO milestones (package_name, threshold, reached_at) VALUES (?, ?, NOW())'
                    );
                    $ins->execute([$pkg, $t]);
                }
            }
        }

        // 4. Fetch milestones reached in last 90 days
        $stmt = $pdo->prepare(
            'SELECT package_name, threshold, reached_at
             FROM milestones
             WHERE reached_at >= DATE_SUB(NOW(), INTERVAL 90 DAY)
             ORDER BY reached_at DESC
             LIMIT 20'
        );
        $stmt->execute();
        foreach ($stmt->fetchAll() as $row) {
            $items[] = [
                'type' => 'milestone',
                'title' => $row['package_name'] . ' reached ' . $row['threshold'] . ' downloads',
                'timestamp' => date('c', strtotime($row['reached_at'])),
                'url' => '/packages/?name=' . urlencode($row['package_name']),
            ];
        }
    } catch (PDOException $e) {
        error_log('amiport activity DB query failed: ' . $e->getMessage());
        // Continue with JSON-sourced events only
    }
}

// Sort newest-first, limit to 15 (homepage shows ~10 via JS)
usort($items, function ($a, $b) {
    return strcmp($b['timestamp'], $a['timestamp']);
});
$items = array_slice($items, 0, 15);

// Write cache
file_put_contents($cacheFile, json_encode($items, JSON_UNESCAPED_SLASHES));

if ($format === 'html') {
    renderHtml($items);
}

amiport_json($items);

// --- HTML renderer ---

function renderHtml(array $items): void
{
    header('Content-Type: text/html; charset=utf-8');
    echo '<!DOCTYPE html><html><head><meta charset="utf-8"><title>amiport activity</title></head><body>';
    echo '<h1>Recent Activity</h1>';
    if (count($items) === 0) {
        echo '<p>No recent activity.</p>';
    } else {
        echo '<ul>';
        foreach ($items as $item) {
            $title = htmlspecialchars($item['title'], ENT_QUOTES, 'UTF-8');
            $ts = htmlspecialchars($item['timestamp'], ENT_QUOTES, 'UTF-8');
            $url = htmlspecialchars($item['url'], ENT_QUOTES, 'UTF-8');
            echo '<li>';
            if ($url !== '') {
                echo '<a href="' . $url . '">' . $title . '</a>';
            } else {
                echo $title;
            }
            echo ' <small>' . $ts . '</small>';
            echo '</li>';
        }
        echo '</ul>';
    }
    echo '</body></html>';
    exit;
}
