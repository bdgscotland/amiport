<?php
/**
 * Bug report endpoint — submit an issue for a specific package
 *
 * POST /api/v1/report-bug.php
 * Body: {"package": "sed", "description": "...", "command": "...", "setup": "...", "email": "...", "website": ""}
 *
 * Returns: {"ok": true, "ticket": 42, "message": "...", "github_url": "..."}
 *
 * Anti-spam: honeypot field + rate limit (3 reports per IP per hour)
 */

require_once dirname(__DIR__, 2) . '/db.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    amiport_json(['ok' => false, 'error' => 'Method not allowed'], 405);
}

$input = json_decode(file_get_contents('php://input'), true);
if (!is_array($input)) {
    amiport_json(['ok' => false, 'error' => 'Invalid JSON'], 400);
}

// Honeypot check: if the hidden "website" field is filled, it's a bot
$honeypot = $input['website'] ?? '';
if ($honeypot !== '') {
    // Fool the bot: return fake success
    amiport_json(['ok' => true, 'ticket' => 0, 'message' => 'Report submitted!']);
}

// Extract and sanitize fields
$package     = trim($input['package'] ?? '');
$description = trim($input['description'] ?? '');
$command     = trim($input['command'] ?? '');
$setup       = trim($input['setup'] ?? '');
$email       = trim($input['email'] ?? '');

// Validate required fields
if ($package === '') {
    amiport_json(['ok' => false, 'error' => 'Package name is required'], 400);
}
if ($description === '') {
    amiport_json(['ok' => false, 'error' => 'Please describe what happened'], 400);
}

// Validate email format if provided
if ($email !== '' && !filter_var($email, FILTER_VALIDATE_EMAIL)) {
    amiport_json(['ok' => false, 'error' => 'Invalid email address'], 400);
}

// Truncate to limits
$package     = substr($package, 0, 100);
$description = substr($description, 0, 1000);
$command     = substr($command, 0, 500);
$setup       = substr($setup, 0, 200);
$email       = substr($email, 0, 200);

// Note: no htmlspecialchars() here — PDO prepared statements handle SQL safety,
// GitHub Markdown doesn't need HTML escaping, and plain-text email is safe as-is.
// The JSON response uses json_encode() which escapes for JSON context.

$pdo = amiport_db();
if ($pdo === null) {
    amiport_json(['ok' => false, 'error' => 'Could not submit report'], 500);
}

$ipHash = amiport_ip_hash();

// Rate limit: max 3 reports per IP per hour
try {
    $stmt = $pdo->prepare(
        'SELECT COUNT(*) FROM bug_reports WHERE ip_hash = ? AND created_at > DATE_SUB(NOW(), INTERVAL 1 HOUR)'
    );
    $stmt->execute([$ipHash]);
    $recentCount = (int)$stmt->fetchColumn();
    if ($recentCount >= 3) {
        amiport_json(['ok' => false, 'error' => 'Too many reports. Please try again later.'], 429);
    }
} catch (PDOException $e) {
    // Table might not exist yet — continue and let the insert create-or-fail
    error_log('amiport rate limit check failed: ' . $e->getMessage());
}

// Insert bug report
try {
    $stmt = $pdo->prepare(
        'INSERT INTO bug_reports (package, description, command, setup, email, ip_hash) VALUES (?, ?, ?, ?, ?, ?)'
    );
    $stmt->execute([$package, $description, $command ?: null, $setup ?: null, $email ?: null, $ipHash]);
    $ticketId = (int)$pdo->lastInsertId();
} catch (PDOException $e) {
    error_log('amiport bug report insert failed: ' . $e->getMessage());
    amiport_json(['ok' => false, 'error' => 'Could not submit report'], 500);
}

// Create GitHub issue
$githubUrl = null;
$ghToken = amiport_env('GITHUB_TOKEN');
$ghRepo  = amiport_env('GITHUB_REPO', 'bdgscotland/amiport');
if ($ghToken !== '') {
    $issueTitle = "Bug report #$ticketId: $package";
    $issueBody  = "**Package:** $package\n";
    $issueBody .= "**Ticket:** #$ticketId\n\n";
    $issueBody .= "## Description\n$description\n\n";
    if ($command !== '') {
        $issueBody .= "## Command\n```\n$command\n```\n\n";
    }
    if ($setup !== '') {
        $issueBody .= "## Amiga Setup\n$setup\n\n";
    }
    // Note: email is deliberately excluded from the public GitHub issue.
    // It's stored in the database and included in the private email notification only.
    $issueBody .= "---\n*Submitted via amiport.platesteel.net*";

    $ch = curl_init("https://api.github.com/repos/$ghRepo/issues");
    curl_setopt_array($ch, [
        CURLOPT_POST => true,
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_HTTPHEADER => [
            'Authorization: Bearer ' . $ghToken,
            'Accept: application/vnd.github+json',
            'Content-Type: application/json',
            'User-Agent: amiport-site',
            'X-GitHub-Api-Version: 2022-11-28',
        ],
        CURLOPT_POSTFIELDS => json_encode([
            'title'  => $issueTitle,
            'body'   => $issueBody,
            'labels' => ['bug-report', $package],
        ]),
        CURLOPT_TIMEOUT => 10,
    ]);
    $ghResponse = curl_exec($ch);
    $ghStatus   = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);

    if ($ghStatus === 201 && $ghResponse) {
        $ghData = json_decode($ghResponse, true);
        $githubUrl = $ghData['html_url'] ?? null;

        // Store the GitHub issue URL back in the database
        if ($githubUrl !== null) {
            try {
                $stmt = $pdo->prepare('UPDATE bug_reports SET github_url = ? WHERE id = ?');
                $stmt->execute([$githubUrl, $ticketId]);
            } catch (PDOException $e) {
                // Non-fatal — report was already saved
                error_log('amiport github_url update failed: ' . $e->getMessage());
            }
        }
    } else {
        error_log("amiport GitHub issue creation failed: status=$ghStatus response=$ghResponse");
    }
}

// Send email notification
$notifyEmail = amiport_env('NOTIFY_EMAIL');
if ($notifyEmail !== '') {
    $subject = "[amiport] Bug report #$ticketId: $package";
    $body  = "Bug report #$ticketId for package: $package\n\n";
    $body .= "Description:\n$description\n\n";
    if ($command !== '') $body .= "Command: $command\n\n";
    if ($setup !== '')   $body .= "Setup: $setup\n\n";
    if ($email !== '')   $body .= "Reporter email: $email\n\n";
    if ($githubUrl)      $body .= "GitHub issue: $githubUrl\n\n";
    $body .= "IP hash: $ipHash\n";
    $body .= "Time: " . date('Y-m-d H:i:s T') . "\n";

    $headers = "From: noreply@amiport.platesteel.net\r\n"
             . "Reply-To: " . ($email !== '' ? $email : 'noreply@amiport.platesteel.net') . "\r\n"
             . "X-Mailer: amiport/1.0";

    @mail($notifyEmail, $subject, $body, $headers);
}

// Build response
$response = [
    'ok'      => true,
    'ticket'  => $ticketId,
    'message' => "Report #$ticketId submitted. Thank you!",
];
if ($githubUrl !== null) {
    $response['github_url'] = $githubUrl;
}

amiport_json($response);
