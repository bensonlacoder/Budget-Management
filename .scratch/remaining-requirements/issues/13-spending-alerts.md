# 13: Spending alerts

**Owner:** P4

**Requirements:** FR-41, FR-42

**Files:** `UI/planpage.*`, `tests/ui/plan_page_test.cpp`

**What to build:** On the Plan tab, the user turns two alerts on or off separately: going over today's daily budget, and running out of points before the quarter ends. An alert that is on appears when its condition happens.

**Blocked by:** 03 (Quarter calendar), 06 (Plan tab and reserve goal)

**Status:** ready-for-agent

- [ ] There are two separate on/off settings, and both are saved
- [ ] When today's recorded spending goes over today's daily budget and that alert is on, the user sees one in-app alert that day
- [ ] When projected spending would use up the balance before the quarter ends and that alert is on, the user sees an alert
- [ ] An alert that is off never appears
- [ ] Unit tests cover each condition with its alert on and off
