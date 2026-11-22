# 15: Dining card sync

**Requirements:** FR-34, FR-35

**What to build:** The user can connect a supported dining card so that balances, purchases, and refunds are imported automatically, and can disconnect it at any time. Manual entry keeps working. When an update fails, the user sees its status and the last successful update, and can retry without creating duplicate entries.

**Blocked by:** 09 (Refunds), 12 ("Last updated" indicator)

**Status:** ready-for-agent

- [ ] The team confirms a card source can actually be used (an SCU or card-provider data export or API). If none can, the team scopes this ticket down to importing a file and records that decision
- [ ] The user can connect and disconnect a card from the Account menu
- [ ] While connected, new balances, purchases, and refunds are imported and marked as imported
- [ ] Manual entry still works while connected
- [ ] A failed update shows an error status along with the last successful update time
- [ ] Retrying or importing again never duplicates a transaction or applies it to the balance twice; transactions are matched by their source ID
- [ ] Tests import the same data twice and confirm the balance and history are unchanged after the second import
