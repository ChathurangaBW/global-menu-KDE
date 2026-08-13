// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

#include "desktopfallback.h"

#include <KLocalizedString>

#include <QAction>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

namespace
{
QString normalizedDesktopFileName(QString name)
{
    name = name.trimmed();
    if (!name.endsWith(QStringLiteral(".desktop"), Qt::CaseInsensitive)) {
        name += QStringLiteral(".desktop");
    }
    return name;
}

bool writeNewFile(const QString &path, const QByteArray &contents, QString *errorMessage)
{
    if (QFileInfo::exists(path)) {
        if (errorMessage) {
            *errorMessage = i18n("An item named %1 already exists.", QFileInfo(path).fileName());
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    if (file.write(contents) != contents.size()) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        file.remove();
        return false;
    }
    return true;
}

QString desktopEntryValue(QString value)
{
    value.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    value.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    return value;
}

class DefaultDesktopFallbackActionRunner final : public DesktopFallbackActionRunner
{
public:
    bool programAvailable(const QString &program) const override
    {
        return !QStandardPaths::findExecutable(program).isEmpty();
    }

    void openUrl(const QUrl &url) override
    {
        QDesktopServices::openUrl(url);
    }

    void startProgram(const QString &program, const QStringList &arguments) override
    {
        const QString executable = QStandardPaths::findExecutable(program);
        if (!executable.isEmpty()) {
            QProcess::startDetached(executable, arguments);
        }
    }

    bool restartShellAvailable() const override
    {
        const QString systemctl = QStandardPaths::findExecutable(QStringLiteral("systemctl"));
        if (!systemctl.isEmpty() && QProcess::execute(systemctl, {QStringLiteral("--user"), QStringLiteral("cat"), QStringLiteral("plasma-plasmashell.service")}) == 0) {
            return true;
        }
        return programAvailable(QStringLiteral("plasmashell"));
    }

    void restartShell() override
    {
        const QString systemctl = QStandardPaths::findExecutable(QStringLiteral("systemctl"));
        if (!systemctl.isEmpty() && QProcess::execute(systemctl, {QStringLiteral("--user"), QStringLiteral("cat"), QStringLiteral("plasma-plasmashell.service")}) == 0) {
            QProcess::startDetached(systemctl, {QStringLiteral("--user"), QStringLiteral("restart"), QStringLiteral("plasma-plasmashell.service")});
            return;
        }
        const QString plasmashell = QStandardPaths::findExecutable(QStringLiteral("plasmashell"));
        if (!plasmashell.isEmpty()) {
            QProcess::startDetached(plasmashell, {QStringLiteral("--replace")});
        }
    }

    void callSessionBus(const QString &service,
                        const QString &path,
                        const QString &interface,
                        const QString &method,
                        const QVariantList &arguments) override
    {
        QDBusMessage message = QDBusMessage::createMethodCall(service, path, interface, method);
        message.setArguments(arguments);
        QDBusConnection::sessionBus().asyncCall(message);
    }

    bool confirm(QWidget *parent, const QString &title, const QString &text) override
    {
        return QMessageBox::question(parent, title, text, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes;
    }

    void createDesktopItem(DesktopCreateKind kind, const QString &desktopDirectory, QWidget *parent) override
    {
        QDir desktop(desktopDirectory);
        if (!desktop.exists() && !desktop.mkpath(QStringLiteral("."))) {
            QMessageBox::warning(parent, i18n("Create New"), i18n("The Desktop folder is not available."));
            return;
        }

        QString error;
        if (kind == DesktopCreateKind::Folder || kind == DesktopCreateKind::TextFile || kind == DesktopCreateKind::HtmlFile) {
            QString defaultName;
            QString title;
            if (kind == DesktopCreateKind::Folder) {
                defaultName = i18n("New Folder");
                title = i18n("Create New Folder");
            } else if (kind == DesktopCreateKind::HtmlFile) {
                defaultName = i18n("New HTML File.html");
                title = i18n("Create New HTML File");
            } else {
                defaultName = i18n("New Text File.txt");
                title = i18n("Create New Text File");
            }

            bool accepted = false;
            const QString name = QInputDialog::getText(parent, title, i18n("Name:"), QLineEdit::Normal, defaultName, &accepted);
            if (!accepted) {
                return;
            }

            if (kind == DesktopCreateKind::Folder) {
                if (!createEmptyDesktopItem(desktopDirectory, name, true, &error)) {
                    QMessageBox::warning(parent, title, error);
                }
                return;
            }

            if (!isSafeDesktopItemName(name)) {
                QMessageBox::warning(parent, title, i18n("Enter a name without path separators."));
                return;
            }
            const QByteArray contents = kind == DesktopCreateKind::HtmlFile
                ? QByteArrayLiteral("<!doctype html>\n<html>\n<head>\n  <meta charset=\"utf-8\">\n  <title></title>\n</head>\n<body>\n</body>\n</html>\n")
                : QByteArray();
            if (!writeNewFile(desktop.filePath(name.trimmed()), contents, &error)) {
                QMessageBox::warning(parent, title, error);
            }
            return;
        }

        if (kind == DesktopCreateKind::UrlLink) {
            bool accepted = false;
            const QString urlText = QInputDialog::getText(parent,
                                                          i18n("Link to Location"),
                                                          i18n("Location URL:"),
                                                          QLineEdit::Normal,
                                                          QStringLiteral("https://"),
                                                          &accepted)
                                        .trimmed();
            if (!accepted) {
                return;
            }
            const QUrl url = QUrl::fromUserInput(urlText);
            if (!url.isValid() || url.isEmpty()) {
                QMessageBox::warning(parent, i18n("Link to Location"), i18n("Enter a valid URL."));
                return;
            }
            QString name = QInputDialog::getText(parent,
                                                 i18n("Link to Location"),
                                                 i18n("Name:"),
                                                 QLineEdit::Normal,
                                                 i18n("New Link.desktop"),
                                                 &accepted);
            if (!accepted) {
                return;
            }
            name = normalizedDesktopFileName(name);
            if (!isSafeDesktopItemName(name)) {
                QMessageBox::warning(parent, i18n("Link to Location"), i18n("Enter a name without path separators."));
                return;
            }
            const QByteArray contents = QStringLiteral("[Desktop Entry]\nType=Link\nName=%1\nURL=%2\nIcon=internet-web-browser\n")
                                            .arg(desktopEntryValue(QFileInfo(name).completeBaseName()), desktopEntryValue(url.toString()))
                                            .toUtf8();
            if (!writeNewFile(desktop.filePath(name), contents, &error)) {
                QMessageBox::warning(parent, i18n("Link to Location"), error);
            }
            return;
        }

        if (kind == DesktopCreateKind::FileOrDirectoryLink) {
            bool accepted = false;
            const QString target = QInputDialog::getText(parent,
                                                         i18n("Basic Link to File or Directory"),
                                                         i18n("Existing file or directory:"),
                                                         QLineEdit::Normal,
                                                         QDir::homePath(),
                                                         &accepted)
                                       .trimmed();
            if (!accepted) {
                return;
            }
            const QFileInfo targetInfo(target);
            if (!targetInfo.exists()) {
                QMessageBox::warning(parent, i18n("Basic Link"), i18n("The selected target does not exist."));
                return;
            }
            QString name = QInputDialog::getText(parent,
                                                 i18n("Basic Link"),
                                                 i18n("Name:"),
                                                 QLineEdit::Normal,
                                                 targetInfo.fileName(),
                                                 &accepted);
            if (!accepted) {
                return;
            }
            name = name.trimmed();
            if (!isSafeDesktopItemName(name) || QFileInfo::exists(desktop.filePath(name)) || !QFile::link(targetInfo.absoluteFilePath(), desktop.filePath(name))) {
                QMessageBox::warning(parent, i18n("Basic Link"), i18n("The link could not be created. Check the name and target."));
            }
            return;
        }

        const QString source = QFileDialog::getOpenFileName(parent,
                                                             i18n("Link to Application"),
                                                             QStringLiteral("/usr/share/applications"),
                                                             i18n("Application Launchers (*.desktop)"));
        if (source.isEmpty()) {
            return;
        }
        bool accepted = false;
        QString name = QInputDialog::getText(parent,
                                             i18n("Link to Application"),
                                             i18n("Name:"),
                                             QLineEdit::Normal,
                                             QFileInfo(source).fileName(),
                                             &accepted);
        if (!accepted) {
            return;
        }
        name = normalizedDesktopFileName(name);
        if (!isSafeDesktopItemName(name) || QFileInfo::exists(desktop.filePath(name)) || !QFile::copy(source, desktop.filePath(name))) {
            QMessageBox::warning(parent, i18n("Link to Application"), i18n("The application link could not be created."));
        }
    }
};

QAction *addAction(QMenu *menu, const QString &text, const QString &iconName = {})
{
    QAction *action = menu->addAction(text);
    if (!iconName.isEmpty()) {
        action->setIcon(QIcon::fromTheme(iconName));
    }
    return action;
}

QMenu *addMenu(QMenu *menu, const QString &text, const QString &iconName = {})
{
    QMenu *subMenu = menu->addMenu(text);
    if (!iconName.isEmpty()) {
        subMenu->setIcon(QIcon::fromTheme(iconName));
    }
    return subMenu;
}
}

bool isSafeDesktopItemName(const QString &name)
{
    const QString trimmed = name.trimmed();
    return !trimmed.isEmpty() && trimmed != QLatin1String(".") && trimmed != QLatin1String("..")
        && !trimmed.contains(QLatin1Char('/')) && !trimmed.contains(QLatin1Char('\\'))
        && !trimmed.contains(QLatin1Char('\n')) && !trimmed.contains(QLatin1Char('\r'));
}

bool createEmptyDesktopItem(const QString &desktopDirectory, const QString &name, bool directory, QString *errorMessage)
{
    if (!isSafeDesktopItemName(name)) {
        if (errorMessage) {
            *errorMessage = i18n("Enter a name without path separators.");
        }
        return false;
    }

    QDir desktop(desktopDirectory);
    if (!desktop.exists() && !desktop.mkpath(QStringLiteral("."))) {
        if (errorMessage) {
            *errorMessage = i18n("The Desktop folder is not available.");
        }
        return false;
    }

    const QString trimmed = name.trimmed();
    const QString path = desktop.filePath(trimmed);
    if (QFileInfo::exists(path)) {
        if (errorMessage) {
            *errorMessage = i18n("An item named %1 already exists.", trimmed);
        }
        return false;
    }

    if (directory) {
        if (desktop.mkdir(trimmed)) {
            return true;
        }
        if (errorMessage) {
            *errorMessage = i18n("The folder could not be created.");
        }
        return false;
    }

    return writeNewFile(path, QByteArray(), errorMessage);
}

DesktopFallback::DesktopFallback(QObject *parent)
    : DesktopFallback(nullptr, QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), parent)
{
}

DesktopFallback::DesktopFallback(DesktopFallbackActionRunner *runner, const QString &desktopDirectory, QObject *parent)
    : QObject(parent)
    , m_ownedRunner(runner ? nullptr : std::make_unique<DefaultDesktopFallbackActionRunner>())
    , m_runner(runner ? runner : m_ownedRunner.get())
    , m_desktopDirectory(desktopDirectory)
    , m_menu(std::make_unique<QMenu>())
{
    auto addProgramAction = [this](QMenu *menu,
                                   const QString &text,
                                   const QString &program,
                                   const QStringList &arguments = {},
                                   const QString &iconName = {}) {
        QAction *action = addAction(menu, text, iconName);
        action->setEnabled(m_runner->programAvailable(program));
        QObject::connect(action, &QAction::triggered, action, [this, program, arguments] {
            m_runner->startProgram(program, arguments);
        });
        return action;
    };

    auto addUrlAction = [this](QMenu *menu, const QString &text, const QUrl &url, const QString &iconName = {}) {
        QAction *action = addAction(menu, text, iconName);
        action->setEnabled(url.isValid() && !url.isEmpty());
        QObject::connect(action, &QAction::triggered, action, [this, url] {
            m_runner->openUrl(url);
        });
        return action;
    };

    auto addLocationAction = [&addUrlAction](QMenu *menu,
                                             const QString &text,
                                             QStandardPaths::StandardLocation location,
                                             const QString &iconName) {
        const QString path = QStandardPaths::writableLocation(location);
        QAction *action = addUrlAction(menu, text, QUrl::fromLocalFile(path), iconName);
        action->setEnabled(!path.isEmpty());
        return action;
    };

    auto addBusAction = [this](QMenu *menu,
                               const QString &text,
                               const QString &service,
                               const QString &path,
                               const QString &interface,
                               const QString &method,
                               const QVariantList &arguments = {},
                               const QString &iconName = {}) {
        QAction *action = addAction(menu, text, iconName);
        QObject::connect(action, &QAction::triggered, action, [this, service, path, interface, method, arguments] {
            m_runner->callSessionBus(service, path, interface, method, arguments);
        });
        return action;
    };

    QMenu *fileMenu = m_menu->addMenu(i18n("&File"));
    QMenu *createMenu = addMenu(fileMenu, i18n("Create New"), QStringLiteral("document-new"));
    const QList<QPair<QString, DesktopCreateKind>> createActions = {
        {i18n("Folder…"), DesktopCreateKind::Folder},
        {i18n("Text File…"), DesktopCreateKind::TextFile},
        {i18n("HTML File…"), DesktopCreateKind::HtmlFile},
        {i18n("Link to Location (URL)…"), DesktopCreateKind::UrlLink},
        {i18n("Basic Link to File or Directory…"), DesktopCreateKind::FileOrDirectoryLink},
        {i18n("Link to Application…"), DesktopCreateKind::ApplicationLink},
    };
    for (const auto &[text, kind] : createActions) {
        QAction *action = addAction(createMenu, text);
        QObject::connect(action, &QAction::triggered, action, [this, kind] {
            m_runner->createDesktopItem(kind, m_desktopDirectory, m_menu.get());
        });
    }
    createMenu->insertSeparator(createMenu->actions().at(3));

    fileMenu->addSeparator();
    QAction *restartAction = addAction(fileMenu, i18n("Restart Plasma Shell…"), QStringLiteral("system-reboot"));
    restartAction->setEnabled(m_runner->restartShellAvailable());
    QObject::connect(restartAction, &QAction::triggered, restartAction, [this] {
        if (m_runner->confirm(m_menu.get(),
                              i18n("Restart Plasma Shell"),
                              i18n("Restart Plasma Shell now? Panels and desktop widgets will briefly disappear."))) {
            m_runner->restartShell();
        }
    });
    m_closeWindowAction = addAction(fileMenu, i18n("Close Window"), QStringLiteral("window-close"));
    QObject::connect(m_closeWindowAction, &QAction::triggered, m_closeWindowAction, [this] {
        if (m_closeWindow) {
            m_closeWindow();
        }
    });
    m_forceQuitWindowAction = addAction(fileMenu, i18n("Force Quit Window…"), QStringLiteral("window-close"));
    QObject::connect(m_forceQuitWindowAction, &QAction::triggered, m_forceQuitWindowAction, [this] {
        if (!m_forceQuitWindow || !m_runner->confirm(m_menu.get(),
                                                     i18n("Force Quit Window"),
                                                     i18n("This will force the selected window to close. Continue?"))) {
            return;
        }
        m_forceQuitWindow();
    });
    addBusAction(fileMenu,
                 i18n("Lock Screen"),
                 QStringLiteral("org.freedesktop.ScreenSaver"),
                 QStringLiteral("/ScreenSaver"),
                 QStringLiteral("org.freedesktop.ScreenSaver"),
                 QStringLiteral("Lock"),
                 {},
                 QStringLiteral("system-lock-screen"));
    addBusAction(fileMenu,
                 i18n("Show Logout Screen…"),
                 QStringLiteral("org.kde.LogoutPrompt"),
                 QStringLiteral("/LogoutPrompt"),
                 QStringLiteral("org.kde.LogoutPrompt"),
                 QStringLiteral("promptAll"),
                 {},
                 QStringLiteral("system-log-out"));
    fileMenu->addSeparator();
    addUrlAction(fileMenu,
                 i18n("Open Default Browser"),
                 QUrl(QStringLiteral("https://www.kde.org/")),
                 QStringLiteral("internet-web-browser"));

    QMenu *editMenu = m_menu->addMenu(i18n("&Edit"));
    addBusAction(editMenu,
                 i18n("Clipboard History"),
                 QStringLiteral("org.kde.klipper"),
                 QStringLiteral("/klipper"),
                 QStringLiteral("org.kde.klipper.klipper"),
                 QStringLiteral("showKlipperPopupMenu"),
                 {},
                 QStringLiteral("edit-paste"));
    editMenu->addSeparator();
    addProgramAction(editMenu,
                     i18n("Desktop and Wallpaper…"),
                     QStringLiteral("kcmshell6"),
                     {QStringLiteral("kcm_wallpaper")},
                     QStringLiteral("preferences-desktop-wallpaper"));
    addProgramAction(editMenu,
                     i18n("Display Configuration…"),
                     QStringLiteral("kcmshell6"),
                     {QStringLiteral("kcm_kscreen")},
                     QStringLiteral("preferences-desktop-display"));

    QMenu *viewMenu = m_menu->addMenu(i18n("&View"));
    addBusAction(viewMenu,
                 i18n("Peek at Desktop"),
                 QStringLiteral("org.kde.KWin"),
                 QStringLiteral("/KWin"),
                 QStringLiteral("org.kde.KWin"),
                 QStringLiteral("showDesktop"),
                 {true},
                 QStringLiteral("user-desktop"));
    addBusAction(viewMenu,
                 i18n("Restore Windows"),
                 QStringLiteral("org.kde.KWin"),
                 QStringLiteral("/KWin"),
                 QStringLiteral("org.kde.KWin"),
                 QStringLiteral("showDesktop"),
                 {false},
                 QStringLiteral("view-restore"));
    viewMenu->addSeparator();
    addBusAction(viewMenu,
                 i18n("Overview"),
                 QStringLiteral("org.kde.kglobalaccel"),
                 QStringLiteral("/component/kwin"),
                 QStringLiteral("org.kde.kglobalaccel.Component"),
                 QStringLiteral("invokeShortcut"),
                 {QStringLiteral("Overview")},
                 QStringLiteral("view-grid"));
    addBusAction(viewMenu,
                 i18n("Activities"),
                 QStringLiteral("org.kde.kglobalaccel"),
                 QStringLiteral("/component/plasmashell"),
                 QStringLiteral("org.kde.kglobalaccel.Component"),
                 QStringLiteral("invokeShortcut"),
                 {QStringLiteral("manage activities")},
                 QStringLiteral("activities"));

    QMenu *goMenu = m_menu->addMenu(i18n("&Go"));
    addUrlAction(goMenu, i18n("Home Folder"), QUrl::fromLocalFile(QDir::homePath()), QStringLiteral("user-home"));
    addLocationAction(goMenu, i18n("Documents"), QStandardPaths::DocumentsLocation, QStringLiteral("folder-documents"));
    addLocationAction(goMenu, i18n("Downloads"), QStandardPaths::DownloadLocation, QStringLiteral("folder-download"));
    addUrlAction(goMenu, i18n("Trash"), QUrl(QStringLiteral("trash:/")), QStringLiteral("user-trash"));
    goMenu->addSeparator();
    addUrlAction(goMenu, i18n("Root Filesystem"), QUrl(QStringLiteral("file:///")), QStringLiteral("drive-harddisk-root"));
    addUrlAction(goMenu, i18n("Network"), QUrl(QStringLiteral("network:/")), QStringLiteral("network-workgroup"));
    addUrlAction(goMenu, i18n("Recent Locations"), QUrl(QStringLiteral("recentlyused:/")), QStringLiteral("document-open-recent"));

    QMenu *toolsMenu = m_menu->addMenu(i18n("&Tools"));
    addProgramAction(toolsMenu, i18n("Find Files…"), QStringLiteral("kfind"), {}, QStringLiteral("edit-find"));
    addProgramAction(toolsMenu, i18n("Run Command…"), QStringLiteral("krunner"), {}, QStringLiteral("system-run"));
    addProgramAction(toolsMenu, i18n("Terminal"), QStringLiteral("konsole"), {}, QStringLiteral("utilities-terminal"));
    addProgramAction(toolsMenu, i18n("System Monitor"), QStringLiteral("plasma-systemmonitor"), {}, QStringLiteral("utilities-system-monitor"));
    toolsMenu->addSeparator();
    addProgramAction(toolsMenu, i18n("Manage Disk Usage"), QStringLiteral("filelight"), {}, QStringLiteral("filelight"));
    addProgramAction(toolsMenu,
                     i18n("Partition Manager"),
                     QStringLiteral("partitionmanager"),
                     {},
                     QStringLiteral("partitionmanager"));

    QMenu *settingsMenu = m_menu->addMenu(i18n("&Settings"));
    addProgramAction(settingsMenu, i18n("System Settings"), QStringLiteral("systemsettings"), {}, QStringLiteral("settings-configure"));
    settingsMenu->addSeparator();
    const QList<QPair<QString, QString>> settingsPages = {
        {i18n("Power Management…"), QStringLiteral("kcm_powerdevilprofilesconfig")},
        {i18n("Date and Time…"), QStringLiteral("kcm_clock")},
        {i18n("Region and Language…"), QStringLiteral("kcm_regionandlang")},
        {i18n("Bluetooth…"), QStringLiteral("kcm_bluetooth")},
    };
    for (const auto &[text, module] : settingsPages) {
        addProgramAction(settingsMenu, text, QStringLiteral("kcmshell6"), {module});
    }

    QMenu *helpMenu = m_menu->addMenu(i18n("&Help"));
    addProgramAction(helpMenu, i18n("KDE Help Center"), QStringLiteral("khelpcenter"), {}, QStringLiteral("help-contents"));
    helpMenu->addSeparator();
    addUrlAction(helpMenu,
                 i18n("Global Menu KDE Documentation"),
                 QUrl(QStringLiteral("https://github.com/ChathurangaBW/global-menu-KDE#readme")),
                 QStringLiteral("documentation"));
    addUrlAction(helpMenu,
                 i18n("Global Menu KDE Issues"),
                 QUrl(QStringLiteral("https://github.com/ChathurangaBW/global-menu-KDE/issues")),
                 QStringLiteral("tools-report-bug"));
    addUrlAction(helpMenu,
                 i18n("KDE Community"),
                 QUrl(QStringLiteral("https://www.reddit.com/r/kde/")),
                 QStringLiteral("kde"));
}

DesktopFallback::~DesktopFallback() = default;

void DesktopFallback::setActiveWindowActions(bool canClose,
                                              std::function<void()> closeWindow,
                                              bool canForceQuit,
                                              std::function<void()> forceQuitWindow)
{
    m_closeWindow = std::move(closeWindow);
    m_forceQuitWindow = std::move(forceQuitWindow);
    if (m_closeWindowAction) {
        m_closeWindowAction->setEnabled(canClose);
    }
    if (m_forceQuitWindowAction) {
        m_forceQuitWindowAction->setEnabled(canForceQuit);
    }
}

QMenu *DesktopFallback::menu() const
{
    return m_menu.get();
}
