#pragma once

#include <string>

namespace Telemetry {

// Call before QApplication. No-op if DSN is empty or user opted out.
void init(bool enabled);

// Call after a.exec() returns to flush queued events.
void close();

// Add a breadcrumb (category + message) to the current session context.
void breadcrumb(const char* category, const std::string& message);

// Capture a handled exception (e.g. from a catch block) as a Sentry error event.
void captureException(const std::string& message);

} // namespace Telemetry
