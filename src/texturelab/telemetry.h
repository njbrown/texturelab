#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Telemetry {

// Structured payload attached to a breadcrumb, context or event. Deliberately
// Qt-free and sentry-free so this header stays cheap to include: the mapping
// onto sentry_value_t lives in telemetry.cpp.
using FieldValue = std::variant<std::string, int64_t, double, bool>;
using Fields = std::vector<std::pair<std::string, FieldValue>>;

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
//
// With the Crashpad backend every breadcrumb flushes the scope to disk so it
// survives a hard crash — which is the point, but it also means these belong at
// user-action granularity. Do not add per-node or per-frame breadcrumbs.
void breadcrumb(const char* category, const std::string& message);
void breadcrumb(const char* category, const std::string& message,
                const Fields& data);

// Set a searchable tag on every subsequent event. Cheap enough to keep current
// as session state changes (resolution, node count), which is what makes a
// crash event self-describing.
void setTag(const char* key, const std::string& value);

// Set a structured context block (Sentry's "gpu", "device", "app", ...).
void setContext(const char* name, const Fields& fields);

// Capture a handled exception (e.g. from a catch block) as a Sentry error event.
void captureException(const std::string& message);
void captureException(const std::string& message, const Fields& data);

} // namespace Telemetry
