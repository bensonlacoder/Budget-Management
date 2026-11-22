# 10: Spending by location and spending charts

**Requirements:** FR-12, FR-13

**What to build:** The user can see points spent at each dining location for a chosen date range, plus daily and weekly spending charts. All of these include purchases and subtract refunds.

**Blocked by:** 09 (Refunds)

**Status:** ready-for-agent

- [ ] For a chosen date range, a table lists each location and the net points spent there
- [ ] A daily chart shows net spending per day, and a weekly chart shows net spending per week
- [ ] Adding, editing, or removing a purchase or refund updates the totals and charts
- [ ] Empty ranges show a clear empty state instead of a blank chart
- [ ] Unit tests cover the totals calculations, including refunds and ranges that cross a week boundary
