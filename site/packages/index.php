<?php
/**
 * Per-port detail page — Homebrew-style package case study
 *
 * URL: packages/?name=grep
 * Data source: flat-file JSON from data/packages/{name}.json
 * No MySQL queries for page content.
 */

require_once dirname(__DIR__) . '/db.php';

// Build an allowlist of known package names from the data directory.
// This avoids any user input in file paths — we only open files we enumerated.
$dataDir = dirname(__DIR__) . '/data/packages';
$allowedPackages = [];
$allJsonFiles = glob($dataDir . '/*.json');
if (is_array($allJsonFiles)) {
    foreach ($allJsonFiles as $f) {
        $base = basename($f, '.json');
        $allowedPackages[$base] = $f;
    }
}

// Validate: the requested name must exist in our allowlist
$requestedName = isset($_GET['name']) && is_string($_GET['name']) ? $_GET['name'] : '';
if ($requestedName === '' || !isset($allowedPackages[$requestedName])) {
    http_response_code(404);
    echo '<!DOCTYPE html><html><head><title>Not Found</title></head><body><h1>Package not found</h1><p><a href="/packages.html">Browse all packages</a></p></body></html>';
    exit;
}

// $name is now a key from our allowlist — guaranteed to be a valid filename on disk.
// $safeName is the HTML-escaped version for template output.
$name = $requestedName;
$safeName = htmlspecialchars($name, ENT_QUOTES, 'UTF-8');

// Read from the allowlisted path (no user input in the path)
$pkg = json_decode(file_get_contents($allowedPackages[$name]), true);
if (!is_array($pkg)) {
    http_response_code(404);
    echo '<!DOCTYPE html><html><head><title>Not Found</title></head><body><h1>Package not found</h1><p><a href="/packages.html">Browse all packages</a></p></body></html>';
    exit;
}

// All data below comes from trusted JSON files on disk (not user input).
// We still escape everything for defense-in-depth.

// Augment with vote data from MySQL (best-effort)
$voteScore = 0;
$votesUp = 0;
$votesDown = 0;
$downloads = 0;
$pdo = amiport_db();
if ($pdo !== null) {
    try {
        $stmt = $pdo->prepare(
            'SELECT
                SUM(CASE WHEN vote = 1 THEN 1 ELSE 0 END) as votes_up,
                SUM(CASE WHEN vote = -1 THEN 1 ELSE 0 END) as votes_down
             FROM votes WHERE package_name = ?'
        );
        $stmt->execute([$name]);
        $row = $stmt->fetch();
        $votesUp = (int) ($row['votes_up'] ?? 0);
        $votesDown = (int) ($row['votes_down'] ?? 0);
        $voteScore = $votesUp - $votesDown;

        $stmt = $pdo->prepare('SELECT COUNT(*) FROM downloads WHERE package_name = ?');
        $stmt->execute([$name]);
        $downloads = (int) $stmt->fetchColumn();
    } catch (PDOException $e) {
        error_log('amiport per-port page DB error: ' . $e->getMessage());
    }
}

// Find related ports (same category, max 3, exclude self)
$related = [];
$category = $pkg['category'] ?? '';
if ($category !== '') {
    $allFiles = glob($dataDir . '/*.json');
    if (is_array($allFiles)) {
        foreach ($allFiles as $f) {
            $other = json_decode(file_get_contents($f), true);
            if (is_array($other) && ($other['name'] ?? '') !== $name && ($other['category'] ?? '') === $category) {
                $related[] = $other;
                if (count($related) >= 3) break;
            }
        }
    }
}

// Extract values from trusted JSON, cast to safe types
$pkgVersion = (string) ($pkg['version'] ?? '');
$pkgDesc = (string) ($pkg['description'] ?? '');
$pkgSource = (string) ($pkg['source'] ?? '');
$pkgLicense = (string) ($pkg['license'] ?? '');
$pkgStatus = (string) ($pkg['status'] ?? '');
$pkgCategory = (string) ($pkg['category'] ?? 'unknown');
$pkgAminet = (string) ($pkg['aminet'] ?? '');
$pkgSha256 = (string) ($pkg['sha256'] ?? '');
$pkgDownload = (string) ($pkg['download'] ?? '');
$pkgPortingNotes = (string) ($pkg['porting_notes'] ?? '');
$pkgLimitations = (string) ($pkg['known_limitations'] ?? '');
$pkgRequires = is_array($pkg['requires'] ?? null) ? $pkg['requires'] : [];
$pkgSize = (int) ($pkg['size'] ?? 0);
$pkgStack = (int) ($pkg['stack'] ?? 0);
$testCount = (int) ($pkg['test_count'] ?? 0);
$testPass = (int) ($pkg['test_pass'] ?? 0);
$testPct = $testCount > 0 ? round(($testPass / $testCount) * 100) : 0;
$stackKB = $pkgStack > 0 ? round($pkgStack / 1024) : 0;
$publishedAt = (string) ($pkg['published_at'] ?? '');
$publishDate = $publishedAt !== '' ? date('j M Y', strtotime($publishedAt)) : '';

// Format file size
function formatSize($bytes) {
    if ($bytes < 1024) return $bytes . ' B';
    if ($bytes < 1048576) return round($bytes / 1024, 1) . ' KB';
    return round($bytes / 1048576, 1) . ' MB';
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="description" content="<?php echo htmlspecialchars($name, ENT_QUOTES, 'UTF-8'); ?> <?php echo htmlspecialchars($pkgVersion, ENT_QUOTES, 'UTF-8'); ?> for AmigaOS 3.x — <?php echo htmlspecialchars($pkgDesc, ENT_QUOTES, 'UTF-8'); ?>">
    <title><?php echo htmlspecialchars($name, ENT_QUOTES, 'UTF-8'); ?> <?php echo htmlspecialchars($pkgVersion, ENT_QUOTES, 'UTF-8'); ?> — amiport</title>
    <link rel="stylesheet" href="../css/style.css?v=20260325c">
    <link rel="icon" type="image/svg+xml" href="../img/favicon.svg">
    <link rel="alternate" type="application/rss+xml" title="amiport — New Ports" href="/feed.php">
</head>
<body>

<!-- Skip to Content -->
<a href="#main" class="skip-link">Skip to content</a>

<!-- Screen Bar -->
<header class="screen-bar" role="banner">
    <a href="/" class="screen-bar__brand" aria-label="amiport home"><svg width="170" height="30" viewBox="0 0 240 44" aria-hidden="true"><rect x="4" y="6" width="32" height="32" fill="#8B6914"/><circle cx="12" cy="14" r="2.5" fill="#DAA520"/><rect x="10" y="12" width="20" height="20" fill="none" stroke="#DAA520" stroke-width=".75"/><rect x="0" y="11" width="6" height="2.5" fill="#DAA520"/><rect x="0" y="18" width="6" height="2.5" fill="#DAA520"/><rect x="0" y="25" width="6" height="2.5" fill="#DAA520"/><rect x="0" y="32" width="6" height="2.5" fill="#DAA520"/><rect x="34" y="11" width="6" height="2.5" fill="#DAA520"/><rect x="34" y="18" width="6" height="2.5" fill="#DAA520"/><rect x="34" y="25" width="6" height="2.5" fill="#DAA520"/><rect x="34" y="32" width="6" height="2.5" fill="#DAA520"/><rect x="11" y="2" width="2.5" height="6" fill="#DAA520"/><rect x="18" y="2" width="2.5" height="6" fill="#DAA520"/><rect x="25" y="2" width="2.5" height="6" fill="#DAA520"/><rect x="11" y="36" width="2.5" height="6" fill="#DAA520"/><rect x="18" y="36" width="2.5" height="6" fill="#DAA520"/><rect x="25" y="36" width="2.5" height="6" fill="#DAA520"/><text x="48" y="31" font-family="-apple-system,BlinkMacSystemFont,sans-serif" font-size="26" font-weight="700" fill="#E0D0B8">ami<tspan fill="#DAA520">port</tspan></text></svg></a>
    <nav aria-label="Main navigation">
        <ul class="screen-bar__nav">
            <li><a href="/packages.html">Packages</a></li>
            <li><a href="/catalog.html">Catalog</a></li>
            <li><a href="/stats.html">Stats</a></li>
            <li><a href="/changelog.html">Changelog</a></li>
            <li><a href="/shims.html">Shims</a></li>
            <li><a href="/amiga.html">Amiga</a></li>
            <li><a href="https://github.com/bdgscotland/amiport" target="_blank" rel="noopener">GitHub</a></li>
        </ul>
    </nav>
</header>

<!-- Main Content -->
<main id="main" class="content">

    <h1 class="sr-only"><?php echo htmlspecialchars($name, ENT_QUOTES, 'UTF-8'); ?> <?php echo htmlspecialchars($pkgVersion, ENT_QUOTES, 'UTF-8'); ?></h1>

    <!-- Breadcrumb -->
    <nav class="breadcrumb" aria-label="Breadcrumb">
        <a href="/packages.html">Packages</a> <span class="breadcrumb__sep" aria-hidden="true">&gt;</span> <span class="breadcrumb__current"><?php echo htmlspecialchars($name, ENT_QUOTES, 'UTF-8'); ?></span>
    </nav>

    <!-- Package Detail -->
    <section class="group-frame" aria-labelledby="pkg-heading">
        <div class="group-frame__title" id="pkg-heading" role="heading" aria-level="2">Package Detail</div>
        <div class="group-frame__body">
            <div class="port-header">
                <div class="port-header__info">
                    <span class="port-header__name"><?php echo htmlspecialchars($name, ENT_QUOTES, 'UTF-8'); ?></span>
                    <span class="port-header__version"><?php echo htmlspecialchars($pkgVersion, ENT_QUOTES, 'UTF-8'); ?></span>
                    <div class="port-header__desc"><?php echo htmlspecialchars($pkgDesc, ENT_QUOTES, 'UTF-8'); ?></div>
                    <div class="port-header__badges">
                        <span class="badge badge--published"><?php echo htmlspecialchars($pkgCategory, ENT_QUOTES, 'UTF-8'); ?></span>
                        <?php if ($pkgStatus === 'stable'): ?>
                        <span class="badge badge--published">Published</span>
                        <?php endif; ?>
                        <?php if ($voteScore > 0): ?>
                        <span class="badge badge--popular"><?php echo (int)$voteScore; ?> vote<?php echo $voteScore !== 1 ? 's' : ''; ?></span>
                        <?php endif; ?>
                    </div>
                </div>
                <div class="port-header__actions">
                    <?php if ($pkgDownload !== ''): ?>
                    <a href="/api/v1/download.php?name=<?php echo urlencode($safeName); ?>" class="btn btn--primary">Download .lha</a>
                    <?php endif; ?>
                </div>
            </div>
            <?php if ($pkgDownload !== ''): ?>
            <div class="install-block mt-md">
                <span class="install-block__prompt">1.SYS:&gt;</span>
                <span class="install-block__cmd">lha x <?php echo htmlspecialchars($name, ENT_QUOTES, 'UTF-8'); ?>-<?php echo htmlspecialchars($pkgVersion, ENT_QUOTES, 'UTF-8'); ?>.lha SYS:</span>
            </div>
            <?php endif; ?>
            <?php if ($publishDate !== ''): ?>
            <p class="text-muted mt-sm">Published <?php echo htmlspecialchars($publishDate, ENT_QUOTES, 'UTF-8'); ?><?php if ($downloads > 0): ?> &middot; <?php echo number_format($downloads); ?> download<?php echo $downloads !== 1 ? 's' : ''; ?><?php endif; ?></p>
            <?php endif; ?>
        </div>
    </section>

    <!-- Porting Story -->
    <section class="group-frame" aria-labelledby="story-heading">
        <div class="group-frame__title" id="story-heading" role="heading" aria-level="2">Porting Story</div>
        <div class="group-frame__body">
            <?php if ($pkgPortingNotes !== ''): ?>
            <p><?php echo nl2br(htmlspecialchars($pkgPortingNotes, ENT_QUOTES, 'UTF-8')); ?></p>
            <?php else: ?>
            <p class="text-muted">Porting story coming soon. This port was built with the <a href="https://github.com/bdgscotland/amiport">amiport AI pipeline</a>.</p>
            <?php endif; ?>
        </div>
    </section>

    <!-- Test Results -->
    <?php if ($testCount > 0): ?>
    <section class="group-frame" aria-labelledby="test-heading">
        <div class="group-frame__title" id="test-heading" role="heading" aria-level="2">Test Results</div>
        <div class="group-frame__body">
            <div class="port-test">
                <div class="gauge" role="img" aria-label="<?php echo (int)$testPass; ?> of <?php echo (int)$testCount; ?> tests passing (<?php echo (int)$testPct; ?>%)">
                    <div class="gauge__fill" style="width: <?php echo (int)$testPct; ?>%"></div>
                    <span class="gauge__text"><?php echo (int)$testPct; ?>%</span>
                </div>
                <span class="port-test__label"><?php echo (int)$testPass; ?>/<?php echo (int)$testCount; ?> tests passing</span>
            </div>
        </div>
    </section>
    <?php endif; ?>

    <!-- Known Limitations -->
    <?php if ($pkgLimitations !== ''): ?>
    <section class="group-frame" aria-labelledby="limits-heading">
        <div class="group-frame__title" id="limits-heading" role="heading" aria-level="2">Known Limitations</div>
        <div class="group-frame__body">
            <p><?php echo nl2br(htmlspecialchars($pkgLimitations, ENT_QUOTES, 'UTF-8')); ?></p>
        </div>
    </section>
    <?php endif; ?>

    <!-- Technical Details -->
    <section class="group-frame" aria-labelledby="tech-heading">
        <div class="group-frame__title" id="tech-heading" role="heading" aria-level="2">Technical Details</div>
        <div class="group-frame__body">
            <table class="port-details-table">
                <?php if ($pkgSource !== ''): ?>
                <tr><td class="port-details-table__label">Source</td><td><?php echo htmlspecialchars($pkgSource, ENT_QUOTES, 'UTF-8'); ?></td></tr>
                <?php endif; ?>
                <?php if ($pkgLicense !== ''): ?>
                <tr><td class="port-details-table__label">License</td><td><?php echo htmlspecialchars($pkgLicense, ENT_QUOTES, 'UTF-8'); ?></td></tr>
                <?php endif; ?>
                <?php if ($pkgSize > 0): ?>
                <tr><td class="port-details-table__label">Binary size</td><td><?php echo htmlspecialchars(formatSize($pkgSize), ENT_QUOTES, 'UTF-8'); ?></td></tr>
                <?php endif; ?>
                <?php if ($stackKB > 0): ?>
                <tr><td class="port-details-table__label">Stack</td><td><?php echo (int)$stackKB; ?> KB</td></tr>
                <?php endif; ?>
                <?php if (!empty($pkgRequires)): ?>
                <tr><td class="port-details-table__label">Requires</td><td><?php echo htmlspecialchars(implode(', ', $pkgRequires), ENT_QUOTES, 'UTF-8'); ?></td></tr>
                <?php endif; ?>
                <?php if ($pkgSha256 !== ''): ?>
                <tr><td class="port-details-table__label">SHA256</td><td><code class="text-mono"><?php echo htmlspecialchars(substr($pkgSha256, 0, 16), ENT_QUOTES, 'UTF-8'); ?>...</code></td></tr>
                <?php endif; ?>
                <?php if ($pkgAminet !== ''): ?>
                <tr><td class="port-details-table__label">Aminet</td><td><a href="<?php echo htmlspecialchars($pkgAminet, ENT_QUOTES, 'UTF-8'); ?>" target="_blank" rel="noopener"><?php echo htmlspecialchars($pkgAminet, ENT_QUOTES, 'UTF-8'); ?></a></td></tr>
                <?php endif; ?>
            </table>
        </div>
    </section>

    <!-- Community -->
    <section class="group-frame" aria-labelledby="community-heading">
        <div class="group-frame__title" id="community-heading" role="heading" aria-level="2">Community</div>
        <div class="group-frame__body">
            <div class="port-community">
                <div class="port-community__votes">
                    <button class="btn btn--ghost port-vote-btn" data-package="<?php echo htmlspecialchars($name, ENT_QUOTES, 'UTF-8'); ?>" data-vote="1" aria-label="Upvote <?php echo htmlspecialchars($name, ENT_QUOTES, 'UTF-8'); ?>">&#9650;</button>
                    <span class="port-community__score" id="vote-score"><?php echo (int)$voteScore; ?></span>
                    <button class="btn btn--ghost port-vote-btn" data-package="<?php echo htmlspecialchars($name, ENT_QUOTES, 'UTF-8'); ?>" data-vote="-1" aria-label="Downvote <?php echo htmlspecialchars($name, ENT_QUOTES, 'UTF-8'); ?>">&#9660;</button>
                </div>
                <a href="https://github.com/bdgscotland/amiport/issues/new?labels=bug&amp;title=<?php echo urlencode('Bug in ' . $name); ?>" class="btn btn--ghost" target="_blank" rel="noopener">Report a Bug</a>
            </div>
        </div>
    </section>

    <!-- Related Ports -->
    <?php if (!empty($related)): ?>
    <section class="group-frame" aria-labelledby="related-heading">
        <div class="group-frame__title" id="related-heading" role="heading" aria-level="2">Related Ports</div>
        <div class="group-frame__body">
            <ul class="featured-list">
                <?php foreach ($related as $rel): ?>
                <li class="featured-list__item">
                    <a href="/packages/?name=<?php echo htmlspecialchars((string)($rel['name'] ?? ''), ENT_QUOTES, 'UTF-8'); ?>" class="featured-list__name"><?php echo htmlspecialchars((string)($rel['name'] ?? ''), ENT_QUOTES, 'UTF-8'); ?></a>
                    <span class="featured-list__ver"><?php echo htmlspecialchars((string)($rel['version'] ?? ''), ENT_QUOTES, 'UTF-8'); ?></span>
                    <span class="featured-list__desc">- <?php echo htmlspecialchars((string)($rel['description'] ?? ''), ENT_QUOTES, 'UTF-8'); ?></span>
                </li>
                <?php endforeach; ?>
            </ul>
        </div>
    </section>
    <?php endif; ?>

</main>

<!-- Footer -->
<footer class="footer content" role="contentinfo">
    <span>
        <a href="https://github.com/bdgscotland/amiport" target="_blank" rel="noopener">GitHub</a>
        &middot;
        <a href="https://aminet.net/" target="_blank" rel="noopener">Aminet</a>
        &middot;
        <a href="/feed.php">RSS</a>
    </span>
    <span>
        By <strong>Duncan Bowring</strong> &middot; Built with <a href="https://claude.ai/claude-code" target="_blank" rel="noopener">Claude Code</a>.
    </span>
</footer>

<script>
// Vote handling for per-port page
(function() {
    var btns = document.querySelectorAll('.port-vote-btn');
    var scoreEl = document.getElementById('vote-score');
    btns.forEach(function(btn) {
        btn.addEventListener('click', function() {
            var pkg = btn.getAttribute('data-package');
            var vote = parseInt(btn.getAttribute('data-vote'), 10);
            btns.forEach(function(b) { b.disabled = true; });

            var xhr = new XMLHttpRequest();
            xhr.open('POST', '/api/v1/vote.php', true);
            xhr.setRequestHeader('Content-Type', 'application/json');
            xhr.onload = function() {
                btns.forEach(function(b) { b.disabled = false; });
                try {
                    var resp = JSON.parse(xhr.responseText);
                    if (resp.ok && scoreEl) {
                        scoreEl.textContent = resp.score;
                    }
                } catch (ex) {}
            };
            xhr.onerror = function() {
                btns.forEach(function(b) { b.disabled = false; });
            };
            xhr.send(JSON.stringify({package: pkg, vote: vote}));
        });
    });
})();
</script>

</body>
</html>
