#ifndef EMAILSENDER_H
#define EMAILSENDER_H

#include <QString>

#include <functional>

// Sends sign-up verification, password reset and two-step sign-in codes by email.
//
// The real sender posts to a Google Apps Script web app (apps-script/send-code.gs),
// whose URL and shared secret are read from an INI file kept out of git:
//
//   [sender]
//   url=https://script.google.com/macros/s/.../exec
//   secret=the-same-secret-as-the-script-property
//
// The file is `email-sender.ini` in the app's local data folder, or the path in the
// DINING_POINTS_EMAIL_CONFIG environment variable. See email-sender.example.ini.
namespace EmailSender {
enum class Purpose { Verification, PasswordReset, SignIn };

// Returns true once the email has been handed to the sender.
using SendFunction = std::function<bool(const QString &email, const QString &code, Purpose)>;

// Sends one code. Uses the function from setSender() if one is installed, otherwise
// the Apps Script web app. Blocks for up to TimeoutMs while waiting for the reply.
bool send(const QString &email, const QString &code, Purpose purpose);

// Replaces the sender, for tests. Pass an empty function to restore the real one.
void setSender(SendFunction sender);

// Where the real sender looks for its settings.
QString configPath();

constexpr int TimeoutMs = 15000;
}

#endif // EMAILSENDER_H
