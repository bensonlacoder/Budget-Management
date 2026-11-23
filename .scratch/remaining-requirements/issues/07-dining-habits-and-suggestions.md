# 07: Dining habits and habit-aware suggestions

**Owner:** P4

**Requirements:** FR-38, FR-39, FR-40

**Files:** `src/budget.*`, `UI/planpage.*`, `tests/budget_happy_test.cpp`, `tests/budget_edge_test.cpp`, `tests/ui/plan_page_test.cpp`

**What to build:** On the Plan tab, the user enters which days they eat on campus and how many meals they eat on those days. Suggestions then spread the points above the reserve across the remaining on-campus meals, and refresh after any input changes.

**Blocked by:** 03 (Quarter calendar), 06 (Plan tab and reserve goal)

**Status:** ready-for-agent

- [ ] The user selects on-campus weekdays and meals per day, and these are saved
- [ ] The user can upload habits from a CSV file; the format is documented in SPECIFICATIONS.md, and a bad file shows a clear error
- [ ] Suggestions show points per meal and points per campus day, counting only the on-campus days left in the quarter
- [ ] Changing the balance, days left, reserve, or habits refreshes the suggestions immediately
- [ ] Unit tests cover suggestions with and without habits, and a week with no campus days
