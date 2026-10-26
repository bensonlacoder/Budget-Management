# Project Dining Budget
This is the walking skeleton of Project Dining Budget, an app designed to help students
manage their dining points responsibly and efficiently. Our goal lies in making our
application simple and intuitive to use, while also allowing users a reasonable amount
of freedom regarding how they plan to budget their Dining Points.

Current features include: setting a balance, record refills and purchases, and see
daily/weekly spending recommendations. Values last until the app closes.

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
unchanged. Remaining days starts at 70 and can be set from 1 to 366.

For example, 700 points over 70 days recommends 10 points per day and 70 per week.
With fewer than 7 days left, the weekly recommendation equals the remaining balance.

## Scope and limitations

The application keeps one balance and one remaining-day count in memory. It does
not save data, keep a transaction history, connect to a dining account, or update
the day count automatically. Closing and reopening the app resets the balance to
zero and remaining days to 70. Qt SQL is a build dependency inherited from the
template; the app does not currently use a database.

## Tests and continuous integration

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

CMake discovers every `tests/*.cpp` file and builds them into `test_build`.
`tests/test_main.cpp` supplies the doctest entry point. Add other tests as separate
files without another `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` definition, and put
your name in a comment in your contribution.

`tests/Nathan_test.cpp` currently checks setting, adding to, and subtracting from
the balance with valid amounts. Invalid inputs and budget calculations also need
automated coverage; passing this test alone does not verify those behaviors.
See the verification checklist in [SPECIFICATIONS.md](SPECIFICATIONS.md).

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
- `UI/main.cpp`: launch the application for both CMake and qmake builds.
- `tests/`: exercise the model classes with doctest; `test_main.cpp` supplies the test entry point.
- `.github/workflows/ci.yml`: build and test the project in GitHub Actions.
- `REQUIREMENTS.md`: define the walking skeleton's user-facing requirements.
- `SPECIFICATIONS.md`: describe how the current implementation fulfills them.

`MainUI` owns `Balance` and `Budget`. `Budget` references the same `Balance`, so
purchases and refills affect the next recommendation. Declare `Balance` first so
it outlives `Budget`. Keep calculations and validation in the models; keep widget
interaction and number formatting in the UI.
