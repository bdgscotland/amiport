<?php
/**
 * Redirect to packages.html — the canonical package browser.
 *
 * This PHP page was the original server-rendered detail view.
 * packages.html is now the single source of truth with screenshots,
 * roadmap, hardware requirements, and game support.
 */
$name = isset($_GET['name']) && preg_match('/^[a-z0-9_-]+$/', $_GET['name']) ? $_GET['name'] : '';
if ($name !== '') {
    header('Location: /packages.html?name=' . rawurlencode($name), true, 301);
} else {
    header('Location: /packages.html', true, 301);
}
exit;
