# Dining Points Budget Specifications

These are the specifications for our walking skeleton. Each one meets one or
more requirements in [REQUIREMENTS.md](REQUIREMENTS.md) and has one test case
in `tests/`.

## SP-01: Set the balance

**Requirements:** FR-01, FR-02

The app starts with a balance of 0. Entering an amount and selecting
**Set balance** replaces the balance with that amount, and the new balance is
shown as `700.00 points`.

**Test case:** a new `Balance` is 0. After `setBalance(700)`, `getBalance()` is 700.

## SP-02: Add and spend points

**Requirements:** FR-03, FR-04, FR-09

**Add points** increases the balance by the entered amount. **Spend points**
decreases it by the entered amount. Spending the entire balance is allowed and
leaves 0. The user can make successive balance changes in the same session;
each change uses the latest balance.

**Test case:** start at 700, add 50 to get 750, spend 25 to get 725, then spend
725 to get 0.

## SP-03: Prevent invalid balance changes

**Requirements:** FR-05

The balance can never go below 0. Spending more than the balance, or using a
negative amount, is rejected (`Balance` throws `std::invalid_argument`) and the
balance stays the same. The app shows `Not enough points. Enter an amount no
greater than your balance.`

**Test case:** start at 725. `subtract(726)` and `add(-5)` both throw, and the
balance is still 725.

## SP-04: Recommend daily and weekly spending

**Requirements:** FR-06, FR-07, FR-08

Remaining days counts today and must be a whole number from 0 to 90, inclusive.
Zero days means the budgeting period has ended.
The app starts with 70 remaining days. Typed day counts are validated when the
user presses Enter or leaves the field; arrow adjustments are validated immediately.
If the user enters more than 90 days, the app restores the previous day count,
keeps the existing recommendations, and displays `Remaining days must be between
0 and 90, including today. Your previous day count has been kept.`
Constructing a `Budget` or calling `setRemainingDays` with a value outside
this range throws `std::invalid_argument`. A rejected update leaves the previous
remaining-day count unchanged.

- With 0 days remaining, both recommendations equal the current balance; the balance is unchanged.
- With at least 1 day remaining, per day = current balance / remaining days.
- Next 7 days = per day x 7, or the whole balance if fewer than 7 days remain, including 0.
- Recommendations use the latest balance and update when the balance or days change.

**Test case:** with 700 points and 70 days, per day is 10 and next 7 days is 70.
Change to 3 days: next 7 days becomes 700. Values of 0, 1, and 90 days are accepted.
Change to 0 days: both recommendations become 700 and the balance remains 700.
Setting -1 or 91 days throws and leaves the previous remaining-day count unchanged.
Use `doctest::Approx` for calculated values.

**UI regression test:** enter 91 or 366 days. The app shows the range message,
retains the previous day count and recommendations, and remains usable. Entering
90 afterward succeeds and clears the error.
