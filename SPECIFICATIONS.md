# Dining Points Budget Specifications

## Implementation overview

The application uses C++17 and Qt 6 Widgets. `UI/main.cpp` creates a `QApplication`,
shows `MainUI`, and starts the event loop. Both CMake and qmake use this entry point.
Qt Designer defines the window in `UI/mainui.ui`.

| Component | Responsibility |
| --- | --- |
| `Balance` in `src/balance.*` | Store the current points and validate set, add, and subtract operations. |
| `Budget` in `src/budget.*` | Store remaining days and calculate recommendations using the current balance. |
| `MainUI` in `UI/mainui.*` | Read inputs, call model methods, refresh labels, and show feedback. |
| `tests/` | Exercise model behavior with doctest. |
| `CMakeLists.txt` | Build the `app` and `test_build` targets and register tests with CTest. |
| `UI/UI.pro` | Build the GUI application with qmake. |

`MainUI` owns a `Balance` followed by a `Budget`. `Budget` holds a `const Balance&`
rather than a copy, so recommendations use the latest balance. The declaration
order ensures that the balance exists before the budget and outlives it.

## Model contracts

### Balance

The model stores points as a `double` and starts at zero unless a starting balance
is supplied. The public operations are:

| Operation | Result | Invalid input |
| --- | --- | --- |
| `Balance(startingBalance)` | Initialize the balance. | A negative starting balance throws `std::invalid_argument`. |
| `setBalance(amount)` | Replace the stored balance. | A negative amount throws before changing the balance. |
| `getBalance()` | Return the stored balance. | None. |
| `add(amount)` | Increase the balance by the amount. | A negative amount throws before changing the balance. |
| `subtract(amount)` | Decrease the balance by the amount. | A negative amount or an amount above the balance throws before changing it. |

Zero is a valid amount. Spending the entire balance is permitted. These contracts
describe ordinary finite numbers entered through the UI; the model does not
explicitly reject non-finite values supplied by another caller.

### Budget

`Budget(balance, remainingDays)` references the balance and stores an integer day
count. Construction and `setRemainingDays(days)` throw `std::invalid_argument`
for values below 1. A failed setter leaves the previous day count unchanged.
The UI limits days to 366; the model itself has no upper day-count limit.

```text
daily recommendation = current balance / remaining days

if remaining days < 7:
    weekly recommendation = current balance
else:
    weekly recommendation = daily recommendation * 7
```

Today counts as one remaining day. Calculations use the stored values; the UI
formats the results to two decimal places and appends `points`. Display rounding
does not change the stored balance. Binary floating-point arithmetic means this
skeleton does not provide exact fixed-point accounting.

## Window and input behavior

| Control or display | Behavior |
| --- | --- |
| Current balance | Start at `0.00 points`; refresh after balance actions. |
| Amount | Use a `QDoubleSpinBox` with range 0 to 1,000,000 and two decimal places. Show no unit suffix inside the field. |
| Set balance / Add points / Spend points | Validate that an amount was entered, then call the corresponding `Balance` method. |
| Days left (including today) | Use a `QSpinBox` with range 1 to 366; initialize it to 70. |
| Per day / Next 7 days | Display the current model recommendations with two decimal places. |
| Shared message area | Use the existing `errorLabel` below the balance buttons for both errors and success messages. |

When the amount field receives focus and its value is zero, an event filter clears
its text. A successful balance action refreshes all result labels, resets the
amount control's value to zero, returns focus to it, clears its text, and displays
`Successfully Updated`. A blank action displays `Enter an amount first.` without
changing the balance.

An oversized purchase throws from `Balance::subtract()`. `MainUI::spendPoints()`
catches it and displays `Not enough points. Enter an amount no greater than your
balance.` The balance and recommendations remain unchanged, and the entered
amount remains available for correction.

Changing the day count updates recommendations immediately and clears the current
message. It does not show a success confirmation or clear an entered amount.
There are no success popups or automatic message-dismissal timers.

## State and storage

All state stays in memory. A new application session starts with zero points and
70 remaining days. There is no transaction log, database, account connection, or
automatic countdown. CMake retains the course template's Qt SQL dependency, but
the current application does not access a database.

## Build and automated verification

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

Configure the project first using the instructions in [README.md](README.md).
CMake discovers model `.cpp` files in `src/` and test `.cpp` files in `tests/`.
The application target explicitly includes the UI sources. `test_build` excludes
the application entry point and UI sources; `tests/test_main.cpp` supplies main
through doctest. The qmake project builds the GUI only.

GitHub Actions installs Qt 6 development dependencies on Ubuntu, configures the
project, builds it, and runs CTest. Pushes and pull requests trigger the workflow.
An intentional-failure CI exercise belongs on a separate branch and must be
followed by a corrected passing run.

### Current coverage and remaining checks

`tests/Nathan_test.cpp` covers valid balance set, add, and subtract operations.
Additional model tests should cover negative starting balances and amounts,
overspending without mutation, spending down to zero, invalid remaining days,
daily and weekly calculations, fewer than seven days remaining, and recalculation
after balance or day-count changes. Use approximate comparisons for computed
floating-point results. Current test success alone does not establish this coverage.

Use this manual UI checklist alongside model tests:

1. Start a new session. Confirm zero balance and 70 remaining days.
2. Enter 700 and select **Set balance**. Confirm 700.00 balance, 10.00 per day,
   70.00 for the next seven days, an empty amount field, and the inline success message.
3. Enter 50 and select **Add points**, then enter 25 and select **Spend points**.
   Confirm balances of 750.00 and 725.00 and updated recommendations after each action.
4. Enter 726 and select **Spend points**. Confirm the error and unchanged 725.00 balance.
5. Set remaining days to 3. Confirm 241.67 per day and 725.00 for the next seven days.
6. Spend 725. Confirm zero balance and zero recommendations. Try another action
   with the cleared amount field and confirm the blank-input error.
7. Set a nonzero balance, close the app, and reopen it. Confirm the default state returns.

These scenarios verify the behavior specified in [REQUIREMENTS.md](REQUIREMENTS.md).
