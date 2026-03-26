<?php
/**
 * feed.php — RSS 2.0 feed of amiport packages
 *
 * Reads data/packages/*.json, sorts by publish_date descending,
 * generates RSS 2.0 XML. Limit: 50 most recent items.
 */

header('Content-Type: application/rss+xml; charset=UTF-8');

$dataDir = __DIR__ . '/data/packages';
$packages = [];

// Optional category filter (validated: alphanumeric + / . _ - only)
$categoryFilter = '';
$categoryFilterEscaped = '';
if (isset($_GET['category']) && $_GET['category'] !== '') {
    $raw = $_GET['category'];
    if (preg_match('/^[a-zA-Z0-9_\/.\-]+$/', $raw)) {
        $categoryFilter = $raw;
        $categoryFilterEscaped = htmlspecialchars($categoryFilter, ENT_XML1, 'UTF-8');
    }
}

foreach (glob($dataDir . '/*.json') as $file) {
    $data = json_decode(file_get_contents($file), true);
    if ($data && isset($data['name']) && ($data['status'] ?? '') !== 'testing') {
        if ($categoryFilter !== '' && ($data['category'] ?? '') !== $categoryFilter) {
            continue;
        }
        $packages[] = $data;
    }
}

// Sort by publish date descending (most recent first)
usort($packages, function($a, $b) {
    $dateA = $a['published_at'] ?? $a['publish_date'] ?? '1970-01-01';
    $dateB = $b['published_at'] ?? $b['publish_date'] ?? '1970-01-01';
    return strcmp($dateB, $dateA);
});

// Limit to 50
$packages = array_slice($packages, 0, 50);

echo '<?xml version="1.0" encoding="UTF-8"?>' . "\n";
?>
<rss version="2.0">
  <channel>
    <title><?php echo $categoryFilterEscaped !== '' ? 'amiport — New Ports (' . $categoryFilterEscaped . ')' : 'amiport — New Ports'; ?></title>
    <link>https://amiport.platesteel.net</link>
    <description>POSIX tools ported to AmigaOS 3.x</description>
    <language>en-us</language>
    <lastBuildDate><?php echo gmdate('D, d M Y H:i:s') . ' +0000'; ?></lastBuildDate>
<?php foreach ($packages as $pkg): ?>
    <item>
      <title><?php echo htmlspecialchars($pkg['name'], ENT_XML1, 'UTF-8'); ?> <?php $feedRev = (int)($pkg['revision'] ?? 1); echo htmlspecialchars(($pkg['version'] ?? '') . ($feedRev > 1 ? '-' . $feedRev : ''), ENT_XML1, 'UTF-8'); ?></title>
      <link>https://amiport.platesteel.net/packages.html?name=<?php echo urlencode($pkg['name']); ?></link>
      <description><?php echo htmlspecialchars($pkg['description'] ?? '', ENT_XML1, 'UTF-8'); ?></description>
<?php
    $pubDate = $pkg['published_at'] ?? $pkg['publish_date'] ?? null;
    if ($pubDate):
        $ts = strtotime($pubDate);
        if ($ts !== false):
?>
      <pubDate><?php echo gmdate('D, d M Y H:i:s', $ts) . ' +0000'; ?></pubDate>
<?php
        endif;
    endif;
?>
      <guid isPermaLink="true">https://amiport.platesteel.net/packages.html?name=<?php echo urlencode($pkg['name']); ?></guid>
    </item>
<?php endforeach; ?>
  </channel>
</rss>
