#!/bin/bash

BINARY=./city_manager
DISTRICT=test_script_district
PASS=0
FAIL=0

# ── helpers ──────────────────────────────────────────────────────────────────

assert_contains() {
    local desc="$1" output="$2" expected="$3"
    if echo "$output" | grep -q "$expected"; then
        echo "  PASS: $desc"
        ((PASS++))
    else
        echo "  FAIL: $desc"
        echo "        expected : '$expected'"
        echo "        got      : $(echo "$output" | tail -5)"
        ((FAIL++))
    fi
}

assert_not_contains() {
    local desc="$1" output="$2" unexpected="$3"
    if ! echo "$output" | grep -q "$unexpected"; then
        echo "  PASS: $desc"
        ((PASS++))
    else
        echo "  FAIL: $desc"
        echo "        did not expect: '$unexpected'"
        ((FAIL++))
    fi
}

assert_file_perm() {
    local desc="$1" file="$2" expected_octal="$3"
    actual=$(stat -f "%Lp" "$file" 2>/dev/null)
    if [ "$actual" = "$expected_octal" ]; then
        echo "  PASS: $desc ($actual)"
        ((PASS++))
    else
        echo "  FAIL: $desc (expected $expected_octal, got $actual)"
        ((FAIL++))
    fi
}

cleanup() {
    rm -rf "$DISTRICT" "active-reports-$DISTRICT"
}

add_report() {
    local user="$1" cat="$2" sev="$3" desc="$4"
    printf "10.0\n20.0\n%s\n%s\n%s\n" "$cat" "$sev" "$desc" \
        | $BINARY --role inspector --user "$user" --add "$DISTRICT" 2>/dev/null
}

section() { echo; echo "══ $1 ══"; }

# ── build ─────────────────────────────────────────────────────────────────────

section "BUILD"
gcc -o city_manager city_manager.c
if [ $? -eq 0 ]; then
    echo "  PASS: compilation"
    ((PASS++))
else
    echo "  FAIL: compilation — aborting"
    exit 1
fi

# ── setup ─────────────────────────────────────────────────────────────────────

cleanup
add_report Alice   road      2 "Pothole on main street"
add_report Alice   flooding  3 "Severe flooding near bridge"
add_report Bob     lightning 1 "Street light out"
add_report Bob     road      3 "Large sinkhole blocking road"
add_report Carol   other     2 "Abandoned vehicle on sidewalk"

# ── ADD ───────────────────────────────────────────────────────────────────────

section "ADD"
out=$($BINARY --role inspector --user Alice --list "$DISTRICT" 2>/dev/null)
assert_contains     "5 reports present after seeding"        "$out" "ID: 5"
assert_contains     "IDs are sequential (ID 1 exists)"       "$out" "ID: 1"
assert_contains     "inspector name stored correctly"         "$out" "Inspector: Alice"
assert_contains     "category stored correctly"               "$out" "flooding"
assert_not_contains "no permission warnings on reports.dat"  "$out" "Warning"

# ── LIST ──────────────────────────────────────────────────────────────────────

section "LIST"
out=$($BINARY --role inspector --user Alice --list "$DISTRICT" 2>/dev/null)
assert_contains "all 5 reports listed"         "$out" "ID: 5"
assert_contains "road category appears"        "$out" "road"
assert_contains "lightning category appears"   "$out" "lightning"
assert_contains "other category appears"       "$out" "other"

# ── VIEW ──────────────────────────────────────────────────────────────────────

section "VIEW"
out=$($BINARY --role inspector --user Alice --view "$DISTRICT" 2 2>/dev/null)
assert_contains "view report 2 — correct ID"        "$out" "ID: 2"
assert_contains "view report 2 — correct inspector" "$out" "Inspector: Alice"
assert_contains "view report 2 — correct category"  "$out" "flooding"
assert_contains "view report 2 — full description"  "$out" "Severe flooding near bridge"

out=$($BINARY --role inspector --user Alice --view "$DISTRICT" 99 2>/dev/null)
assert_contains "view non-existent ID prints not-found message" "$out" "not found"

# ── FILTER ────────────────────────────────────────────────────────────────────

section "FILTER — single conditions"

out=$($BINARY --role inspector --user Alice --filter "$DISTRICT" "severity:>=:2" 2>/dev/null)
assert_contains     "severity>=2 returns ID 1" "$out" "ID: 1"
assert_contains     "severity>=2 returns ID 2" "$out" "ID: 2"
assert_not_contains "severity>=2 excludes ID 3 (severity 1)" "$out" "ID: 3"
assert_contains     "severity>=2 returns ID 4" "$out" "ID: 4"
assert_contains     "severity>=2 returns ID 5" "$out" "ID: 5"

out=$($BINARY --role inspector --user Alice --filter "$DISTRICT" "category:==:road" 2>/dev/null)
assert_contains     "category==road returns ID 1" "$out" "ID: 1"
assert_contains     "category==road returns ID 4" "$out" "ID: 4"
assert_not_contains "category==road excludes ID 2 (flooding)" "$out" "ID: 2"
assert_not_contains "category==road excludes ID 3 (lightning)" "$out" "ID: 3"

out=$($BINARY --role inspector --user Alice --filter "$DISTRICT" "inspector:==:Bob" 2>/dev/null)
assert_contains     "inspector==Bob returns ID 3" "$out" "ID: 3"
assert_contains     "inspector==Bob returns ID 4" "$out" "ID: 4"
assert_not_contains "inspector==Bob excludes ID 1 (Alice)" "$out" "ID: 1"

out=$($BINARY --role inspector --user Alice --filter "$DISTRICT" "severity:==:1" 2>/dev/null)
assert_contains     "severity==1 returns only ID 3" "$out" "ID: 3"
assert_not_contains "severity==1 excludes ID 1"     "$out" "ID: 1"

out=$($BINARY --role inspector --user Alice --filter "$DISTRICT" "category:!=:road" 2>/dev/null)
assert_contains     "category!=road returns ID 2 (flooding)"   "$out" "ID: 2"
assert_contains     "category!=road returns ID 3 (lightning)"  "$out" "ID: 3"
assert_not_contains "category!=road excludes ID 1 (road)"      "$out" "ID: 1"
assert_not_contains "category!=road excludes ID 4 (road)"      "$out" "ID: 4"

section "FILTER — multiple conditions (AND)"

out=$($BINARY --role inspector --user Alice --filter "$DISTRICT" "severity:==:3" "category:==:road" 2>/dev/null)
assert_contains     "sev==3 AND cat==road returns ID 4 only" "$out" "ID: 4"
assert_not_contains "sev==3 AND cat==road excludes ID 2 (flooding sev3)" "$out" "ID: 2"
assert_not_contains "sev==3 AND cat==road excludes ID 1 (road sev2)"     "$out" "ID: 1"

out=$($BINARY --role inspector --user Alice --filter "$DISTRICT" "severity:!=:3" "inspector:!=:Bob" 2>/dev/null)
assert_contains     "sev!=3 AND ins!=Bob returns ID 1" "$out" "ID: 1"
assert_contains     "sev!=3 AND ins!=Bob returns ID 5" "$out" "ID: 5"
assert_not_contains "sev!=3 AND ins!=Bob excludes ID 2 (sev3)" "$out" "ID: 2"
assert_not_contains "sev!=3 AND ins!=Bob excludes ID 3 (Bob)"  "$out" "ID: 3"
assert_not_contains "sev!=3 AND ins!=Bob excludes ID 4 (Bob)"  "$out" "ID: 4"

section "FILTER — edge cases"

out=$($BINARY --role inspector --user Alice --filter "$DISTRICT" "severity:>:3" 2>/dev/null)
assert_contains "no match returns appropriate message" "$out" "No reports match"

out=$($BINARY --role inspector --user Alice --filter "$DISTRICT" 2>/dev/null)
assert_contains "no conditions returns all 5 reports" "$out" "ID: 5"

# ── REMOVE REPORT ─────────────────────────────────────────────────────────────

section "REMOVE REPORT"

out=$($BINARY --role manager --user Bob --remove_report "$DISTRICT" 3 2>/dev/null)
assert_contains "manager can remove a report" "$out" "deleted"

out=$($BINARY --role inspector --user Alice --list "$DISTRICT" 2>/dev/null)
assert_not_contains "removed report no longer in list" "$out" "ID: 3"
assert_contains     "other reports still present after remove" "$out" "ID: 4"

out=$($BINARY --role inspector --user Alice --remove_report "$DISTRICT" 1 2>/dev/null)
assert_contains "non-manager cannot remove a report" "$out" "Only managers"

# ── UPDATE_THRESHOLD ──────────────────────────────────────────────────────────

section "UPDATE_THRESHOLD"

out=$($BINARY --role manager --user Bob --update_threshold "$DISTRICT" 5 2>/dev/null)
assert_not_contains "manager can update threshold (no error)" "$out" "Warning"
assert_not_contains "manager can update threshold (no error)" "$out" "Failed"

cfg="$DISTRICT/district.cfg"
assert_contains "threshold written to config file" "$(cat "$cfg")" "escalation_threshold=5"

out=$($BINARY --role inspector --user Alice --update_threshold "$DISTRICT" 9 2>/dev/null)
assert_contains "non-manager cannot update threshold" "$out" "Only managers"

# ── LOG ───────────────────────────────────────────────────────────────────────

section "LOG"

log="$DISTRICT/logged_district"
assert_contains "ADD operations are logged"              "$(cat "$log")" "Operation: ADD"
assert_contains "LIST operations are logged"             "$(cat "$log")" "Operation: LIST"
assert_contains "VIEW operations are logged"             "$(cat "$log")" "Operation: VIEW"
assert_contains "FILTER operations are logged"           "$(cat "$log")" "Operation: FILTER"
assert_contains "REMOVE operations are logged"           "$(cat "$log")" "Operation: REMOVE"
assert_contains "UPDATE_THRESHOLD operations are logged" "$(cat "$log")" "Operation: UPDATE_THRESHOLD"
assert_contains "role is logged"                         "$(cat "$log")" "Role: inspector"
assert_contains "inspector name is logged"               "$(cat "$log")" "Inspector: Alice"
assert_contains "timestamp is logged"                    "$(cat "$log")" "Timestamp:"

# ── FILE PERMISSIONS ──────────────────────────────────────────────────────────

section "FILE PERMISSIONS"

assert_file_perm "reports.dat is 0664"       "$DISTRICT/reports.dat"       "664"
assert_file_perm "district.cfg is 0640"      "$DISTRICT/district.cfg"      "640"
assert_file_perm "logged_district is 0644"   "$DISTRICT/logged_district"   "644"

# ── SYMLINK ───────────────────────────────────────────────────────────────────

section "SYMLINK"

link="active-reports-$DISTRICT"
if [ -L "$link" ]; then
    echo "  PASS: symlink $link exists"
    ((PASS++))
else
    echo "  FAIL: symlink $link does not exist"
    ((FAIL++))
fi

target=$(readlink "$link")
assert_contains "symlink points to reports.dat" "$target" "reports.dat"

# ── REMOVE DISTRICT ───────────────────────────────────────────────────────────

section "REMOVE DISTRICT"

out=$($BINARY --role inspector --user Alice --remove_district "$DISTRICT" 2>/dev/null)
assert_contains "non-manager cannot remove district" "$out" "Only managers"
[ -d "$DISTRICT" ] && echo "  PASS: district still exists after failed remove" && ((PASS++)) \
                   || { echo "  FAIL: district was removed by non-manager"; ((FAIL++)); }

out=$($BINARY --role manager --user Bob --remove_district "$DISTRICT" 2>/dev/null)
assert_contains "manager can remove district" "$out" "removed successfully"
[ ! -d "$DISTRICT" ] && echo "  PASS: district directory is gone" && ((PASS++)) \
                     || { echo "  FAIL: district directory still exists"; ((FAIL++)); }
[ ! -L "$link" ] && echo "  PASS: symlink is gone after district removal" && ((PASS++)) \
                 || { echo "  FAIL: symlink still exists after district removal"; ((FAIL++)); }

# ── summary ───────────────────────────────────────────────────────────────────

echo
echo "══════════════════════════════"
echo "  Results: $PASS passed, $FAIL failed"
echo "══════════════════════════════"

[ $FAIL -eq 0 ] && exit 0 || exit 1
