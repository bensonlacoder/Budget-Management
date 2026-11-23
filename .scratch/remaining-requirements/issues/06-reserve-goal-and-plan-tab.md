# 06: Plan tab and reserve goal

**Owner:** P4

**Requirements:** FR-06, FR-07, FR-16, FR-37, FR-43

**Files:** `src/budget.*`, `UI/planpage.*`, `tests/budget_happy_test.cpp`, `tests/budget_edge_test.cpp`, `tests/ui/plan_page_test.cpp`

**What to build:** The Plan tab shows the real spending advice (it moves here from the Budget tab, which becomes a sandbox). The user can also set a reserve goal: points to keep at the end of the quarter, which the advice excludes. A warning appears when projected spending would leave less than the reserve.

**Blocked by:** 00 (Day-one skeleton)

**Status:** ready-for-agent

- [ ] The Plan tab shows the real balance, days left, daily advice, and weekly advice, and refreshes when the balance or days change
- [ ] The reserve goal defaults to 0 and is saved
- [ ] Advice = (balance - reserve) / days left, rounded down as it is now
- [ ] A reserve that is negative or larger than the balance is rejected with a clear message, and the previous value is kept
- [ ] The projected end-of-quarter balance uses the recent spending rate; SPECIFICATIONS.md defines that rate. A warning appears when the projection is below the reserve
- [ ] Only P4 edits `Budget`; other pages only call it
- [ ] Unit tests cover advice with a reserve, an invalid reserve, and the warning appearing and not appearing
