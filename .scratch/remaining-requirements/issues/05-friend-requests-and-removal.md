# 05: Pending requests and removing friends

**Owner:** P4

**Requirements:** FR-27, FR-29

**Files:** `UI/friendsdialog.*`, `tests/ui/friends_dialog_test.cpp`

**What to build:** In the Friends dialog, the user can see the requests they've sent that are still pending, and can remove a friend so that neither person sees the other's balance anymore.

**Blocked by:** 01 (SQLite storage), but the UI can be built against the stubs from 00

**Status:** ready-for-agent

- [ ] An "Outgoing requests" list shows the display name and friend ID of each pending request the user has sent
- [ ] A request leaves the list once the other person accepts or declines it
- [ ] Selecting a friend and choosing Remove, after a confirmation, removes the friendship for both people
- [ ] After removal, either person can send a new request
- [ ] Tests cover listing outgoing requests, removing a friend from both sides, and sending a new request after removal
