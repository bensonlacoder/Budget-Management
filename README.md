# Dining Points Budget

A Qt Widgets walking skeleton for managing student dining points. Set a balance,
record refills and purchases, and see daily and weekly spending recommendations.
Values last until the app closes.

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
   Overspending displays an error and leaves the balance unchanged.

For example, 700 points over 70 days recommends 10 points per day and 70 per week.
With fewer than 7 days left, the weekly recommendation equals the remaining balance.

## Project structure

- `src/balance.h` and `src/balance.cpp`: store points and reject invalid balance changes.
- `src/budget.h` and `src/budget.cpp`: calculate recommendations from the balance and days left.
- `UI/mainui.h` and `UI/mainui.cpp`: connect user actions to the models and update the display.
- `UI/mainui.ui`: define the layout; edit it in Qt Designer.
- `UI/main.cpp`: launch the application for both CMake and qmake builds.
- `tests/`: exercise the model classes with doctest; `test_main.cpp` supplies the test entry point.
- `src/prime.*` and `tests/test_prime.cpp`: retain the course template's example model and tests.

`MainUI` owns `Balance` and `Budget`. `Budget` references the same `Balance`, so
purchases and refills affect the next recommendation. Declare `Balance` first so
it outlives `Budget`. Keep calculations and validation in the models; keep widget
interaction and number formatting in the UI.
