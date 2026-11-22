# 01: See pending outgoing requests and remove friends

**Requirements:** FR-27, FR-29

**What to build:** In the Friends dialog, the user can see every friend request they have sent that is still pending. They can also remove an accepted friend, after which neither person can see the other's dining-point balance.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] The Friends dialog has an "Outgoing requests" list showing the display name and friend ID of each pending request the user sent
- [ ] A sent request appears in the list right away and disappears once the other person accepts or declines it
- [ ] Each accepted friend can be selected and removed after a confirmation prompt
- [ ] After removal, the friend disappears from both users' friend lists and neither can see the other's balance
- [ ] After removal, either user can send a new friend request to the other
- [ ] Pending requests and removals are still in effect after signing out and back in (FR-23)
- [ ] Tests cover listing outgoing requests, removing a friend from both sides, and sending a new request after removal
