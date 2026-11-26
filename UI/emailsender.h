#ifndef EMAILSENDER_H
#define EMAILSENDER_H

#include <QString>
#include <QUrl>

#include <functional>

// Sends sign-up verification, password reset and two-step sign-in codes by email.
//
// The real sender posts to a Google Apps Script web app (apps-script/send-code.gs)
// that sends the email from the team's Gmail account. Its URL is built in, so the app
// works on any computer with no setup. The app holds no secret: the Gmail access stays
// on Google's servers, and the script limits how many emails it sends.
//
// To use a different deployment, put an INI file at `email-sender.ini` in the app's
// local data folder, or at the path in DINING_POINTS_EMAIL_CONFIG:
//
//   [sender]
//   url=https://script.google.com/macros/s/.../exec
//
// An empty `url=` turns sending off.
namespace EmailSender {
enum class Purpose { Verification, PasswordReset, SignIn };

// Returns true once the email has been handed to the sender.
using SendFunction = std::function<bool(const QString &email, const QString &code, Purpose)>;

// Sends one code. Uses the function from setSender() if one is installed, otherwise
// the Apps Script web app. Blocks for up to TimeoutMs while waiting for the reply.
bool send(const QString &email, const QString &code, Purpose purpose);

// Replaces the sender, for tests. Pass an empty function to restore the real one.
void setSender(SendFunction sender);

// Where the real sender looks for an optional settings file.
QString configPath();

// The web app the real sender posts to: the settings file's url if it has one,
// otherwise DefaultUrl. Empty when the settings file turns sending off.
QUrl url();

constexpr char DefaultUrl[] =
    "https://script.google.com/macros/s/AKfycbwhc1UJliteH-c7c6jPYMhDJqEdFRTLVjWoRcuGFTmxC0dbm"
    "KmYYbCWOfwvs9Z8GvMB5A/exec";
constexpr int TimeoutMs = 15000;
}

#endif // EMAILSENDER_H
