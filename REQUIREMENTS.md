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
| FR-15 | See when the balance and purchase history were last successfully updated and whether the update was manual or imported. |
| ✅ FR-16 | See the current spending plan from the main budget view, including daily and weekly recommendations and time remaining in the quarter. |

### Manage account

| ID | Requirement |
| --- | --- |
| ✅ FR-17 | Create an account and sign in to access its saved data. |
| ✅ FR-18 | Sign out and return to the sign-in screen. |
| FR-19 | View the user ID for the signed-in account. |
| FR-20 | View and update profile information, including display name and email address. |
| FR-21 | Change the account password while signed in or reset a forgotten password through a verified recovery method. |
| FR-22 | View the quarter end date, which defaults to the last day of finals week, and set a custom earlier or later end date. |
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
| ✅ FR-05 | Enter the number of days remaining in the quarter manually. |
| ✅ FR-08 | Get an error when entering an invalid amount or day count, or trying to spend more points than are available. Keep the previously saved values after a rejected entry. |
| ✅ FR-09 | Enter and save several point amounts in succession. After each successful entry, see confirmation and have the amount field ready for the next entry. |
| ✅ FR-31 | Record a purchase manually with its date, location, and points spent, and see the new balance. |
| FR-32 | Record a full or partial refund, associate it with the original purchase when available, and see the returned points reflected in the balance and spending totals. |
| FR-33 | Correct or remove a manual purchase or refund and see the resulting changes to the balance, charts, and budget advice. |
| FR-34 | Connect or disconnect a supported dining card. While connected, receive automatic updates to balances, purchases, and refunds as data becomes available. Keep manual entry available. |
| FR-35 | When a dining-card update fails, see its status and the last successful update. Retry or re-import without duplicating entries or applying a transaction to the balance twice. |
| FR-36 | Try different point balances and spending plans without changing the saved balance, purchase history, or balance shared with friends. |
| FR-37 | Enter how many points to keep in reserve at the end of the quarter. |
| FR-38 | Enter or upload usual dining habits, including which days they eat on campus and how many meals they eat on those days. |

### Receive advice

| ID | Requirement |
| --- | --- |
| ✅ FR-06 | See how many dining points they can spend per day. |
| ✅ FR-07 | See how many dining points they can spend over the next seven days or through the quarter end if sooner. |
| FR-39 | Get spending suggestions based on the current balance, time remaining, reserve goal, and dining habits. Use the quarter end date to calculate time remaining when set and the manually entered day count otherwise. |
| FR-40 | See spending recommendations refresh after changes to the balance, remaining time, reserve goal, or dining habits. |
| FR-41 | Choose separately whether to receive alerts for going over the daily budget and for running out of points before the quarter ends. |
| FR-42 | Receive enabled alerts when recorded spending exceeds the daily budget or projected spending would exhaust the balance before quarter end. |
| FR-43 | See when projected spending would leave fewer points than the end-of-quarter reserve goal. |

### Find and save

| ID | Requirement |
| --- | --- |
| FR-44 | Find available meal options at or below a chosen point limit and see their dining locations and point prices. |
| FR-45 | Save specific menu sections for quick access later and remove them from the saved list when desired. |
