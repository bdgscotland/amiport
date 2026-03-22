<?php
/**
 * Admin dashboard — password-protected via bcrypt hash in .env
 */

require_once __DIR__ . '/db.php';

session_start();

$adminHash = amiport_env('ADMIN_PASSWORD_HASH');
$pdo = amiport_db();
$error = '';
$success = '';

// --- Logout ---
if (isset($_GET['logout'])) {
    session_destroy();
    header('Location: admin.php');
    exit;
}

// --- Login ---
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['password']) && amiport_csrf_check($_POST['csrf_token'] ?? '')) {
    // Rate limiting: check login_attempts table
    $ipHash = amiport_ip_hash();
    $blocked = false;

    if ($pdo !== null) {
        try {
            $stmt = $pdo->prepare(
                'SELECT COUNT(*) FROM login_attempts
                 WHERE ip_hash = ? AND attempted_at >= DATE_SUB(NOW(), INTERVAL 1 HOUR)'
            );
            $stmt->execute([$ipHash]);
            $attempts = (int) $stmt->fetchColumn();

            if ($attempts >= 5) {
                $error = 'Too many login attempts. Try again later.';
                $blocked = true;
            }

            // Log this attempt
            $stmt = $pdo->prepare('INSERT INTO login_attempts (ip_hash) VALUES (?)');
            $stmt->execute([$ipHash]);
        } catch (PDOException $e) {
            error_log('amiport admin rate limit check failed: ' . $e->getMessage());
        }
    }

    if (!$blocked && $adminHash !== '' && password_verify($_POST['password'], $adminHash)) {
        $_SESSION['admin'] = true;
        $_SESSION['admin_time'] = time();
        header('Location: admin.php');
        exit;
    } elseif (!$blocked) {
        $error = 'Invalid password.';
    }
}

// --- Status update for port requests ---
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['request_id'], $_POST['new_status']) && isset($_SESSION['admin']) && amiport_csrf_check($_POST['csrf_token'] ?? '')) {
    $requestId = (int) $_POST['request_id'];
    $newStatus = $_POST['new_status'];
    $notes = $_POST['admin_notes'] ?? '';

    $validStatuses = ['pending', 'accepted', 'rejected', 'completed'];
    if ($pdo !== null && in_array($newStatus, $validStatuses, true)) {
        try {
            $stmt = $pdo->prepare(
                'UPDATE port_requests SET status = ?, admin_notes = ? WHERE id = ?'
            );
            $stmt->execute([$newStatus, $notes, $requestId]);
            $success = 'Request updated.';
        } catch (PDOException $e) {
            error_log('amiport admin status update failed: ' . $e->getMessage());
            $error = 'Could not update request.';
        }
    }
}

// --- Session check ---
$isAdmin = isset($_SESSION['admin']) && $_SESSION['admin'] === true;

// Session timeout (1 hour)
if ($isAdmin && isset($_SESSION['admin_time']) && (time() - $_SESSION['admin_time']) > 3600) {
    session_destroy();
    $isAdmin = false;
    $error = 'Session expired. Please log in again.';
}

?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="robots" content="noindex, nofollow">
    <title>Admin — amiport</title>
    <link rel="stylesheet" href="css/style.css">
</head>
<body>

<a href="#main" class="skip-link">Skip to content</a>

<header class="screen-bar" role="banner">
    <a href="/" class="screen-bar__brand">amiport</a>
    <nav aria-label="Main navigation">
        <ul class="screen-bar__nav">
            <li><a href="packages.html">Packages</a></li>
            <li><a href="stats.html">Stats</a></li>
            <?php if ($isAdmin): ?>
            <li><a href="admin.php?logout=1">Logout</a></li>
            <?php endif; ?>
        </ul>
    </nav>
    <div class="screen-bar__depth" aria-hidden="true"></div>
</header>

<main id="main" class="content">

<h1 class="sr-only">amiport Admin</h1>

<?php if (!$isAdmin): ?>
<!-- Login Form -->
<section class="window">
    <div class="window__title">
        <div class="window__gadget window__gadget--close" aria-hidden="true"></div>
        <span class="window__title-text" role="heading" aria-level="2">Admin Login</span>
        <div class="window__gadget window__gadget--resize" aria-hidden="true"></div>
    </div>
    <div class="window__body">
        <?php if ($error): ?>
        <div class="alert alert--error mb-md" role="alert"><?php echo htmlspecialchars($error); ?></div>
        <?php endif; ?>
        <form method="POST" action="admin.php">
            <input type="hidden" name="csrf_token" value="<?php echo htmlspecialchars(amiport_csrf_token()); ?>">
            <label for="password">Password:</label>
            <input type="password" id="password" name="password" class="wb-input mb-md" required>
            <button type="submit" class="btn btn--primary">Login</button>
        </form>
    </div>
</section>

<?php else: ?>
<!-- Dashboard -->

<?php if ($error): ?>
<div class="alert alert--error mb-md" role="alert"><?php echo htmlspecialchars($error); ?></div>
<?php endif; ?>
<?php if ($success): ?>
<div class="alert alert--success mb-md" role="alert"><?php echo htmlspecialchars($success); ?></div>
<?php endif; ?>

<?php
// Gather dashboard data
$totalDownloads = 0;
$todayDownloads = 0;
$totalVotes = 0;
$pendingRequests = 0;
$recentDownloads = [];
$portRequests = [];
$voteSummary = [];

if ($pdo !== null) {
    try {
        $totalDownloads = (int) $pdo->query('SELECT COUNT(*) FROM downloads')->fetchColumn();
        $todayDownloads = (int) $pdo->query(
            'SELECT COUNT(*) FROM downloads WHERE downloaded_at >= CURDATE()'
        )->fetchColumn();
        $totalVotes = (int) $pdo->query('SELECT COUNT(*) FROM votes')->fetchColumn();
        $pendingRequests = (int) $pdo->query(
            "SELECT COUNT(*) FROM port_requests WHERE status = 'pending'"
        )->fetchColumn();

        $recentDownloads = $pdo->query(
            'SELECT package_name, ip_hash, downloaded_at
             FROM downloads ORDER BY downloaded_at DESC LIMIT 50'
        )->fetchAll();

        $portRequests = $pdo->query(
            'SELECT id, tool_name, tool_url, status, admin_notes, requested_at
             FROM port_requests ORDER BY requested_at DESC LIMIT 50'
        )->fetchAll();

        $voteSummary = $pdo->query(
            'SELECT package_name,
                    SUM(CASE WHEN vote=1 THEN 1 ELSE 0 END) as up,
                    SUM(CASE WHEN vote=-1 THEN 1 ELSE 0 END) as down
             FROM votes GROUP BY package_name ORDER BY (SUM(vote)) DESC'
        )->fetchAll();
    } catch (PDOException $e) {
        error_log('amiport admin queries failed: ' . $e->getMessage());
    }
}
?>

<!-- Overview -->
<section class="window">
    <div class="window__title">
        <div class="window__gadget window__gadget--close" aria-hidden="true"></div>
        <span class="window__title-text" role="heading" aria-level="2">Overview</span>
        <div class="window__gadget window__gadget--resize" aria-hidden="true"></div>
    </div>
    <div class="window__body">
        <div class="stats-bar">
            <span class="stats-bar__item"><span class="stats-bar__value"><?php echo $totalDownloads; ?></span> total downloads</span>
            <span class="stats-bar__sep" aria-hidden="true">|</span>
            <span class="stats-bar__item"><span class="stats-bar__value"><?php echo $todayDownloads; ?></span> today</span>
            <span class="stats-bar__sep" aria-hidden="true">|</span>
            <span class="stats-bar__item"><span class="stats-bar__value"><?php echo $totalVotes; ?></span> votes</span>
            <span class="stats-bar__sep" aria-hidden="true">|</span>
            <span class="stats-bar__item"><span class="stats-bar__value"><?php echo $pendingRequests; ?></span> pending requests</span>
        </div>
    </div>
</section>

<!-- Recent Downloads -->
<section class="window">
    <div class="window__title">
        <div class="window__gadget window__gadget--close" aria-hidden="true"></div>
        <span class="window__title-text" role="heading" aria-level="2">Recent Downloads</span>
        <div class="window__gadget window__gadget--resize" aria-hidden="true"></div>
    </div>
    <div class="window__body">
        <?php if (empty($recentDownloads)): ?>
        <p class="text-muted">No downloads yet.</p>
        <?php else: ?>
        <div class="wb-table-wrap">
            <table class="wb-table">
                <thead><tr><th scope="col">Time</th><th scope="col">Package</th><th scope="col">IP (hash)</th></tr></thead>
                <tbody>
                <?php foreach ($recentDownloads as $dl): ?>
                <tr>
                    <td><?php echo htmlspecialchars($dl['downloaded_at']); ?></td>
                    <td><?php echo htmlspecialchars($dl['package_name']); ?></td>
                    <td><?php echo htmlspecialchars(substr($dl['ip_hash'], 0, 12)); ?>...</td>
                </tr>
                <?php endforeach; ?>
                </tbody>
            </table>
        </div>
        <?php endif; ?>
    </div>
</section>

<!-- Port Requests -->
<section class="window">
    <div class="window__title">
        <div class="window__gadget window__gadget--close" aria-hidden="true"></div>
        <span class="window__title-text" role="heading" aria-level="2">Port Requests</span>
        <div class="window__gadget window__gadget--resize" aria-hidden="true"></div>
    </div>
    <div class="window__body">
        <?php if (empty($portRequests)): ?>
        <p class="text-muted">No port requests yet. Once users start requesting tools, they'll appear here.</p>
        <?php else: ?>
        <?php foreach ($portRequests as $req): ?>
        <div class="window mb-md" style="background:var(--wb-surface)">
            <div class="window__body" style="padding:var(--sp-sm)">
                <strong><?php echo htmlspecialchars($req['tool_name']); ?></strong>
                <?php if ($req['tool_url']): ?>
                — <a href="<?php echo htmlspecialchars($req['tool_url']); ?>" target="_blank" rel="noopener"><?php echo htmlspecialchars($req['tool_url']); ?></a>
                <?php endif; ?>
                <br>
                <span class="text-muted"><?php echo htmlspecialchars($req['requested_at']); ?></span>
                — <span class="badge"><?php echo htmlspecialchars($req['status']); ?></span>
                <?php if ($req['admin_notes']): ?>
                <br><em class="text-muted"><?php echo htmlspecialchars($req['admin_notes']); ?></em>
                <?php endif; ?>
                <form method="POST" action="admin.php" style="margin-top:var(--sp-xs);display:flex;gap:var(--sp-xs);flex-wrap:wrap;align-items:center">
                    <input type="hidden" name="csrf_token" value="<?php echo htmlspecialchars(amiport_csrf_token()); ?>">
                    <input type="hidden" name="request_id" value="<?php echo (int) $req['id']; ?>">
                    <button type="submit" name="new_status" value="accepted" class="btn btn--sm btn--default">Accept</button>
                    <button type="submit" name="new_status" value="rejected" class="btn btn--sm btn--default">Reject</button>
                    <button type="submit" name="new_status" value="completed" class="btn btn--sm btn--accent">Complete</button>
                    <input type="text" name="admin_notes" placeholder="Notes..." class="wb-input" style="flex:1;min-height:28px;padding:var(--sp-xs)">
                </form>
            </div>
        </div>
        <?php endforeach; ?>
        <?php endif; ?>
    </div>
</section>

<!-- Vote Summary -->
<section class="window">
    <div class="window__title">
        <div class="window__gadget window__gadget--close" aria-hidden="true"></div>
        <span class="window__title-text" role="heading" aria-level="2">Vote Summary</span>
        <div class="window__gadget window__gadget--resize" aria-hidden="true"></div>
    </div>
    <div class="window__body">
        <?php if (empty($voteSummary)): ?>
        <p class="text-muted">No votes yet.</p>
        <?php else: ?>
        <div class="wb-table-wrap">
            <table class="wb-table">
                <thead><tr><th scope="col">Package</th><th scope="col">Up</th><th scope="col">Down</th><th scope="col">Score</th></tr></thead>
                <tbody>
                <?php foreach ($voteSummary as $v): ?>
                <tr>
                    <td><?php echo htmlspecialchars($v['package_name']); ?></td>
                    <td><?php echo (int) $v['up']; ?></td>
                    <td><?php echo (int) $v['down']; ?></td>
                    <td><?php echo ((int) $v['up'] - (int) $v['down']); ?></td>
                </tr>
                <?php endforeach; ?>
                </tbody>
            </table>
        </div>
        <?php endif; ?>
    </div>
</section>

<?php endif; ?>

</main>

<footer class="footer content" role="contentinfo">
    <span><a href="/">Home</a></span>
    <span class="text-muted">Admin Dashboard</span>
</footer>

</body>
</html>
