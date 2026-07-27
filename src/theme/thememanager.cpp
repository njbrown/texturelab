#include "thememanager.h"

#include "qssbuilder.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTimer>

#include <cstdio>

namespace {

// Resolve a JSON string value that may be an "@ref" pointing at another key in
// `colors`. Follows a chain of references with a guard against cycles.
QColor resolveColor(const QString& raw, const QJsonObject& colors)
{
    QString v = raw;
    int guard = 0;
    while (v.startsWith('@')) {
        if (++guard > 32) {
            qWarning("ThemeManager: color reference cycle at '%s'", qPrintable(raw));
            return QColor();
        }
        const QString ref = v.mid(1);
        if (!colors.contains(ref)) {
            qWarning("ThemeManager: dangling color reference '@%s'", qPrintable(ref));
            return QColor();
        }
        v = colors.value(ref).toString();
    }
    QColor c(v);
    if (!c.isValid())
        qWarning("ThemeManager: invalid color literal '%s'", qPrintable(v));
    return c;
}

} // namespace

ThemeManager& ThemeManager::instance()
{
    static ThemeManager s_instance;
    return s_instance;
}

bool ThemeManager::loadFromResource(const QString& resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("ThemeManager: cannot open theme '%s'", qPrintable(resourcePath));
        return false;
    }

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning("ThemeManager: JSON parse error in '%s': %s", qPrintable(resourcePath),
                 qPrintable(err.errorString()));
        return false;
    }

    const QJsonObject root = doc.object();
    const QJsonObject colors = root.value("color").toObject();

    Theme t;
    t.m_name = root.value("meta").toObject().value("name").toString();

    // --- colors (resolve @refs) ---
    for (auto it = colors.constBegin(); it != colors.constEnd(); ++it) {
        const QColor c = resolveColor(it.value().toString(), colors);
        t.m_colors.insert(it.key(), c);
        t.m_qssVars.insert(it.key(), c.name(QColor::HexRgb)); // "#rrggbb"
    }

    // --- scalar groups: radius / space / motion ---
    const auto loadInts = [&](const char* group) {
        const QJsonObject obj = root.value(group).toObject();
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            const int val = it.value().toInt();
            const QString flatKey = QString("%1.%2").arg(group, it.key());
            t.m_ints.insert(flatKey, val);
            t.m_qssVars.insert(flatKey, QString::number(val));
        }
    };
    loadInts("radius");
    loadInts("space");
    loadInts("motion");

    // --- fonts ---
    const QJsonObject fonts = root.value("font").toObject();
    for (auto it = fonts.constBegin(); it != fonts.constEnd(); ++it) {
        const QJsonObject f = it.value().toObject();
        const QString family = f.value("family").toString();
        const int size = f.value("size").toInt(12);
        const int weight = f.value("weight").toInt(400);

        QFont font;
        // Family may be a CSS-style fallback list; take the first as the primary
        // and register the rest as substitute candidates via setFamilies.
        QStringList families;
        for (const QString& part : family.split(','))
            families << part.trimmed();
        if (!families.isEmpty()) {
            font.setFamily(families.first());
            font.setFamilies(families);
        }
        font.setPixelSize(size);
        font.setWeight(QFont::Weight(weight));
        t.m_fonts.insert(it.key(), font);

        t.m_qssVars.insert(QString("font.%1.family").arg(it.key()), family);
        t.m_qssVars.insert(QString("font.%1.size").arg(it.key()), QString::number(size));
        t.m_qssVars.insert(QString("font.%1.weight").arg(it.key()), QString::number(weight));
    }

    // --- palette role map ---
    const QJsonObject palette = root.value("palette").toObject();
    for (auto it = palette.constBegin(); it != palette.constEnd(); ++it) {
        t.m_paletteColors.insert(it.key(), resolveColor(it.value().toString(), colors));
    }

    m_theme = t;
    return true;
}

void ThemeManager::setStyleSheetTemplate(const QString& resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("ThemeManager: cannot open QSS template '%s'", qPrintable(resourcePath));
        m_qssTemplate.clear();
        return;
    }
    m_qssTemplate = QString::fromUtf8(file.readAll());
}

QPalette ThemeManager::buildPalette() const
{
    const QHash<QString, QColor>& p = m_theme.paletteColors();
    const auto col = [&](const char* role, QColor fallback) {
        return p.value(QString::fromLatin1(role), fallback);
    };

    QPalette pal;
    pal.setColor(QPalette::Window, col("window", QColor(53, 53, 53)));
    pal.setColor(QPalette::WindowText, col("windowText", Qt::white));
    pal.setColor(QPalette::Base, col("base", QColor(35, 35, 35)));
    pal.setColor(QPalette::AlternateBase, col("alternateBase", QColor(53, 53, 53)));
    pal.setColor(QPalette::ToolTipBase, col("toolTipBase", QColor(25, 25, 25)));
    pal.setColor(QPalette::ToolTipText, col("toolTipText", Qt::white));
    pal.setColor(QPalette::Text, col("text", Qt::white));
    pal.setColor(QPalette::Button, col("button", QColor(53, 53, 53)));
    pal.setColor(QPalette::ButtonText, col("buttonText", Qt::white));
    pal.setColor(QPalette::BrightText, col("brightText", Qt::red));
    pal.setColor(QPalette::Link, col("link", QColor(42, 130, 218)));
    pal.setColor(QPalette::Highlight, col("highlight", QColor(42, 130, 218)));
    pal.setColor(QPalette::HighlightedText, col("highlightedText", Qt::black));

    const QColor disabled = col("disabled.text", QColor(127, 127, 127));
    pal.setColor(QPalette::Disabled, QPalette::WindowText,
                 col("disabled.windowText", disabled));
    pal.setColor(QPalette::Disabled, QPalette::Text, disabled);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText,
                 col("disabled.buttonText", disabled));
    pal.setColor(QPalette::Disabled, QPalette::HighlightedText,
                 col("disabled.highlightedText", disabled));
    pal.setColor(QPalette::Disabled, QPalette::Highlight,
                 col("disabled.highlight", QColor(80, 80, 80)));
    return pal;
}

void ThemeManager::applyToApplication(QApplication& app)
{
    m_app = &app;

    app.setStyle(QStyleFactory::create("Fusion"));
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif

    reapply();
}

void ThemeManager::reapply()
{
    if (!m_app)
        return;

    m_app->setPalette(buildPalette());
    m_app->setStyleSheet(QssBuilder::build(m_qssTemplate, m_theme));

    emit themeChanged();
}

void ThemeManager::enableHotReload(const QString& themeFilePath, const QString& qssFilePath)
{
    m_themePath = themeFilePath;
    m_qssPath = qssFilePath;

    if (!m_watcher) {
        m_watcher = new QFileSystemWatcher(this);

        // Coalesce bursts of change events (editors often fire several per save)
        // into a single reload.
        m_reloadTimer = new QTimer(this);
        m_reloadTimer->setSingleShot(true);
        m_reloadTimer->setInterval(120);
        connect(m_reloadTimer, &QTimer::timeout, this, &ThemeManager::reloadFromDisk);
        connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
                [this](const QString&) { m_reloadTimer->start(); });
    }

    // Initial load from the on-disk source, then start watching.
    reloadFromDisk();
}

void ThemeManager::reloadFromDisk()
{
    // QFile handles plain filesystem paths as well as ":/..." resources.
    const bool ok = loadFromResource(m_themePath); // keeps previous theme if parse fails
    setStyleSheetTemplate(m_qssPath);
    reapply();

    // --dev-theme feedback. Use fprintf, NOT qInfo/qWarning: the app installs a
    // custom Qt message handler that routes logging to Sentry breadcrumbs, which
    // would swallow this and defeat the point of a live-tuning loop.
    std::fprintf(stderr, "[theme] %s: reloaded from %s\n",
                 ok ? "ok" : "FAILED (kept previous theme)", qPrintable(m_themePath));
    std::fflush(stderr);

    // Many editors save by writing a temp file and renaming over the original,
    // which deletes the inode QFileSystemWatcher was tracking and silently drops
    // the watch. Re-add any path the watcher is no longer following.
    if (m_watcher) {
        const QStringList watched = m_watcher->files();
        for (const QString& p : { m_themePath, m_qssPath }) {
            if (!watched.contains(p) && QFile::exists(p))
                m_watcher->addPath(p);
        }
    }
}
