#pragma once

#include <QKeySequence>
#include <QObject>

#include <memory>

namespace recrayon {

/// System-wide keyboard shortcuts that fire even when another application has focus.
///
/// Qt has no portable API for this, so each OS gets its own backend. create() returns the
/// backend for the current platform, or a null backend whose isSupported() is false.
class GlobalHotkeys : public QObject {
    Q_OBJECT

public:
    [[nodiscard]] static std::unique_ptr<GlobalHotkeys> create();

    ~GlobalHotkeys() override;

    [[nodiscard]] virtual bool isSupported() const = 0;

    /// Registers the first chord of @p sequence under @p id. Returns false if the key cannot be
    /// mapped or the OS refused (typically: another application already owns the combination).
    virtual bool registerHotkey(int id, const QKeySequence& sequence) = 0;

    virtual void unregisterAll() = 0;

signals:
    void activated(int id);

protected:
    GlobalHotkeys();
};

} // namespace recrayon
