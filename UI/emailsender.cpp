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
    const QUrl url = EmailSender::url();
    if (!url.isValid() || url.scheme() != "https") {
        qWarning("EmailSender: sending is off or the url in %s is not https",
                 qPrintable(EmailSender::configPath()));
        return false;
    }

    const QJsonObject body{
        {"email", email},
        {"code", code},
        {"purpose", purpose == EmailSender::Purpose::PasswordReset ? "reset"
                    : purpose == EmailSender::Purpose::SignIn      ? "signin"
                                                                   : "verify"}};
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

QUrl url() {
    const QSettings settings(configPath(), QSettings::IniFormat);
    if (settings.contains("sender/url")) {
        return QUrl(settings.value("sender/url").toString().trimmed());
    }
    return QUrl(QString::fromLatin1(DefaultUrl));
}
}
