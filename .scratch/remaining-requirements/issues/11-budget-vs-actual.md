# 11: Actual spending vs. budget

**Owner:** P3

**Requirements:** FR-14

**Files:** `UI/chartspage.*`, `tests/ui/charts_page_test.cpp`

**What to build:** For each day and week, the Charts tab shows actual spending next to the budget for that period, and how far over or under budget the user was.

**Blocked by:** 10 (Location totals and charts), 06 (Plan tab and reserve goal; uses `Budget` without changing it)

**Status:** ready-for-agent

- [ ] Daily and weekly views show actual spending, the budget, and the difference, labelled over or under
- [ ] Charts show the budget as a reference line
- [ ] SPECIFICATIONS.md says which budget a past period is compared against
- [ ] Unit tests cover periods that are over, under, and exactly on budget
