#pragma once

#include <QDateTime>

namespace UndoCommandId {
static constexpr int PropertyChange = 1;
static constexpr int MoveItems      = 2; // unused: moves never merge
static constexpr int EditFrame      = 3;
static constexpr int EditComment    = 4;
static constexpr int GlobalSeed     = 5;
static constexpr int NodeSeed       = 6;
static constexpr int VariablesChange = 7;
} // namespace UndoCommandId

// Mergeable commands (slider scrubs, typing) only merge with the previous
// command if it was pushed within this window. Otherwise two separate scrubs
// of the same slider collapse into one undo step.
namespace UndoMerge {
static constexpr qint64 WindowMs = 1000;

inline qint64 now() { return QDateTime::currentMSecsSinceEpoch(); }

inline bool withinWindow(qint64 previous, qint64 next)
{
    return next - previous <= WindowMs;
}
} // namespace UndoMerge
