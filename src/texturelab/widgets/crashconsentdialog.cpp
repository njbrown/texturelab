#include "crashconsentdialog.h"

#include "telemetry.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

// Wide enough that the two fact lines wrap at most once, which is what keeps
// the block reading as a list rather than a paragraph.
constexpr int kDialogWidth = 460;
constexpr int kMargin = 28;
constexpr int kLeadColumn = 72;

} // namespace

CrashConsentDialog::CrashConsentDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Crash Reports"));
    setObjectName(QStringLiteral("ConsentDialog"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedWidth(kDialogWidth);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(kMargin, kMargin - 4, kMargin, kMargin - 6);
    layout->setSpacing(14);

    auto* title = new QLabel(tr("Help fix crashes"), this);
    title->setObjectName(QStringLiteral("ConsentTitle"));
    layout->addWidget(title);

    auto* body = new QLabel(
        tr("If TextureLab crashes, it can send a report so the bug can be found and fixed. "
           "Nothing is sent unless it crashes."),
        this);
    body->setObjectName(QStringLiteral("ConsentBody"));
    body->setWordWrap(true);
    layout->addWidget(body);

    auto* rule = new QFrame(this);
    rule->setObjectName(QStringLiteral("ConsentSeparator"));
    rule->setFrameShape(QFrame::HLine);
    layout->addWidget(rule);

    // The two halves of the answer to "what are you actually sending?", which
    // is the question the prompt exists to answer.
    auto* facts = new QVBoxLayout();
    facts->setSpacing(8);
    facts->addWidget(buildFact(
        tr("Sent"),
        tr("Where in the code the crash happened, TextureLab's version, and your "
           "operating system.")));
    facts->addWidget(buildFact(
        tr("Not sent"),
        tr("Your textures. No project files are uploaded, and there is no account or "
           "sign-in involved.")));
    layout->addLayout(facts);

    auto* provider = new QLabel(
        tr("Reports are handled by Sentry, a crash-reporting service. You can change this "
           "any time from the ⚙ menu."),
        this);
    provider->setObjectName(QStringLiteral("ConsentNote"));
    provider->setWordWrap(true);
    layout->addWidget(provider);

    layout->addSpacing(2);

    auto* buttons = new QHBoxLayout();
    buttons->setSpacing(8);
    buttons->addStretch(1);

    auto* decline = new QPushButton(tr("Not now"), this);
    decline->setCursor(Qt::PointingHandCursor);
    connect(decline, &QPushButton::clicked, this, &QDialog::reject);
    buttons->addWidget(decline);

    auto* accept = new QPushButton(tr("Send crash reports"), this);
    accept->setProperty("variant", "primary");
    accept->setCursor(Qt::PointingHandCursor);
    accept->setDefault(true);
    connect(accept, &QPushButton::clicked, this, &QDialog::accept);
    buttons->addWidget(accept);

    layout->addLayout(buttons);

    // Word-wrapped labels only know their height once they know their width, so
    // the height has to be pinned after the fixed width is in effect.
    layout->activate();
    setFixedHeight(sizeHint().height());
}

void CrashConsentDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    // Centred by hand rather than left to QDialog. Its own placement runs off
    // the parent's geometry at construction, and on a multi-monitor X11 desktop
    // the launcher has not been placed by the window manager that early — the
    // prompt then lands centred on where the launcher was going to be, which
    // can be a different screen from where it ended up.
    if (const QWidget* owner = parentWidget() ? parentWidget()->window() : nullptr) {
        const QRect area = owner->frameGeometry();
        move(area.center() - QPoint(width() / 2, height() / 2));
    }
}

QWidget* CrashConsentDialog::buildFact(const QString& lead, const QString& detail)
{
    auto* row = new QWidget(this);

    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* leadLabel = new QLabel(lead, row);
    leadLabel->setObjectName(QStringLiteral("ConsentFactLead"));
    leadLabel->setFixedWidth(kLeadColumn);
    // Top-aligned so a lead word stays level with the first line of a detail
    // that wraps, rather than drifting to the middle of the block.
    leadLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    layout->addWidget(leadLabel);

    auto* detailLabel = new QLabel(detail, row);
    detailLabel->setObjectName(QStringLiteral("ConsentFact"));
    detailLabel->setWordWrap(true);
    detailLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    layout->addWidget(detailLabel, 1);

    return row;
}

bool CrashConsentDialog::ask(QWidget* parent)
{
    CrashConsentDialog dialog(parent);
    const bool allowed = dialog.exec() == QDialog::Accepted;

    Telemetry::recordConsent(allowed);
    Telemetry::setEnabled(allowed);
    return allowed;
}
