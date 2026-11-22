# 02: Account page with user ID, profile editing, and password change

**Requirements:** FR-19, FR-20, FR-21 (changing the password while signed in)

**What to build:** An Account page or dialog, opened from the Account menu, where the signed-in user can see their user ID, change their display name and email address, and change their password.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] The page shows the signed-in username (user ID) as read-only text
- [ ] The user can change their display name; friends see the new name the next time they refresh
- [ ] The user can add or change their email address; an address that is not a valid email is rejected with a clear message
- [ ] Changing the password requires the current password and a new password entered twice; a wrong current password, a new password that is too short, or two entries that don't match are each rejected with a clear message
- [ ] After a password change, the old password no longer works and the new one does
- [ ] Profile changes are still there after signing out and back in (FR-23)
- [ ] Tests cover profile updates, email validation, and each password-change success and failure case
