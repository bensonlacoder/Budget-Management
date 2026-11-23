# 10: Location totals and spending charts

**Owner:** P3

**Requirements:** FR-12, FR-13, FR-47 (unlogged spending)

**Files:** `UI/chartspage.*`, `tests/ui/charts_page_test.cpp`

**What to build:** The Charts tab shows net points spent per location for a chosen date range, plus daily and weekly spending charts. Refunds are subtracted, and balance corrections that lowered the balance count as "Unlogged".

**Blocked by:** 09 (Refunds), 02 (Profile page, for corrections)

**Status:** ready-for-agent

- [ ] For a chosen date range, a table lists each location with its net points, including an "Unlogged" row for corrections that lowered the balance
- [ ] Corrections that raised the balance are not counted as spending
- [ ] Daily and weekly charts show net spending. Draw them with QPainter, or add Qt Charts to CI if you use it, since CI only installs Qt base
- [ ] Charts and totals update after any purchase, refund, or correction
- [ ] An empty range shows a clear message instead of a blank chart
- [ ] Unit tests cover the totals, including refunds, corrections, and a range that crosses a week boundary
