/**
 * Dining Points Budget: email sender for sign-up, password reset and sign-in codes.
 *
 * Deploy from a Gmail account used only for this app. Setup steps are in README.md
 * under "Email codes". The shared secret is NOT written in this file: set it as the
 * script property SHARED_SECRET (Project Settings > Script properties), and put the
 * same value in the app's email-sender.ini.
 *
 * Request (POST, JSON): {"secret", "email", "code", "purpose": "verify" | "reset" | "signin"}
 * Reply (JSON): {"ok": true} or {"ok": false, "error": "..."}
 */

// At most this many emails to one address per hour, to stop the web app being used
// to flood someone's inbox. Apps Script also limits a Gmail account to about 100
// emails a day.
const MAX_EMAILS_PER_ADDRESS_PER_HOUR = 5;

function doPost(e) {
  try {
    const body = JSON.parse(e.postData.contents);
    const secret = PropertiesService.getScriptProperties().getProperty('SHARED_SECRET');
    if (!secret || body.secret !== secret) {
      return reply({ ok: false, error: 'unauthorized' });
    }

    const email = String(body.email || '').trim();
    const code = String(body.code || '');
    if (!/^[^@\s]+@[^@\s]+\.[^@\s.]+$/.test(email) || !/^\d{6}$/.test(code)) {
      return reply({ ok: false, error: 'invalid request' });
    }

    const cache = CacheService.getScriptCache();
    const key = 'sent:' + email.toLowerCase();
    const sent = Number(cache.get(key) || 0);
    if (sent >= MAX_EMAILS_PER_ADDRESS_PER_HOUR) {
      return reply({ ok: false, error: 'too many emails' });
    }

    const messages = {
      verify: ['Confirm your Dining Points Budget email',
               'Use this code to finish creating your account: '],
      reset: ['Your Dining Points Budget password reset code',
              'Use this code to reset your password: '],
      signin: ['Your Dining Points Budget sign-in code',
               'Use this code to finish signing in: '],
    };
    const [subject, intro] = messages[body.purpose] || messages.verify;
    const text = intro + code
      + '\n\nThe code expires in 15 minutes. If you did not ask for it, ignore this email.';
    MailApp.sendEmail({ to: email, subject: subject, body: text, name: 'Dining Points Budget' });

    cache.put(key, String(sent + 1), 60 * 60);
    return reply({ ok: true });
  } catch (err) {
    return reply({ ok: false, error: 'bad request' });
  }
}

function reply(result) {
  return ContentService.createTextOutput(JSON.stringify(result))
    .setMimeType(ContentService.MimeType.JSON);
}
