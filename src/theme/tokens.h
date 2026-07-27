#pragma once

// Canonical token keys. Paint code (node graph, viewports, custom widgets) and
// palette-building should reference these constants instead of hardcoding the
// dotted strings, so a rename is a compile-time break rather than a silent miss.
//
// The string values MUST match the keys under "color" in the theme JSON
// (resources/themes/*.json).
namespace Tokens {

// --- semantic roles (surface A/B — general chrome) ---
constexpr const char* BgWindow      = "bg.window";
constexpr const char* BgPanel       = "bg.panel";
constexpr const char* BgBase        = "bg.base";
constexpr const char* BgElevated    = "bg.elevated";
constexpr const char* BorderSubtle  = "border.subtle";
constexpr const char* BorderStrong  = "border.strong";
constexpr const char* TextPrimary   = "text.primary";
constexpr const char* TextSecondary = "text.secondary";
constexpr const char* TextDisabled  = "text.disabled";
constexpr const char* Selection     = "selection";
constexpr const char* Accent        = "accent";

// --- node graph (surface C) ---
constexpr const char* NodeBg          = "node.bg";
constexpr const char* NodeBorder      = "node.border";
constexpr const char* NodeBorderHover = "node.border.hover";
constexpr const char* NodeBorderSelect = "node.border.select";
constexpr const char* NodeTitle       = "node.title";
constexpr const char* NodeChannel     = "node.channel";
constexpr const char* SocketFill      = "socket.fill";
constexpr const char* Wire            = "wire";
constexpr const char* WireDragging    = "wire.dragging";
constexpr const char* WireSelected    = "wire.selected";
constexpr const char* GridBg          = "grid.bg";
constexpr const char* GridFine        = "grid.fine";
constexpr const char* GridCoarse      = "grid.coarse";
constexpr const char* CheckerA        = "checker.a";
constexpr const char* CheckerB        = "checker.b";
constexpr const char* FrameSelect     = "frame.select";
constexpr const char* CommentFill     = "comment.fill";
constexpr const char* CommentText     = "comment.text";

// --- 3D viewport (surface D) ---
constexpr const char* View3dClear = "view3d.clear";
constexpr const char* View3dGrid  = "view3d.grid";

// --- curve editor (surface B) ---
constexpr const char* CurveBg           = "curve.bg";
constexpr const char* CurveGrid         = "curve.grid";
constexpr const char* CurveIdentity     = "curve.identity";
constexpr const char* CurveLine         = "curve.line";
constexpr const char* CurveAnchor       = "curve.anchor";
constexpr const char* CurveAnchorHover  = "curve.anchor.hover";
constexpr const char* CurveAnchorSelect = "curve.anchor.select";
constexpr const char* CurveHandleLine   = "curve.handle.line";
constexpr const char* CurveHandleDot    = "curve.handle.dot";
constexpr const char* CurveHandleHover  = "curve.handle.hover";
constexpr const char* CurveHandleCorner = "curve.handle.corner";

} // namespace Tokens
