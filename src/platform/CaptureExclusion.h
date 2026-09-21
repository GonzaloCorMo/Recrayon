#pragma once

class QWindow;

namespace recrayon::platform {

/// Hides @p window from screenshots and screen recordings (ours and third-party tools) where
/// the OS supports it. Used for the toolbar so it never shows up in captures.
///
/// Returns false when the platform has no such feature (the window stays capturable).
/// Windows 10 2004+: SetWindowDisplayAffinity(WDA_EXCLUDEFROMCAPTURE).
bool setExcludedFromCapture(QWindow* window, bool excluded);

} // namespace recrayon::platform
