/**
 * Dining Points Budget: email sender for sign-up, password reset and sign-in codes.
 *
 * Deploy from a Gmail account used only for this app. Setup steps are in README.md
 * under "Email codes".
 *
 * The web app's URL is public (it is built into the desktop app), so this script holds
 * no shared secret. Instead it only ever sends one fixed message containing a 6-digit
 * code, and limits how often it sends:
 *   - one email per address per minute, and MAX_EMAILS_PER_ADDRESS_PER_HOUR per hour;
 *   - MAX_EMAILS_PER_DAY in total, below Gmail's limit of about 100 a day.
 * The worst anyone else can do with the URL is send a few code emails.
 *
 * Request (POST, JSON): {"email", "code", "purpose": "verify" | "reset" | "signin"}
 * Reply (JSON): {"ok": true} or {"ok": false, "error": "..."}
 */

const MAX_EMAILS_PER_ADDRESS_PER_HOUR = 5;
const SECONDS_BETWEEN_EMAILS_TO_ONE_ADDRESS = 60;
const MAX_EMAILS_PER_DAY = 90;

const MESSAGES = {
  verify: ['Confirm your Dining Points Budget email',
           'Use this code to finish creating your account: '],
  reset: ['Your Dining Points Budget password reset code',
          'Use this code to reset your password: '],
  signin: ['Your Dining Points Budget sign-in code',
           'Use this code to finish signing in: '],
};

function doPost(e) {
  let body;
  try {
    body = JSON.parse(e.postData.contents);
  } catch (err) {
    return reply({ ok: false, error: 'bad request' });
  }

  const email = String(body.email || '').trim();
  const code = String(body.code || '');
  const message = MESSAGES[body.purpose];
  if (email.length > 254 || !/^[^@\s]+@[^@\s]+\.[^@\s.]+$/.test(email)
      || !/^\d{6}$/.test(code) || !message) {
    return reply({ ok: false, error: 'invalid request' });
  }

  // One request at a time, so two at once can't both slip under a limit.
  const lock = LockService.getScriptLock();
  if (!lock.tryLock(10000)) {
    return reply({ ok: false, error: 'busy' });
  }
  try {
    const cache = CacheService.getScriptCache();
    const address = email.toLowerCase();
    const recentKey = 'recent:' + address;
    const hourKey = 'hour:' + address;
    const sentThisHour = Number(cache.get(hourKey) || 0);
    if (cache.get(recentKey) || sentThisHour >= MAX_EMAILS_PER_ADDRESS_PER_HOUR) {
      return reply({ ok: false, error: 'too many emails' });
    }

    // Cache entries can be dropped early, so the daily total is kept in properties.
    const properties = PropertiesService.getScriptProperties();
    const dayKey = 'sent:' + Utilities.formatDate(new Date(), 'UTC', 'yyyy-MM-dd');
    const sentToday = Number(properties.getProperty(dayKey) || 0);
    if (sentToday >= MAX_EMAILS_PER_DAY) {
      return reply({ ok: false, error: 'daily limit reached' });
    }

    const [subject, intro] = message;
    const text = intro + code
      + '\n\nThe code expires in 15 minutes. If you did not ask for it, ignore this email.';
    MailApp.sendEmail({ to: email, subject: subject, body: text, name: 'Dining Points Budget' });

    cache.put(recentKey, '1', SECONDS_BETWEEN_EMAILS_TO_ONE_ADDRESS);
    cache.put(hourKey, String(sentThisHour + 1), 60 * 60);
    properties.setProperty(dayKey, String(sentToday + 1));
    removeOldDays(properties, dayKey);
    return reply({ ok: true });
  } catch (err) {
    return reply({ ok: false, error: 'could not send' });
  } finally {
    lock.releaseLock();
  }
}

// Keeps only today's counter in the script properties.
function removeOldDays(properties, todayKey) {
  Object.keys(properties.getProperties())
    .filter((key) => key.startsWith('sent:') && key !== todayKey)
    .forEach((key) => properties.deleteProperty(key));
}

function reply(result) {
  return ContentService.createTextOutput(JSON.stringify(result))
    .setMimeType(ContentService.MimeType.JSON);
}
