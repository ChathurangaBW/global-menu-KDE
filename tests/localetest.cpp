// SPDX-License-Identifier: GPL-2.0-or-later
#include "desktopfallback.h"
#include <KLocalizedString>
#include <QApplication>
#include <QMenu>
#include <QtTest>

class LocaleTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void usesProcessLocale()
    {
        // The test harness mirrors this process LANGUAGE value into KI18n.
        const bool french = qEnvironmentVariable("LANGUAGE") == QStringLiteral("fr");
        if (french) {
            QVERIFY(KLocalizedString::languages().contains(QStringLiteral("fr")));
            QVERIFY(KLocalizedString::availableDomainTranslations("plasma_applet_org.chathuranga.globalmenu").contains(QStringLiteral("fr")));
        }
        DesktopFallback fallback;
        const auto headings = fallback.menu()->actions();
        QCOMPARE(headings.size(), 7);
        QCOMPARE(headings.first()->text(), french ? QStringLiteral("&Fichier") : QStringLiteral("&File"));
        QCOMPARE(i18n("Search…"), french ? QStringLiteral("Rechercher…") : QStringLiteral("Search…"));
        QCOMPARE(i18n("Find Files…"), french ? QStringLiteral("Rechercher des fichiers…") : QStringLiteral("Find Files…"));
        QCOMPARE(i18n("This will force the selected window to close. Continue?"), french
            ? QStringLiteral("La fermeture de la fenêtre sélectionnée sera forcée. Continuer ?")
            : QStringLiteral("This will force the selected window to close. Continue?"));
        QCOMPARE(i18nc("@info:usagetip", "Open a menu"), french ? QStringLiteral("Ouvrir un menu") : QStringLiteral("Open a menu"));
    }
};

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    KLocalizedString::addDomainLocaleDir("plasma_applet_org.chathuranga.globalmenu", QStringLiteral(TEST_LOCALE_DIR));
    // Minimal CI images do not always generate locale definitions. Mirror the
    // process LANGUAGE value explicitly so the test remains deterministic while
    // production continues to follow Plasma/KI18n's configured language list.
    const QString language = qEnvironmentVariable("LANGUAGE");
    if (!language.isEmpty()) {
        KLocalizedString::setLanguages(QStringList{language});
    }
    KLocalizedString::setApplicationDomain("plasma_applet_org.chathuranga.globalmenu");
    LocaleTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "localetest.moc"
