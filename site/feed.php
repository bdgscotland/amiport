<?php
/**
 * feed.php — RSS 2.0 feed of amiport packages + news
 *
 * Merges published packages (data/packages/*.json) and news entries
 * (data/news.json), sorts by date descending, emits RSS 2.0 XML.
 * Limit: 50 most recent items. The ?category= filter narrows packages
 * to that category and suppresses news (news is project-wide).
 */

header('Content-Type: application/rss+xml; charset=UTF-8');

$baseDir = __DIR__;
$dataDir = $baseDir . '/data/packages';
$newsFile = $baseDir . '/data/news.json';

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

$feedItems = [];

// --- Package items ---
foreach (glob($dataDir . '/*.json') as $file) {
    $data = json_decode(file_get_contents($file), true);
    if (!$data || !isset($data['name'])) continue;
    if (($data['status'] ?? '') === 'testing') continue;
    if ($categoryFilter !== '' && ($data['category'] ?? '') !== $categoryFilter) continue;

    $pubDate = $data['published_at'] ?? $data['publish_date'] ?? '1970-01-01';
    $rev = (int)($data['revision'] ?? 1);
    $version = ($data['version'] ?? '') . ($rev > 1 ? '-' . $rev : '');
    $name = $data['name'];
    $link = 'https://amiport.platesteel.net/packages.html?name=' . urlencode($name);

    $feedItems[] = [
        'sort'        => $pubDate,
        'title'       => trim($name . ' ' . $version),
        'link'        => $link,
        'description' => $data['description'] ?? '',
        'pubDate'     => $pubDate,
        'guid'        => $link,
    ];
}

// --- News items (suppressed when a category filter is active) ---
if ($categoryFilter === '' && file_exists($newsFile)) {
    $rawNews = json_decode(file_get_contents($newsFile), true);
    $newsItems = [];
    if (is_array($rawNews)) {
        if (isset($rawNews['items']) && is_array($rawNews['items'])) {
            $newsItems = $rawNews['items'];
        } elseif ($rawNews === [] || array_keys($rawNews) === range(0, count($rawNews) - 1)) {
            $newsItems = $rawNews;
        }
    }
    foreach ($newsItems as $entry) {
        if (!is_array($entry)) continue;
        $date = $entry['date'] ?? '';
        $title = $entry['title'] ?? '';
        if ($date === '' || $title === '') continue;
        $id = $entry['id'] ?? '';
        $link = 'https://amiport.platesteel.net/news.html' . ($id !== '' ? '#' . rawurlencode($id) : '');
        // Flatten body into a one-paragraph plain-text description; the XML
        // encoder will escape what it needs to.
        $body = is_string($entry['body'] ?? null) ? trim($entry['body']) : '';
        $description = $body !== '' ? $body : $title;

        $feedItems[] = [
            'sort'        => $date,
            'title'       => 'News: ' . $title,
            'link'        => $link,
            'description' => $description,
            'pubDate'     => $date,
            'guid'        => $link,
        ];
    }
}

// Sort newest-first and limit
usort($feedItems, function ($a, $b) {
    return strcmp($b['sort'], $a['sort']);
});
$feedItems = array_slice($feedItems, 0, 50);

echo '<?xml version="1.0" encoding="UTF-8"?>' . "\n";
?>
<rss version="2.0">
  <channel>
    <title><?php echo $categoryFilterEscaped !== '' ? 'amiport — New Ports (' . $categoryFilterEscaped . ')' : 'amiport — New Ports'; ?></title>
    <link>https://amiport.platesteel.net</link>
    <description>POSIX tools ported to AmigaOS 3.x</description>
    <language>en-us</language>
    <lastBuildDate><?php echo gmdate('D, d M Y H:i:s') . ' +0000'; ?></lastBuildDate>
<?php foreach ($feedItems as $item): ?>
    <item>
      <title><?php echo htmlspecialchars($item['title'], ENT_XML1, 'UTF-8'); ?></title>
      <link><?php echo htmlspecialchars($item['link'], ENT_XML1, 'UTF-8'); ?></link>
      <description><?php echo htmlspecialchars($item['description'], ENT_XML1, 'UTF-8'); ?></description>
<?php
    $ts = strtotime($item['pubDate']);
    if ($ts !== false):
?>
      <pubDate><?php echo gmdate('D, d M Y H:i:s', $ts) . ' +0000'; ?></pubDate>
<?php endif; ?>
      <guid isPermaLink="true"><?php echo htmlspecialchars($item['guid'], ENT_XML1, 'UTF-8'); ?></guid>
    </item>
<?php endforeach; ?>
  </channel>
</rss>
