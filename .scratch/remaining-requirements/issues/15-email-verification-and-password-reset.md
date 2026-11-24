# 15: Email verification and password reset

**Owner:** P1

**Requirements:** FR-21 (forgotten password), FR-46

**Files:** `UI/emailsender.*`, `UI/authdialog.*`, `UI/localaccounts.*`, `apps-script/send-code.gs`, `.gitignore`, `tests/ui/email_code_test.cpp`

**What to build:** New accounts confirm their email with a 6-digit code before they can be used, and a forgotten password can be reset with a code sent by email. A Google Apps Script web app on a dedicated Gmail account sends the emails, the same `MailApp.sendEmail()` approach as the class activity.

**Blocked by:** 01 (SQLite storage)

**Status:** code done (branch `p1-storage`); deploying the Apps Script is a manual step in README "Email codes"

- [x] `apps-script/send-code.gs` accepts an email address, a code, and a shared secret, rejects requests with the wrong secret, and sends a short email. README explains how to deploy it
- [x] The web app URL and shared secret are read from a config file that is listed in `.gitignore`; the repo includes only an example file
- [x] At sign-up the app generates a code, stores only its hash with a 15-minute expiry, and sends it
- [x] The account can't sign in until the code is entered
- [x] **Resend code** sends a new code and invalidates the old one
- [x] "Forgot password?" sends a reset code, and the user can set a new password only with a valid code
- [x] If the sender can't be reached, the app shows "Couldn't send the code. Try again." and nothing is half-created
- [x] Tests use a fake sender (no real email) and cover a correct code, a wrong code, an expired code, and resending
