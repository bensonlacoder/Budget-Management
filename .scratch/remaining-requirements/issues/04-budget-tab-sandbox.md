# 04: Budget tab sandbox

**Owner:** P2

**Requirements:** FR-36

**Files:** `UI/mainui.*`, `UI/mainui.ui`, `tests/ui/budget_page_test.cpp`, `tests/ui/remaining_days_test.cpp`

**What to build:** The Budget tab becomes a place to try out numbers. It starts from the user's real balance and days left, but nothing done there is saved or seen by friends.

**Blocked by:** 02 (Profile page; the real corrections must exist before they leave this tab), 03 (Quarter calendar)

**Status:** ready-for-agent

- [ ] Opening the tab loads the current real balance and days left
- [ ] Set, Add, Spend, and days changes on this tab update only this tab's advice and are never saved
- [ ] The tab shows a clear "Sandbox: changes here aren't saved" label and a **Use my real numbers** button that reloads the real values
- [ ] The Reset days button and the fixed 77-day default are removed from this tab
- [ ] Friends and the Plan tab always show the real balance
- [ ] Tests confirm that nothing is saved from this tab and that **Use my real numbers** restores the real values
