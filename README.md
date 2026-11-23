# Project Dining Budget
This is the walking skeleton of Project Dining Budget, an app designed to help students
manage their dining points responsibly and efficiently. Our goal lies in making our
application simple and intuitive to use, while also allowing users a reasonable amount
of freedom regarding how they plan to budget their Dining Points.

Current features include: local accounts with a dining plan, logging purchases from
SCU menus or a receipt, purchase history, friends who can see each other's balance,
and daily/weekly spending recommendations. Accounts, balances and purchases are saved
on this computer between sessions.

See [REQUIREMENTS.md](REQUIREMENTS.md) for its scope and acceptance criteria, and
[SPECIFICATIONS.md](SPECIFICATIONS.md) for its design, calculations, and validation behavior.

## Run in Qt Creator

1. Open `CMakeLists.txt` or `UI/UI.pro` in Qt Creator.
2. Select a Qt 6 desktop kit with a matching C++17 compiler.
3. Click **Run**.

## Build and test from a terminal

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Install CMake 3.16 or newer, a C++17 compiler, and Qt 6 with the Widgets and SQL
modules. Run these commands from the repository root. The GitHub Actions workflow
builds the application and runs the model tests on each push and pull request.

Run `./build/app` on Linux or `./build/app.exe` on Windows. Add Qt's `bin` directory
to `PATH` before launching from a terminal. If CMake cannot find Qt, add
`-DCMAKE_PREFIX_PATH=<your-Qt-kit-directory>` to the configure command.

### Windows with the installed Qt MinGW kit

```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\Ninja;" + $env:PATH
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe -DCMAKE_C_COMPILER=C:/Qt/Tools/mingw1310_64/bin/gcc.exe
cmake --build build
ctest --test-dir build --output-on-failure
./build/app.exe
```

Adjust the Qt and MinGW paths for other installations. Use the compiler bundled
with your Qt kit. If an existing `build` directory uses another generator or
compiler, choose a new build directory in all commands.

## Use the UI

### Sign in

Create an account with a username (3 to 32 letters, numbers or underscores), a
display name, a dining plan and a password of at least 8 characters. Your balance
starts at the plan's points. Use **Account > Sign out** to switch accounts.

### Purchases tab

Choose a date (up to today) and a location. For SCU locations, pick a menu item to
fill in its name and price, choose a size or portion and any add-ons, then adjust the
total if your receipt differs. Choose **Enter receipt manually** for anything else.
**Log purchase** deducts the points; **Remove selected purchase** returns them.
Opening hours are fetched from the SCU dining site when available.

### Friends

**Account > Friends** shows your friend ID. Send a request with a friend's ID; once
they accept, each of you can see the other's display name and balance.

### Budget tab

1. Enter your points in **Amount** and click **Set balance**. The initial zero clears
   when the field receives focus so you can type immediately.
2. Set **Days left (including today)**. Recommendations update immediately.
3. Enter a refill or purchase amount, then click **Add points** or **Spend points**.
4. After a successful balance action, the amount field clears and
   **Successfully Updated** appears below the buttons. Errors appear in the same
   place; there are no confirmation popups.

The amount field accepts numbers from 0 to 1,000,000 with two decimal places.
It has no `points` suffix. Enter a new amount before each action. A blank amount
or a purchase larger than the balance displays an error and leaves the balance
unchanged. Balances are kept to two decimal places. Remaining days starts at 77 for a new account, goes down by one each day (including
while the app is open), and can be set from 0 to 90; larger values are rejected and
the previous count is kept. **Reset days** sets it back to 77.

For example, 700 points over 70 days recommends 10 points per day and 70 per week.
With fewer than 7 days left, the weekly recommendation equals the remaining balance.

## Scope and limitations

Accounts are stored in `accounts.json` in the app's local data folder (on Windows,
`%LOCALAPPDATA%\DiningPointsBudget`). Passwords are stored as salted PBKDF2 hashes.
Accounts and friends exist only on that computer. Each account's days-left count is
saved with the date you set it, and at sign-in it is reduced by the days that have
passed since then (new accounts start at 77). The app does not connect to a real
dining account. Built-in menu prices are a dated reference (see
[docs/dining-menu.md](docs/dining-menu.md)). Qt SQL is a build dependency inherited
from the template; the app does not use a database.

## Tests and continuous integration

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

CMake discovers every `tests/*.cpp` file and builds them into `test_build`.
`tests/test_main.cpp` supplies the doctest entry point. Add other tests as separate
files without another `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` definition. Model tests
are grouped by class and kind; add new ones to the matching file and put your name
in an `// Author:` comment above each test you write:

| File | Covers |
| --- | --- |
| `tests/balance_happy_test.cpp` | Setting, adding and spending valid amounts |
| `tests/balance_edge_test.cpp` | Zero amounts, rejected amounts, two-decimal rounding |
| `tests/budget_happy_test.cpp` | Daily and weekly advice for a normal number of days |
| `tests/budget_edge_test.cpp` | 0 and 90 day limits, fewer than 7 days, rejected day counts |

Each test case in [SPECIFICATIONS.md](SPECIFICATIONS.md) has a test whose name
starts with its ID (for example `SP-03:`).

`tests/ui/*.cpp` build into a second binary, `ui_tests`, which runs with a
`QApplication` (offscreen in CTest). It covers the Budget and Purchases tabs, the
saved account store (sign-in, balances, purchases, friends), and SCU hours parsing.
`tests/ui/remaining_days_test.cpp` supplies its `main()`. Tests that touch saved
accounts use `TestAccountStore` from `tests/ui/test_account_store.h`, which points
the app at a temporary `accounts.json` so your real accounts are never modified.

`.github/workflows/ci.yml` configures, builds, and runs CTest on Ubuntu for pushes
and pull requests. Open the repository's **Actions** tab to inspect each run.
For the individual CI exercise, use a separate branch to push an intentionally
failing assertion, confirm that test fails in CI, restore the assertion, and push
again to confirm a passing run. Keep the submitted application branch passing.

## Project structure

- `src/balance.h` and `src/balance.cpp`: store points and reject invalid balance changes.
- `src/budget.h` and `src/budget.cpp`: calculate recommendations from the balance and days left.
- `UI/mainui.h` and `UI/mainui.cpp`: connect user actions to the models and update the display.
- `UI/mainui.ui`: define the layout; edit it in Qt Designer.
- `UI/main.cpp`: show sign-in, then build the main window, tabs and Account menu.
- `UI/authdialog.*`: sign in or create an account.
- `UI/localaccounts.*`: save accounts, balances, purchases and friends to `accounts.json`.
- `UI/purchasespage.*`: log and remove purchases and show opening hours.
- `UI/scumenu.*` and `UI/lunchcatalog.cpp`: SCU locations, menu prices and hours parsing.
- `UI/friendsdialog.*`: send, accept and view friend requests.
- `tests/`: exercise the model classes with doctest; `test_main.cpp` supplies the test entry point.
- `tests/ui/`: exercise the widgets, account store and menu parsing.
- `.github/workflows/ci.yml`: build and test the project in GitHub Actions.
- `REQUIREMENTS.md`: define the walking skeleton's user-facing requirements.
- `SPECIFICATIONS.md`: describe how the current implementation fulfills them.

`MainUI` owns `Balance` and `Budget`. `Budget` references the same `Balance`, so
purchases and refills affect the next recommendation. Declare `Balance` first so
it outlives `Budget`. Keep calculations and validation in the models; keep widget
interaction and number formatting in the UI.
