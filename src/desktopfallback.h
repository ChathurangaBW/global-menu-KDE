// SPDX-FileCopyrightText: 2026 ChathurangaBW
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantList>

#include <memory>

class QMenu;
class QWidget;

enum class DesktopCreateKind {
    Folder,
    TextFile,
    HtmlFile,
    UrlLink,
    FileOrDirectoryLink,
    ApplicationLink,
};

class DesktopFallbackActionRunner
{
public:
    virtual ~DesktopFallbackActionRunner() = default;

    virtual bool programAvailable(const QString &program) const = 0;
    virtual void openUrl(const QUrl &url) = 0;
    virtual void startProgram(const QString &program, const QStringList &arguments) = 0;
    virtual void callSessionBus(const QString &service,
                                const QString &path,
                                const QString &interface,
                                const QString &method,
                                const QVariantList &arguments = {}) = 0;
    virtual bool confirm(QWidget *parent, const QString &title, const QString &text) = 0;
    virtual void createDesktopItem(DesktopCreateKind kind,
                                   const QString &desktopDirectory,
                                   QWidget *parent) = 0;
};

bool isSafeDesktopItemName(const QString &name);
bool createEmptyDesktopItem(const QString &desktopDirectory,
                            const QString &name,
                            bool directory,
                            QString *errorMessage = nullptr);

class DesktopFallback final : public QObject
{
public:
    explicit DesktopFallback(QObject *parent = nullptr);
    DesktopFallback(DesktopFallbackActionRunner *runner,
                    const QString &desktopDirectory,
                    QObject *parent = nullptr);
    ~DesktopFallback() override;

    QMenu *menu() const;

private:
    std::unique_ptr<DesktopFallbackActionRunner> m_ownedRunner;
    DesktopFallbackActionRunner *m_runner = nullptr;
    QString m_desktopDirectory;
    std::unique_ptr<QMenu> m_menu;
};
