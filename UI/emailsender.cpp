#include "emailsender.h"

#include <QDir>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

#include <utility>

namespace {
EmailSender::SendFunction &installedSender() {
    static EmailSender::SendFunction sender;
    return sender;
}

// Posts the code to the Apps Script web app and waits for {"ok": true}.
bool sendWithAppsScript(const QString &email, const QString &code, EmailSender::Purpose purpose) {
    const QSettings settings(EmailSender::configPath(), QSettings::IniFormat);
    const QUrl url(settings.value("sender/url").toString());
    const QString secret = settings.value("sender/secret").toString();
    if (!url.isValid() || url.scheme() != "https" || secret.isEmpty()) {
        qWarning("EmailSender: no valid url and secret in %s", qPrintable(EmailSender::configPath()));
        return false;
    }

    const QJsonObject body{
        {"secret", secret},
        {"email", email},
        {"code", code},
        {"purpose", purpose == EmailSender::Purpose::PasswordReset ? "reset" : "verify"}};
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // Apps Script answers through a redirect to googleusercontent.com.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(EmailSender::TimeoutMs);

    QNetworkAccessManager network;
    QNetworkReply *reply = network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(EmailSender::TimeoutMs, &loop, &QEventLoop::quit);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
    }
    const bool ok = reply->error() == QNetworkReply::NoError
        && QJsonDocument::fromJson(reply->readAll()).object().value("ok").toBool();
    if (!ok) {
        qWarning("EmailSender: the email sender did not confirm: %s",
                 qPrintable(reply->errorString()));
    }
    reply->deleteLater();
    return ok;
}
}

namespace EmailSender {
bool send(const QString &email, const QString &code, Purpose purpose) {
    const SendFunction &sender = installedSender();
    return sender ? sender(email, code, purpose) : sendWithAppsScript(email, code, purpose);
}

void setSender(SendFunction sender) {
    installedSender() = std::move(sender);
}

QString configPath() {
    const QString overridePath = qEnvironmentVariable("DINING_POINTS_EMAIL_CONFIG");
    if (!overridePath.isEmpty()) {
        return overridePath;
    }
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
        .filePath("email-sender.ini");
}
}
