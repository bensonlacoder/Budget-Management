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

## Build, test, and run from a terminal

Run these commands from the repository root. Each block installs the
dependencies (CMake, a C++17 compiler, and Qt 6), then builds the project, runs
the tests, and launches the app.

### macOS (Homebrew)

```sh
brew install cmake qt
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
ctest --test-dir build --output-on-failure
./build/app
```

### Ubuntu / Linux

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake qt6-base-dev libqt6sql6-sqlite
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/app
```

### Windows with the installed Qt MinGW kit

First install Qt 6 with the **MinGW 64-bit** kit, **MinGW** compiler, **CMake**,
and **Ninja** using the [Qt Online Installer](https://www.qt.io/download-qt-installer).
Then run:

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

Create an account with your email address, a display name (up to 32 characters), a
quarter dining plan and a password of at least 8 characters. The app emails you a
6-digit code; enter it to finish creating the account. Then sign in with your email
and password; upper and lower case in the email don't matter. Your balance starts at
the plan's points. Use **Account > Sign out** to switch accounts.

- **Resend code** sends a new code and cancels the old one. Codes expire after 15
  minutes, and 5 wrong tries cancel a code.
- Signing in before verifying sends a new code first.
- **Forgot password?** on the sign-in screen emails a code for setting a new password.
- **Account > Two-step sign-in** turns on asking for an emailed code after the password
  at every sign-in. **Resend code** works the same way, and each code signs in once.
- Sending codes needs the email sender set up; see [Email codes](#email-codes).

### Purchases tab

Choose a date (up to today) and a location. For SCU locations, pick a menu item to
fill in its name and price, choose a size or portion and any add-ons, then adjust the
total if your receipt differs. Choose **Enter receipt manually** for anything else.
**Log purchase** deducts the points; **Remove selected purchase** returns them.
Opening hours are fetched from the SCU dining site when available.

### Friends

**Account > Friends** shows your friend ID. Send a request with a friend's ID; once
they accept, each of you can see the other's display name and balance.

### Find meals

**Menus > Find meals** lists every menu item, or size or option, that costs at most
the points you enter, cheapest first, with its location. Choose a location to see
only its items. Items priced at the receipt total aren't listed; a note says how
many were left out. Select a meal and choose **Save this section** to keep its
location for quick access; **Show items** jumps back to a saved section and
**Remove** deletes it. Saved sections are kept with your account. Prices come from
the dated reference menu.

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
unchanged. Remaining days starts at 77, goes down by one each day, and can be set
from 0 to 90, including today. Entering more than 90 shows an error and keeps the
previous day count. **Reset days** sets it back to 77. Recommendations are rounded
down to the nearest hundredth so following them never spends more than the balance.

For example, 700 points over 70 days recommends 10 points per day and 70 per week.
With fewer than 7 days left, the weekly recommendation equals the remaining balance.

## Email codes

The app sends sign-up, password reset and two-step sign-in codes through a Google Apps
Script web app, [apps-script/send-code.gs](apps-script/send-code.gs), running on a
Gmail account used only for this app. **Its URL is built into the app**
(`EmailSender::DefaultUrl` in [UI/emailsender.h](UI/emailsender.h)), so email codes
work on any computer with internet access and no setup, including a grader's. Tests
never send email; they use a fake sender.

The app holds no secret. The Gmail access stays on Google's servers, and anyone could
find the URL, so the script protects itself instead:

- It only ever sends one fixed message containing a 6-digit code.
- It sends one email per address per minute and at most 5 per address per hour.
- It sends at most 90 emails a day in total, below Gmail's limit of about 100.

If sending fails (no internet, or a limit was reached), the app shows "Couldn't send
the code. Wait a minute, then try again."

**Set up a new sender (only if replacing the team's):**

1. Create a new Gmail account for the app. Don't use an SCU or personal account; SCU
   accounts can't share web apps with *Anyone*.
2. Signed in as that account, open [script.google.com](https://script.google.com),
   choose **New project**, and replace the contents of `Code.gs` with
   `apps-script/send-code.gs`. Save.
3. Choose **Deploy > New deployment**, select type **Web app**, set **Execute as** to
   *Me* and **Who has access** to *Anyone*, then **Deploy**. Google asks you to
   authorize sending email as that account; allow it. Copy the **Web app URL**
   (it ends in `/exec`). *Anyone* is needed because the desktop app doesn't sign in to
   Google.
4. Put the URL in `EmailSender::DefaultUrl`, or, to try it without changing the code,
   in an `email-sender.ini` file (see [email-sender.example.ini](email-sender.example.ini)).
5. After editing the script later, use **Deploy > Manage deployments > Edit > New
   version** so the URL stays the same.

## Scope and limitations

Accounts are stored in a SQLite database, `budget.sqlite`, in the app's local data
folder (on Windows, `%LOCALAPPDATA%\DiningPointsBudget`). Passwords are stored as
salted PBKDF2 hashes, and points are stored as whole cents. Accounts and friends exist
only on that computer. Accounts from the earlier `accounts.json` file are **not**
carried over; create a new account. Only quarter dining plans are offered. Each account's days-left count is
saved with the date you set it, and at sign-in it is reduced by the days that have
passed since then (new accounts start at 77). The app does not connect to a real
dining account. Built-in menu prices are a dated reference (see
[docs/dining-menu.md](docs/dining-menu.md)).

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
| `tests/balance_happy_case.cpp` | Setting, adding and spending valid amounts |
| `tests/balance_edge_case.cpp` | Zero amounts, rejected amounts, two-decimal rounding |
| `tests/budget_happy_case.cpp` | Daily and weekly advice for a normal number of days |
| `tests/budget_edge_case.cpp` | 0 and 90 day limits, fewer than 7 days, rejected day counts, rounding down |

Each test case in [SPECIFICATIONS.md](SPECIFICATIONS.md) has a test whose name
starts with its ID (for example `SP-03:`).

`tests/ui/*.cpp` build into a second binary, `ui_tests`, which runs with a
`QApplication` (offscreen in CTest). It covers the sign-in dialog, the Budget and
Purchases tabs, the SQLite account store (every storage function, with success and
failure cases), and SCU hours parsing.
`tests/ui/remaining_days_test.cpp` supplies its `main()`. Tests that touch saved
accounts use `TestAccountStore` from `tests/ui/test_account_store.h`, which points
the app at a temporary database and deletes it afterwards, so your real accounts are
never modified. Pass `Profile::accountId` (not the email) to storage functions.

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
- `UI/authdialog.*`: sign in (with an emailed code when two-step sign-in is on), create
  an account, verify its email, or reset a password; also the Two-step sign-in menu item.
- `UI/emailsender.*`: send codes through the Apps Script web app (`apps-script/`).
- `UI/menusearchdialog.*`: find meals within a point limit and keep saved menu sections.
- `UI/localaccounts.*`: all saved data (accounts, balances, purchases, refunds,
  corrections, plan settings, friends, saved menu sections) in SQLite. Pages call only
  these functions and never open the database themselves.
- `UI/purchasespage.*`: log and remove purchases and show opening hours.
- `UI/scumenu.*` and `UI/lunchcatalog.cpp`: SCU locations, menu prices and hours parsing.
- `UI/friendsdialog.*`: send, accept and view friend requests, see sent requests still
  waiting for an answer, and remove a friend.
- `tests/`: exercise the model classes with doctest; `test_main.cpp` supplies the test entry point.
- `tests/ui/`: exercise the widgets, account store and menu parsing.
- `.github/workflows/ci.yml`: build and test the project in GitHub Actions.
- `REQUIREMENTS.md`: define the walking skeleton's user-facing requirements.
- `SPECIFICATIONS.md`: describe how the current implementation fulfills them.

`MainUI` owns `Balance` and `Budget`. `Budget` references the same `Balance`, so
purchases and refills affect the next recommendation. Declare `Balance` first so
it outlives `Budget`. Keep calculations and validation in the models; keep widget
interaction and number formatting in the UI.
