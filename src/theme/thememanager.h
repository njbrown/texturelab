#pragma once

#include "theme.h"

#include <QObject>
#include <QPalette>
#include <QString>

class QApplication;
class QFileSystemWatcher;
class QTimer;

// Owns the active Theme and applies it to the application. Singleton so paint
// code anywhere can read the current theme and subscribe to themeChanged().
//
// Phase 0: loads a JSON theme, builds a QPalette identical to the previous
// hand-coded applyDarkTheme(), applies Fusion + palette + (empty) stylesheet.
// Later phases fill in app.qss and thread tokens into custom paint code.
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    static ThemeManager& instance();

    // Load + resolve a theme JSON (e.g. ":/themes/dark.json"). Returns false and
    // keeps the previous theme on parse failure. Does not apply on its own.
    bool loadFromResource(const QString& resourcePath);

    // Load the QSS template (e.g. ":/qss/app.qss"). Kept separately so it can be
    // re-read on hot-reload without re-parsing the theme.
    void setStyleSheetTemplate(const QString& resourcePath);

    // Load the dock-system (ADS) QSS override template. Applied by MainWindow to
    // the CDockManager, not to qApp (ADS sets its own sheet on the manager). Kept
    // here so it participates in token substitution and hot-reload.
    void setAdsStyleSheetTemplate(const QString& resourcePath);

    // Token-substituted ADS override stylesheet. MainWindow appends this to ADS's
    // own default sheet. Rebuilds from the current theme on each call.
    QString adsStyleSheet() const;

    // The built application stylesheet (same one applied to qApp). MainWindow also
    // appends this to the dock-manager sheet: Qt prefers an ancestor widget's
    // stylesheet over qApp, so without this the app rules don't reach widgets
    // living inside ADS docks (e.g. the properties panel). Rebuilds each call.
    QString appStyleSheet() const;

    const Theme& theme() const { return m_theme; }

    // Apply Fusion style, dark color scheme, the built QPalette, and the built
    // stylesheet to the given application. Emits themeChanged().
    void applyToApplication(QApplication& app);

    // Rebuild + reapply palette/stylesheet from the current theme (used after a
    // hot-reload). No-op if applyToApplication() was never called.
    void reapply();

    QPalette buildPalette() const;

    // Dev convenience: watch the on-disk *source* theme + QSS files and reload
    // live on save (no rebuild needed). Pass real filesystem paths, not ":/..."
    // resource paths — the compiled-in resources can't be watched. Does an
    // initial load from those paths, so in dev the disk files win over the qrc.
    void enableHotReload(const QString& themeFilePath, const QString& qssFilePath,
                         const QString& adsFilePath);

signals:
    void themeChanged();

private:
    ThemeManager() = default;

    void reloadFromDisk();

    Theme m_theme;
    QString m_qssTemplate;    // raw app template text (with {{tokens}})
    QString m_adsTemplate;    // raw dock-system (ADS) override template
    QApplication* m_app = nullptr;

    // hot-reload (dev only; null unless enableHotReload() was called)
    QFileSystemWatcher* m_watcher = nullptr;
    QTimer* m_reloadTimer = nullptr;
    QString m_themePath;
    QString m_qssPath;
    QString m_adsPath;
};
