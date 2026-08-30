#pragma once

#include <string>

namespace Telemetry {

// Call before QApplication. No-op if DSN is empty or user opted out.
void init(bool enabled);

// Call after a.exec() returns to flush queued events.
void close();

// Turns collection on or off after startup, which is what the consent prompt
// needs: answering it has to take effect now, not on the next launch.
void setEnabled(bool enabled);

// Whether crash reporting is currently running.
bool isEnabled();

// The user's stored answer. Off until they have actually been asked — the
// absence of an answer is not consent.
bool isAllowed();

// True when the consent prompt is owed. Asked once per released version, so a
// build that changes what gets collected gets a fresh answer rather than
// inheriting one given about an older one. The build hash is deliberately not
// part of the comparison, or every dev build would re-ask.
bool consentNeeded();

// Stores the answer, and stamps the version it was given about.
void recordConsent(bool allowed);

// Add a breadcrumb (category + message) to the current session context.
void breadcrumb(const char* category, const std::string& message);

// Capture a handled exception (e.g. from a catch block) as a Sentry error event.
void captureException(const std::string& message);

} // namespace Telemetry
