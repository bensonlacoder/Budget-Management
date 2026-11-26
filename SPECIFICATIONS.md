# Dining Points Budget Specifications

These are the specifications for our walking skeleton. Each one meets one or
more requirements in [REQUIREMENTS.md](REQUIREMENTS.md) and is covered by tests
in `tests/`.

## SP-01: Set the balance

**Requirements:** FR-01, FR-02

The app starts with a balance of 0. Entering an amount and selecting
**Set balance** replaces the balance with that amount, and the new balance is
shown as `700.00 points`.

**Test case:** a new `Balance` is 0. After `setBalance(700)`, `getBalance()` is 700.

## SP-02: Add and spend points

**Requirements:** FR-03, FR-04

**Add points** increases the balance by the entered amount. **Spend points**
decreases it by the entered amount. Spending the entire balance is allowed and
leaves 0. The user can make successive balance changes in the same session;
each change uses the latest balance. The balance is kept to two decimal places,
so any balance shown can be spent in full (for example, 0.70 + 0.10 shows 0.80
and spending 0.80 leaves 0).

**Test case:** start at 700, add 50 to get 750, spend 25 to get 725, then spend
725 to get 0.

## SP-03: Prevent invalid balance changes

**Requirements:** FR-08

The balance can never go below 0. Spending more than the balance, or using a
negative amount, is rejected (`Balance` throws `std::invalid_argument`) and the
balance stays the same. The app shows `Not enough points. Enter an amount no
greater than your balance.`

**Test case:** start at 725. `subtract(726)` and `add(-5)` both throw, and the
balance is still 725.

**Additional invalid-input cases:** `subtract(-5)` throws and preserves the
balance. Constructing `Balance(-5)` throws. Setting an existing balance to a
negative amount throws and preserves its previous value. Check the unchanged
balance immediately after each rejected operation.

**UI test:** set 700 points, then try to spend 701. The app shows the
insufficient-points message and preserves the displayed balance and both
recommendations. Spending 70 afterward succeeds, showing 630 points, 8.18 per day,
and 57.27 for the next 7 days with 77 days remaining.

## SP-04: Recommend daily and weekly spending

**Requirements:** FR-05, FR-06, FR-07, FR-08

Remaining days counts today and must be a whole number from 0 to 90, inclusive.
Zero days means the budgeting period has ended.
The days remaining come from the quarter calendar (SP-05), or from a manual
correction on the Profile tab. The count goes down by one each day, including
while the app is open, and never below 0. On the Budget tab, which is a sandbox
(SP-06), typed day counts are validated when the
user presses Enter or leaves the field; arrow adjustments are validated immediately.
If the user enters more than 90 days, the app restores the previous day count,
keeps the existing recommendations, and displays `Remaining days must be between
0 and 90, including today. Your previous day count has been kept.`
Constructing a `Budget` or calling `setRemainingDays` with a value outside
this range throws `std::invalid_argument`. A rejected update leaves the previous
remaining-day count unchanged.

- With 0 days remaining, both recommendations equal the current balance; the balance is unchanged.
- With at least 1 day remaining, per day = current balance / remaining days, rounded down to the
  nearest hundredth so following the advice never spends more than the balance.
- Next 7 days = current balance x 7 / remaining days, rounded down the same way, or the whole
  balance if fewer than 7 days remain, including 0.
- Recommendations use the latest balance and update when the balance or days change.
- When a reserve goal is set (SP-07), "balance" in these rules means the spendable
  points: balance minus the reserve, never below 0.

**Test case:** with 700 points and 70 days, per day is 10 and next 7 days is 70.
Change to 3 days: next 7 days becomes 700. Values of 0, 1, and 90 days are accepted.
Change to 0 days: both recommendations become 700 and the balance remains 700.
Setting -1 or 91 days throws and leaves the previous remaining-day count unchanged.
Use `doctest::Approx` for calculated values.

**Balance-update cases:** with 700 points and 70 days, setting the balance to
770 or adding 70 updates the recommendations to 11 per day and 77 per week.
Independently, spending 70 from 700 updates them to 9 per day and 63 per week.

**Constructor and upper-bound cases:** constructing a `Budget` with -1 or 91
days throws. Constructing with 90 days and updating from 30 to 90 days both
succeed and produce recommendations based on the new day count.

**Balance UI cases:** starting the sandbox from 0 points and 77 days, verify the
initial `0.00 points` display. Setting, adding, and spending update the balance and recommendations with
two decimal places. Setting replaces an existing balance; spending the entire
balance leaves all three displayed point amounts at zero.

**UI regression test:** enter 91 or 366 days. The app shows the range message,
retains the previous day count and recommendations, and remains usable. Entering
90 afterward succeeds and clears the error.

## SP-05: Quarter calendar and days remaining

**Requirements:** FR-05, FR-22, FR-39

Days remaining are calculated from SCU's 2026–27 undergraduate calendar, from
the first day of classes through the last day of finals, counting both:

| Quarter | Start | End | Length |
| --- | --- | --- | --- |
| Fall 2026 | 2026-09-21 | 2026-12-11 | 82 days |
| Winter 2027 | 2027-01-04 | 2027-03-19 | 75 days |
| Spring 2027 | 2027-03-30 | 2027-06-10 | 73 days |

Source: SCU Undergraduate Bulletin 2026–27. Spring's start differs by one day
between SCU sources; March 30 is used. The dates live in `src/quarter.cpp`.

- During a quarter, days remaining are the days from today through the last day
  of finals, counting today. On 2026-11-22 that is 20.
- Between quarters, days remaining are the next quarter's full length, and the
  Profile tab shows the date it starts. On 2026-12-20 that is Winter's 75.
- After the last listed quarter, the Profile tab says the calendar needs
  updating, and the app uses the saved day count until the user corrects it.
- A manual correction from the Profile tab (**Correct days**, 0 to 90) takes
  priority over the calendar and goes down by one each day. **Reset days** clears
  it and returns to the calendar value. Reset is unavailable when there is no
  correction or the calendar has ended.

**Test cases:** the first day of Winter gives 75 and its last day gives 1;
2026-12-12 and 2026-12-20 are breaks before Winter; 2027-06-11 reports that the
calendar has ended; a correction of 40 on 2026-11-22 gives 40 instead of 20.

## SP-06: Budget tab sandbox

**Requirements:** FR-36

The Budget tab starts from the real balance and days remaining each time it is
opened, and **Use my real numbers** reloads them. Set, Add, Spend and day
changes there update only that tab's recommendations; nothing is saved, added to
history, or shown to friends. The tab is labelled `Sandbox: changes here aren't
saved`, and it has no Reset days button. Real corrections are made on the
Profile tab.

**Test cases:** after Set, Add, Spend and a day change on the Budget tab, the
saved balance, days correction and history are unchanged. **Use my real
numbers** restores the real balance and days after sandbox changes.

## SP-07: Reserve goal, spending rate and projection

**Requirements:** FR-37, FR-39, FR-42, FR-43

The reserve goal is the number of points to keep at the end of the quarter. It
defaults to 0 and is kept to the nearest hundredth.

- A reserve that is negative, not a number, or more than the current balance is
  rejected (`std::invalid_argument`), and the previous reserve is kept.
- Spendable points = balance - reserve, never below 0. If the balance later falls
  below the reserve, spendable points and all advice are 0 until the reserve is lowered.
- Daily and weekly advice (SP-04) divide the spendable points instead of the balance.
- **Recent spending rate** = net points spent over the last 7 days, counting today,
  divided by 7. Net spending is purchases plus balance corrections that lowered the
  balance, minus refunds dated in the same 7 days. A rate below 0 counts as 0. It is
  worked out from the history, so it covers spending the user didn't log as a purchase.
- **Projected end balance** = balance - rate x days remaining, to the nearest
  hundredth. It can be negative, which means the points run out before the quarter ends.
- The reserve warning shows when the projected end balance is less than the reserve.
- The run-out condition (for the alert in FR-42) is a projected end balance below 0.

**Test cases:** with 700 points, 70 days and a 140-point reserve, per day is 8 and
next 7 days is 56. A reserve of -1 or 701 is rejected and the previous reserve
stays. Spending 600 of the 700 leaves 0 spendable points. With 700 points, 70 days,
a 140-point reserve and a rate of 9 per day, the projection is 70 and the warning
shows; at 8 per day the projection is 140 and it does not. At 11 per day the
projection is -70, so the points run out.

## SP-08: Dining habits and per-meal suggestions

**Requirements:** FR-38, FR-39, FR-40

Dining habits are the number of meals eaten on campus on each weekday, Monday
through Sunday, each from 0 to 10. A day with 0 meals is not a campus day. A meal
count outside 0 to 10 is rejected and the previous habits are kept.

- The remaining campus days and meals count the days remaining (SP-05) starting
  today, using each day's weekday.
- Points per meal = spendable points / remaining campus meals, and points per campus
  day = spendable points / remaining campus days, each rounded down to the nearest
  hundredth.
- With no habits entered, or no campus days left, both suggestions are 0 and the
  Plan tab shows only the daily and weekly advice.
- Suggestions update when the balance, days remaining, reserve or habits change.
- On the Plan tab, each weekday has a meals box (0 to 10). A change is saved right away.

**Uploading habits (CSV):** **Upload CSV...** reads a text file with one day per line,
the day and its meals separated by a comma:

```csv
day,meals
Monday,2
Wednesday,3
Fri,1
```

- The `day,meals` header is optional. Blank lines and spaces around values are ignored.
- Days are full English names or three-letter abbreviations, in any case. Days not
  listed have 0 meals, and each day may be listed only once.
- Meals are whole numbers from 0 to 10. At least one day must be listed.
- Files over 10 KB are rejected. If any line is wrong, nothing changes and the message
  names the first bad line, for example `Line 3: "Funday" isn't a day of the week.`

**Test cases:** with 2 meals on Monday through Friday, 14 days starting on a Monday
have 10 campus days and 20 meals; with 400 spendable points that is 20 per meal and
40 per campus day. 3 days starting on a Saturday have 1 campus day. Habits of all 0,
or a meal count of 11, give no suggestions or are rejected.

## SP-10: Purchase history and refunds

- History shows purchases, refunds, and balance corrections. Date filters are inclusive.
  A linked refund follows the current location of its original purchase. Standalone
  refunds and balance corrections appear when the location filter is set to All locations.
- During a quarter listed in the calendar, purchase date controls allow dates from the
  first day of classes through today. Outside a listed quarter they allow dates through
  today.
- Removing a purchase also removes all its linked refunds. The balance increases by
  the unrefunded part of the purchase. Removing a refund on its own deducts the points it
  returned. Any edit or removal that would make the balance negative is rejected.

## SP-11: Spending and budget comparison

- Location totals and the daily and weekly charts use transactions dated within the
  selected range. Purchases add to spending, refunds reduce spending on the refund date,
  and balance corrections that lower the balance count as **Unlogged**. Standalone
  refunds reduce spending under **Standalone refunds**. Corrections that increase the
  balance are excluded.
- Past periods are compared with the current Plan budget, calculated from the saved
  balance, reserve, and days remaining. Historical budget snapshots are not stored.
  Weeks run Monday through Sunday. For a partial week at either end of the selected
  range, the weekly budget is multiplied by the fraction of days included and rounded
  down to the nearest hundredth.
## SP-09: Spending alerts

**Requirements:** FR-41, FR-42

The Plan tab has two alert choices, each off until turned on, saved separately:

- **Over today's budget:** applies when today's net spending (SP-07's rules,
  counting only today) is more than today's budget. Today's budget is the daily
  advice (SP-04) worked out from the balance before today's spending, so spending
  today doesn't lower its own limit: (balance + spent today - reserve) / days
  remaining, rounded down to the hundredth.
- **Run out before the quarter ends:** applies when the projected end balance
  (SP-07) is below 0.

Neither applies once the quarter has ended (0 days remaining). An alert that is
off never appears. While an alert that is on applies, the Plan tab shows it in a
red banner at the top, and the app pops it up once a day, at most, the first time
it applies after sign-in or after the balance, days, reserve, or alert choice
changes. The banner disappears when the alert stops applying.

**Test cases:** with 695 points left after spending 25 today, no reserve and 72
days, today's budget is (695 + 25) / 72 = 10 and the over-budget alert applies;
with 710 left after spending exactly 10, it does not. With 700 points, 70 days and 11 points a day of recent spending, the run-out
alert applies; at 10 a day it does not. With either alert off, nothing appears.
