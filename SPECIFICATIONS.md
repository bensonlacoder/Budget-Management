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

**Requirements:** FR-03, FR-04

**Add points** increases the balance by the entered amount. **Spend points**
decreases it by the entered amount. Spending the entire balance is allowed and
leaves 0.

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

Remaining days counts today and must be at least 1.

- Per day = current balance / remaining days.
- Next 7 days = per day x 7, or the whole balance if fewer than 7 days remain.
- Recommendations use the latest balance and update when the balance or days change.

**Test case:** with 700 points and 70 days, per day is 10 and next 7 days is 70.
Change to 3 days: next 7 days becomes 700. Setting 0 days throws.
Use `doctest::Approx` for calculated values.
