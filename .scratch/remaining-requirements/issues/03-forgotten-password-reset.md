# 03: Reset a forgotten password

**Requirements:** FR-21 (resetting a forgotten password)

**What to build:** From the sign-in screen, a user who has forgotten their password can reset it after proving who they are through a recovery method linked to their account.

**Blocked by:** 02 (Account page; the email address is entered there)

**Status:** ready-for-agent

- [ ] The sign-in screen has a "Forgot password?" link
- [ ] The team has chosen the recovery method and written the decision in SPECIFICATIONS.md (for example, a one-time code sent to the saved email address, or security questions answered during sign-up)
- [ ] The user can set a new password only after completing verification
- [ ] An account with no recovery method explains how to recover the account instead of failing silently
- [ ] Tests cover a successful reset, a wrong or expired code or answer, and an account with no recovery method
