# Dining Points Budget Requirements

## Functional requirements

### View funds

| ID | Requirement |
| --- | --- |
| ✅ FR-01 | View the current dining-point balance. |
| FR-10 | View purchase and refund history and filter it by date range or dining location. |
| ✅ FR-11 | Open an individual purchase to see its date, location, and points spent. |
| FR-12 | View daily and weekly spending charts that reflect recorded purchases and refunds. |
| FR-13 | See how many points were spent at each dining location for a selected date range. |
| FR-14 | Compare actual daily and weekly spending with the budget for the same period and see how far over or under budget it is. |
| FR-15 | See when the balance and purchase history were last successfully updated. |
| ✅ FR-16 | See the current spending plan from the main budget view, including daily and weekly recommendations and time remaining in the quarter. |

### Manage account

| ID | Requirement |
| --- | --- |
| FR-17 | Create an account with an email address and password, and sign in with them to access its saved data. |
| ✅ FR-18 | Sign out and return to the sign-in screen. |
| FR-19 | See the email address that identifies the signed-in account. |
| FR-20 | View and update profile information, including display name and email address. |
| FR-21 | Change the account password while signed in, or reset a forgotten password with a code sent to the account's email address. |
| FR-46 | Verify the account's email address with a code sent at sign-up before the account can be used, and request a new code if it expires or does not arrive. |
| FR-22 | See the current quarter's dates from the SCU academic calendar (first day of classes through the last day of finals) and have the days remaining calculated from them automatically. Between quarters, see the next quarter's full length and the date it starts. |
| FR-23 | Reopen the app and find the signed-in account's saved balance, history, budget settings, alert preferences, friends, and saved menus. |

### Add friends

| ID | Requirement |
| --- | --- |
| ✅ FR-24 | View and copy a shareable friend ID. |
| ✅ FR-25 | Look up an exact friend ID in a search field and send a friend request. |
| ✅ FR-26 | View incoming friend requests and choose whether to accept or decline each one. |
| FR-27 | See which outgoing friend requests are still pending. |
| ✅ FR-28 | View an accepted friend's display name and remaining dining-point balance. Keep email, sign-in details, purchase history, and other private account data out of the friend profile. |
| FR-29 | Remove a friend and end access to each other's dining-point balances. |
| ✅ FR-30 | Get clear feedback when a friend ID is unknown, belongs to the signed-in user, or already has a pending request or friendship. |

### Enter, update, and record

| ID | Requirement |
| --- | --- |
| ✅ FR-02 | Set the dining-point balance manually. |
| ✅ FR-03 | Add points to the balance. |
| ✅ FR-04 | Subtract points from the balance. |
| FR-05 | Correct the number of days remaining manually. A corrected count keeps going down by one each day, and a reset returns to the count calculated from the quarter calendar. |
| ✅ FR-08 | Get an error when entering an invalid amount or day count, or trying to spend more points than are available. Keep the previously saved values after a rejected entry. |
| ✅ FR-09 | Enter and save several point amounts in succession. After each successful entry, see confirmation and have the amount field ready for the next entry. |
| ✅ FR-31 | Record a purchase manually with its date, location, and points spent, and see the new balance. |
| FR-32 | Record a full or partial refund, associate it with the original purchase when available, and see the returned points reflected in the balance and spending totals. |
| FR-33 | Correct or remove a manual purchase or refund and see the resulting changes to the balance, charts, and budget advice. |
| FR-36 | Try different point balances, days remaining, and spending plans in a sandbox that starts from the real values and never changes the saved balance, purchase history, or balance shared with friends. |
| FR-47 | Correct the real balance from the profile page by setting, adding, or subtracting points. Each correction appears in history as a balance correction, and a correction that lowers the balance counts as unlogged spending in charts and totals. |
| FR-37 | Enter how many points to keep in reserve at the end of the quarter. |
| FR-38 | Enter or upload usual dining habits, including which days they eat on campus and how many meals they eat on those days. |

### Receive advice

| ID | Requirement |
| --- | --- |
| ✅ FR-06 | See how many dining points they can spend per day. |
| ✅ FR-07 | See how many dining points they can spend over the next seven days or through the quarter end if sooner. |
| FR-39 | Get spending suggestions based on the current balance, time remaining, reserve goal, and dining habits. Use the days remaining from the quarter calendar, or the manually corrected count when there is one. |
| FR-40 | See spending recommendations refresh after changes to the balance, remaining time, reserve goal, or dining habits. |
| FR-41 | Choose separately whether to receive alerts for going over the daily budget and for running out of points before the quarter ends. |
| FR-42 | Receive enabled alerts when recorded spending exceeds the daily budget or projected spending would exhaust the balance before quarter end. |
| FR-43 | See when projected spending would leave fewer points than the end-of-quarter reserve goal. |

### Find and save

| ID | Requirement |
| --- | --- |
| FR-44 | Find available meal options at or below a chosen point limit and see their dining locations and point prices. |
| FR-45 | Save specific menu sections for quick access later and remove them from the saved list when desired. |
