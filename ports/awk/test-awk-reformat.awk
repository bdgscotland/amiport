BEGIN { FS = ":" }
{ printf "%-10s %-6s %s\n", $1, $2, $3 }
