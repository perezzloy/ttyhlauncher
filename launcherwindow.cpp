#include "launcherwindow.h"
#include "ui_launcherwindow.h"

#include "settingsdialog.h"
#include "passworddialog.h"
#include "skinuploaddialog.h"
#include "updatedialog.h"
#include "feedbackdialog.h"
#include "aboutdialog.h"

#include "settings.h"
#include "util.h"

#include <QtGui>
#include <QDesktopWidget>
#include <QMessageBox>

LauncherWindow::LauncherWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LauncherWindow)
{
    // Setup form from ui-file
    ui->setupUi(this);

    // Setup settings and logger
    settings = Settings::instance();
    logger = Logger::logger();

    // Make news menuitems like radiobuttons (it's impossible from qt-designer)
    newsGroup = new QActionGroup(this);
    newsGroup->addAction(ui->ttyhNews);
    newsGroup->addAction(ui->officialNews);

    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(ui->webView, SIGNAL(linkClicked(const QUrl&)), SLOT(linkClicked(const QUrl&)));

    // News Menu connections
    connect(ui->ttyhNews, SIGNAL(changed()), SLOT(loadTtyh()));
    connect(ui->officialNews, SIGNAL(changed()), SLOT(loadOfficial()));

    // Options Menu connections
    connect(ui->runSettings, SIGNAL(triggered()), SLOT(showSettingsDialog()));

    // Additional Menu connections
    connect(ui->changePassword, SIGNAL(triggered()), SLOT(showChangePasswordDialog()));
    connect(ui->changeSkin, SIGNAL(triggered()), SLOT(showSkinLoadDialog()));
    connect(ui->updateManager, SIGNAL(triggered()), SLOT(showUpdateManagerDialog()));

    // Help Menu connections
    connect(ui->bugReport, SIGNAL(triggered()), SLOT(showFeedBackDialog()));
    connect(ui->aboutLauncher, SIGNAL(triggered()), SLOT(showAboutDialog()));

    // Setup login field
    ui->nickEdit->setText(settings->loadLogin());
    // Save login when changed
    connect(ui->nickEdit, SIGNAL(textChanged(QString)), settings, SLOT(saveLogin(QString)));

    // Setup password field
    ui->savePassword->setChecked(settings->loadPassStore());
    if (ui->savePassword->isChecked())
        ui->passEdit->setText(settings->loadPassword());
    // Password are saved on login or exit if savePassword is checked
    connect(ui->savePassword, SIGNAL(clicked(bool)), settings, SLOT(savePassStore(bool)));

    // Setup client combobox
    ui->clientCombo->addItems(settings->getClientsNames());
    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
    connect(ui->clientCombo, SIGNAL(activated(int)), settings, SLOT(saveActiveClientId(int)));

    // Setup news set
    ui->ttyhNews->setChecked(true);
    emit ui->ttyhNews->changed();

    // Setup window parameters
    QRect geometry = settings->loadWindowGeometry();
    // Centering window, if loaded default values
    if (geometry.x() < 0)
        this->move(QApplication::desktop()->screen()->rect().center() - this->rect().center());
    else
        this->setGeometry(geometry);

    // Restore maximized state
    if (settings->loadMaximizedState()) this->showMaximized();

    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(playButtonClicked()));

    logger->append(this->objectName(), "Launcher window opened\n");

    if (ui->clientCombo->count() == 0) {
        this->show(); // hack to show window before error message
        ui->playButton->setEnabled(false);
        QMessageBox::critical(this, "Беда-беда!",  "Не удалось получить список клиентов\nМы все умрём.\n");
    }

}

void LauncherWindow::closeEvent (QCloseEvent* event) {
    logger->append(this->objectName(), "Launcher window closed\n");
    storeParameters();
    event->accept();
}

void LauncherWindow::keyPressEvent(QKeyEvent* pe) {
    if(pe->key() == Qt::Key_Return) playButtonClicked();
    pe->accept();
}


// Run this method on close window and run game
void LauncherWindow::storeParameters() {
    settings->saveWindowGeometry(this->geometry());
    settings->saveMaximizedState(this->isMaximized());

    if (ui->savePassword->isChecked())
        settings->savePassword(ui->passEdit->text());
    // Security issue
    else
        settings->savePassword("");
}

// Show dialog slots
void LauncherWindow::showSettingsDialog() {
    SettingsDialog* d = new SettingsDialog(this);
    d->exec();
    delete d;

    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
}

void LauncherWindow::showChangePasswordDialog() {
    PasswordDialog* d = new PasswordDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showSkinLoadDialog() {
    SkinUploadDialog* d = new SkinUploadDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showUpdateManagerDialog() {
    showUpdateDialog("Для проверки наличия обновлений выберите нужный клиет и нажмите кнопку \"Проверить\"");
}

void LauncherWindow::showFeedBackDialog() {
    FeedbackDialog* d = new FeedbackDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showAboutDialog() {
    AboutDialog* d = new AboutDialog(this);
    d->exec();
    delete d;
}

void LauncherWindow::showUpdateDialog(QString message) {

    UpdateDialog* d = new UpdateDialog(message, this);
    d->exec();
    delete d;

    ui->clientCombo->setCurrentIndex(settings->loadActiveClientId());
}

// Load webpage slots
void LauncherWindow::loadTtyh() {loadPage(QUrl("http://ttyh.ru"));}
void LauncherWindow::loadOfficial() {loadPage(QUrl("http://mcupdate.tumblr.com/"));}

// Open external browser slot
void LauncherWindow::linkClicked(const QUrl& url) {
    logger->append(this->objectName(), "Try to open url in external browser. " + url.toString() + "\n");
    if (!QDesktopServices::openUrl(url))
        logger->append(this->objectName(), "Filed to open system browser!");
}

// Load webpage method
void LauncherWindow::loadPage(const QUrl& url) {
    // Show "loading..." during webpage load
    // FIXME: this is not working
    ui->webView->load(QUrl("qrc:/resources/loading.html"));
    ui->webView->load(url);

    // Setup timeout timer
    QTimer* ptimer = new QTimer(ui->webView);
    connect(ptimer, SIGNAL(timeout()), SLOT(loadPageTimeout()));                // Show error page on timeout
    connect(ui->webView, SIGNAL(loadFinished(bool)), SLOT(pageLoaded(bool)));   // Show error page on loading fail
    connect(ui->webView, SIGNAL(loadFinished(bool)), ptimer, SLOT(stop()));
    ptimer->start(15000); // Timeout value
}

// Slots used by LoadPage method
void LauncherWindow::loadPageTimeout() {ui->webView->load(QUrl("qrc:/resources/error.html"));}
void LauncherWindow::pageLoaded(bool loaded) {if (!loaded) ui->webView->load(QUrl("qrc:/resources/error.html"));}

void LauncherWindow::playButtonClicked() {

    logger->append(this->objectName(), "Try to start game...\n");
    logger->append(this->objectName(), "Client id: "
                   + settings->getClientStrId(settings->loadActiveClientId()) + "\n");

    ui->centralWidget->setEnabled(false);

    if (!ui->playOffline->isChecked()) {
        logger->append(this->objectName(), "Online mode is selected\n");

        // Make JSON login request, see: http://wiki.vg/Authentication
        QJsonObject payload, agent, platform;

        agent["name"] = "Minecraft";
        agent["version"] = 1;

        platform["os"] = settings->getOsName();
        platform["version"] = settings->getOsVersion();
        platform["word"] = settings->getWordSize();

        payload["agent"] = agent;
        payload["platform"] = platform;
        payload["username"] = ui->nickEdit->text();
        payload["password"] = ui->passEdit->text();
        payload["ticket"] = settings->makeMinecraftUuid().remove('{').remove('}');
        payload["launcherVersion"] = Settings::launcherVersion;

        QJsonDocument jsonRequest(payload);

        logger->append(this->objectName(), "Making login request...\n");
        Reply loginReply = Util::makePost(Settings::authUrl, jsonRequest.toJson());

        if (!loginReply.isOK()) {

            QMessageBox::critical(this, "У нас проблема :(", "Упс... Вот ведь незадача...\n"
                                  + loginReply.getErrorString());
            logger->append(this->objectName(), "Error: " + loginReply.getErrorString() + "\n");

        } else { // Successful login request

            QJsonParseError error;
            QJsonDocument jsonLoginReply = QJsonDocument::fromJson(loginReply.reply(), &error);

            if (!(error.error == QJsonParseError::NoError)) {

                QMessageBox::critical(this, "У нас проблема :(", "При попытке логина сервер овтетил ерунду...\n\n"
                                      + error.errorString() + " в позиции " + QString::number(error.offset));
                logger->append(this->objectName(), "JSON parse error: " + error.errorString()
                               + " в поз. "  + QString::number(error.offset) + "\n");

            } else { // Correct login request

                QJsonObject loginReplyData = jsonLoginReply.object();

                if (!loginReplyData["error"].isNull()) {

                    QMessageBox::critical(this, "У нас проблема :(", loginReplyData["errorMessage"].toString());
                    logger->append(this->objectName(), "Error: " + loginReplyData["errorMessage"].toString() + "\n");

                } else { // No "error" field in responce

                    // Prepare to run game in online-mode
                    logger->append(this->objectName(), "OK\n");

                    QString uuid = loginReplyData["clientToken"].toString();
                    QString acessToken = loginReplyData["accessToken"].toString();
                    QString gameVersion = settings->loadClientVersion();

                    // Switch from "latest" to real version
                    bool run = true;
                    if (gameVersion == "latest") {

                        logger->append(this->objectName(), "Looking for 'latest' version on update server...\n");
                        Reply versionReply = Util::makeGet(settings->getVersionsUrl());

                        if (!versionReply.isOK()) {

                            QMessageBox::critical(this, "У нас проблема :(", "Не удалось определить версию для запуска!\n"
                                                  + versionReply.getErrorString());
                            logger->append(this->objectName(), "Error: " + versionReply.getErrorString() + "\n");
                            run = false;

                        } else { // Successful version request

                            QJsonParseError error;
                            QJsonDocument jsonVersionReply = QJsonDocument::fromJson(versionReply.reply(), &error);

                            if (!(error.error == QJsonParseError::NoError)) {

                                QMessageBox::critical(this, "У нас проблема :(", "Не удалось понять что же нужно запустить...\n"
                                                      + error.errorString() + " в поз. "  + QString::number(error.offset));
                                logger->append(this->objectName(), "JSON parse error: " + error.errorString()
                                               + " в поз. "  + QString::number(error.offset) + "\n");
                                run = false;

                            } else { // Correct version reply

                                QJsonObject latest = jsonVersionReply.object()["latest"].toObject();
                                if (latest["release"].isNull()) {

                                    run = false;
                                    QMessageBox::critical(this, "У нас проблема :(", "Не удалось определить версию для запуска!\n");
                                    logger->append(this->objectName(), "Error: empty game version\n");

                                } else {

                                    gameVersion = latest["release"].toString();
                                    logger->append(this->objectName(), "Game version is " + gameVersion + "\n");

                                }
                            }
                        }
                    }

                    // update json indexes before run (version, libs, [files])
                    logger->append(this->objectName(), "Updating game indexes..." + gameVersion + "\n");

                    QString currentVersionDir = settings->getVersionsDir() + "/" + gameVersion + "/" ;

                    Util::downloadFile(settings->getVersionUrl(gameVersion) + gameVersion + ".json", currentVersionDir + gameVersion + ".json");
                    Util::downloadFile(settings->getVersionUrl(gameVersion) + "libs.json", currentVersionDir + "libs.json");

                    QByteArray jsonData;
                    jsonData.append(Util::getFileContetnts(currentVersionDir + gameVersion + ".json"));
                    QJsonObject versionIndex = QJsonDocument::fromJson(jsonData).object();
                    if (versionIndex["customFiles"].toBool()) {
                        Util::downloadFile(settings->getVersionUrl(gameVersion) + "files.json", currentVersionDir + "files.json");
                    }

                    if (!versionIndex["assets"].isNull()) {
                        QString assets = versionIndex["assets"].toString();
                        Util::downloadFile(settings->getAssetsUrl() + "indexes/" + assets + ".json",
                                           settings->getAssetsDir() + "/indexes/" + assets + ".json");
                    }


                    if (run) runGame(uuid, acessToken, gameVersion);
                }
            }
        }

    } else { // Offline mode

        logger->append(this->objectName(), "Offline mode is selected\n");

        QString uuid = "HARD";
        QString acessToken = "CORE";
        QString gameVersion = settings->loadClientVersion();

        bool run = true;
        if (gameVersion == "latest") {
            logger->append(this->objectName(), "Looking for 'latest' local version\n");

            // Great old date for comparsion!
            // releaseDate used for store latest founded release date and write gameVersion if this happened
            QDateTime releaseDate = QDateTime::fromString("1991-05-18T13:15:00+07:00", Qt::ISODate);

            QDir verDir = QDir(settings->getVersionsDir());
            QStringList verList = verDir.entryList();

            for (QStringList::iterator nameit = verList.begin(), end = verList.end(); nameit != end; ++nameit) {
                QString currentVersion = (*nameit);
                QFile* versionFile = new QFile(settings->getVersionsDir()
                                               + "/" + currentVersion
                                               + "/" + currentVersion + ".json");
                if (versionFile->open(QIODevice::ReadOnly)) {

                    QJsonParseError error;
                    QJsonDocument versionJson =  QJsonDocument::fromJson(versionFile->readAll(), &error);
                    if (error.error == QJsonParseError::NoError) {

                        QString currentTimeStr = versionJson.object()["releaseTime"].toString();
                        QDateTime curRelTime = QDateTime::fromString(currentTimeStr, Qt::ISODate);

                        if (curRelTime.isValid() && (curRelTime > releaseDate)) {
                            releaseDate = curRelTime;
                            gameVersion = currentVersion;
                        }

                    }
                    versionFile->close();
                }
                delete versionFile;
            }

            if (gameVersion == "latest") {
                logger->append(this->objectName(), "Error: no local versions\n");
                showUpdateDialog(QString("Похоже, что не установлено ни одной версии клиента. Выполните обновление.\n")
                                 + "Нажмите кнопку \"Проверить\", а затем \"Обновить\"");
                run = false;
            }
        }

        if (run) runGame(uuid, acessToken, gameVersion);

    }

    ui->centralWidget->setEnabled(true);
}

void LauncherWindow::runGame(QString uuid, QString acessToken, QString gameVersion) {

    logger->append(this->objectName(), "Preparing game to run...\n");

    QString java, libpath, classpath,
            mainClass, minecraftArguments;

    // Setup java binary
    if (settings->loadClientJavaState()) {
        java = settings->loadClientJava();
    } else {
        java = "java";
    }

    // Prepare library path
    logger->append(this->objectName(), "Prepare natives directory...\n");
    libpath = settings->getNativesDir();
    Util::removeAll(libpath);

    QDir libsDir = QDir(libpath);
    libsDir.mkpath(libpath);

    if (!libsDir.exists()) {
        QMessageBox::critical(this, "У нас проблема :(",
                              "Не удалось подготовить LIBRARY_PATH. Извините :(");
        logger->append(this->objectName(), "Error: can't create natives directory!\n");
        return;
    }

    // Open version index file
    QFile* versionFile = new QFile(settings->getVersionsDir() + "/" + gameVersion + "/" + gameVersion + ".json");
    logger->append(this->objectName(), "Reading version file: " + versionFile->fileName() + "\n");

    if (!versionFile->open(QIODevice::ReadOnly)) {

        logger->append(this->objectName(), "Error: can't open version file\n");
        showUpdateDialog(QString("Для запуска игры необходимо выполнить обновление! ")
                         + "Нажмите кнопку \"Проверить\", а затем \"Обновить\"");
        delete versionFile;
        return;
    }

    QJsonParseError error;
    QJsonObject versionIndex = QJsonDocument::fromJson(versionFile->readAll(), &error).object();
    versionFile->close();
    delete versionFile;

    if (!(error.error == QJsonParseError::NoError)) {

        QMessageBox::critical(this, "У нас проблема :(", "Не удалось разобрать индекс версии...\n"
                              + error.errorString() + "  поз. " + QString::number(error.offset));
        logger->append(this->objectName(), "JSON parse error: " + error.errorString() + " в поз. "
                       + QString::number(error.offset) + "\n");
        return;
    }

    // Open libs index file
    QFile* libIndexFile = new QFile(settings->getVersionsDir() + "/" + gameVersion + "/" + "libs.json");
    logger->append(this->objectName(), "Reading index file: " + libIndexFile->fileName() + "\n");

    if (!libIndexFile->open(QIODevice::ReadOnly)) {

        logger->append(this->objectName(), "Error: can't open lib index file\n");
        showUpdateDialog(QString("Для запуска игры необходимо выполнить обновление! ")
                         + "Нажмите кнопку \"Проверить\", а затем \"Обновить\"");
        delete libIndexFile;
        return;
    }

    QJsonObject libIndex = QJsonDocument::fromJson(libIndexFile->readAll(), &error).object()["objects"].toObject();
    libIndexFile->close();
    delete libIndexFile;

    if (!(error.error == QJsonParseError::NoError)) {

        QMessageBox::critical(this, "У нас проблема :(", "Не удалось разобрать индекс библиотек...\n"
                              + error.errorString() + "  поз. " + QString::number(error.offset));
        logger->append(this->objectName(), "JSON parse error: " + error.errorString() + " в поз. "
                       + QString::number(error.offset) + "\n");
        return;
    }

    QJsonArray libraries = versionIndex["libraries"].toArray();
    foreach (QJsonValue libValue, libraries) {

        QJsonObject library = libValue.toObject();

        QStringList entry = library["name"].toString().split(':');

        // <package>:<name>:<version> to <package>/<name>/<version>/<name>-<version> and chahge <backage> format from a.b.c to a/b/c
        QString libSuffix = entry.at(0);                  // package
        libSuffix.replace('.', '/');                      // package format
        libSuffix += "/" + entry.at(1)                    // + name
                + "/" + entry.at(2)                       // + version
                + "/" + entry.at(1) + "-" + entry.at(2);  // + name-version

        // Check for allow-disallow rules
        QJsonArray rules = library["rules"].toArray();
        bool allowLib = true;
        if (!rules.isEmpty()) {

            // Disallow libray if not in allow list
            allowLib = false;

            foreach (QJsonValue ruleValue, rules) {
                QJsonObject rule = ruleValue.toObject();

                // Process allow variants (all or specified)
                if (rule["action"].toString() == "allow") {
                    if (rule["os"].toObject().isEmpty()) {
                        allowLib = true;
                    } else if (rule["os"].toObject()["name"].toString() == settings->getOsName()) {
                        allowLib = true;
                    }
                }

                // Make exclusions from allow-list
                if (rule["action"].toString() == "disallow") {
                    if (rule["os"].toObject()["name"].toString() == settings->getOsName()) {
                        allowLib = false;
                    }
                }
            }
        }

        if (!allowLib) {
            logger->append(this->objectName(), "Skipping lib: " + libSuffix + ".jar\n");
            continue;
        }

        if (library["natives"].isNull()) {

            if (!isValidGameFile(settings->getLibsDir() + "/" + libSuffix + ".jar", libIndex[libSuffix + ".jar"].toObject()["hash"].toString())) {

                showUpdateDialog(QString("Для запуска игры необходимо выполнить обновление! ")
                                 + "Нажмите кнопку \"Проверить\", а затем \"Обновить\"");
                return;
            }

            if (settings->getOsName() == "windows") libSuffix += ".jar;";
            else libSuffix += ".jar:";

            classpath += settings->getLibsDir() + "/" + libSuffix;

        } else {

            libSuffix += "-natives-" + settings->getOsName() + ".jar";
            if (!isValidGameFile(settings->getLibsDir() + "/" + libSuffix, libIndex[libSuffix].toObject()["hash"].toString())) {

                showUpdateDialog(QString("Для запуска игры необходимо выполнить обновление! ")
                                 + "Нажмите кнопку \"Проверить\", а затем \"Обновить\"");
                return;
            }
            Util::unzipArchive(settings->getLibsDir() + "/" + libSuffix, settings->getNativesDir());
        }


    }

    // Add game jar to classpath
    classpath += settings->getVersionsDir() + "/" + gameVersion + "/" + gameVersion + ".jar";

    QString jarHash = versionIndex["jarHash"].toString();
    if (!isValidGameFile(settings->getVersionsDir() + "/" + gameVersion + "/" + gameVersion + ".jar", jarHash)) {
        showUpdateDialog(QString("Для запуска игры необходимо выполнить обновление! ")
                         + "Нажмите кнопку \"Проверить\", а затем \"Обновить\"");
        return;
    }

    // Open custom files index
    if (versionIndex["customFiles"].toBool()) {

        QFile* customFilesIndexFile = new QFile(settings->getVersionsDir() + "/" + gameVersion + "/" + "files.json");
        logger->append(this->objectName(), "Reading index file: " + customFilesIndexFile->fileName() + "\n");

        if (!customFilesIndexFile->open(QIODevice::ReadOnly)) {

            logger->append(this->objectName(), "Error: can't open index file\n");
            showUpdateDialog(QString("Для запуска игры необходимо выполнить обновление! ")
                             + "Нажмите кнопку \"Проверить\", а затем \"Обновить\"");
            delete customFilesIndexFile;
            return;
        }

        QJsonObject customFilesObject = QJsonDocument::fromJson(customFilesIndexFile->readAll(), &error).object();
        customFilesIndexFile->close();
        delete customFilesIndexFile;

        if (!(error.error == QJsonParseError::NoError)) {

            QMessageBox::critical(this, "У нас проблема :(", "Не удалось разобрать индекс модификаций...\n"
                                  + error.errorString() + "  поз. " + QString::number(error.offset));
            logger->append(this->objectName(), "JSON parse error: " + error.errorString() + " в поз. "
                           + QString::number(error.offset) + "\n");
            return;
        }

        QStringList mutableFileList;
        QJsonArray mutableFileIndex = customFilesObject["mutable"].toArray();
        foreach (QJsonValue entry, mutableFileIndex) {
            mutableFileList.append(entry.toString());
        }

        QJsonObject regularFileIndex = customFilesObject["objects"].toObject();
        QString filesPrefix = settings->getClientPrefix(gameVersion);

        foreach (QString file, regularFileIndex.keys()) {

            QString hash;
            if (mutableFileList.indexOf(file) == -1) {
                hash = regularFileIndex[file].toObject()["hash"].toString();
            } else {
                hash = "mutable";
            }

            if (!isValidGameFile(filesPrefix + "/" + file, hash)) {
                showUpdateDialog(QString("Для запуска игры необходимо выполнить обновление! ")
                                 + "Нажмите кнопку \"Проверить\", а затем \"Обновить\"");
                return;
            }
        }
    }

    // Setup mainClass
    if (versionIndex["mainClass"].isNull()) {

        QMessageBox::critical(this, "У нас проблема :(", "Вот беда. В конфигурационном файле не указан mainClass.");
        logger->append(this->objectName(), "Error: can't read mainClass\n");
        return;

    } else {

        mainClass = versionIndex["mainClass"].toString();

    }

    // Read assets index
    QString assetsVersion;
    if (versionIndex["assets"].isNull()) {

        QMessageBox::critical(this, "У нас проблема !!!",  "Аааа! В конфигурационном файле не указаны ресурсы игры!");
        logger->append(this->objectName(), "Error: can't read assets index name\n");
        return;

    } else {

        assetsVersion = versionIndex["assets"].toString();

        // Open assets index file
        QFile* assetIndexFile = new QFile(settings->getAssetsDir() + "/indexes/" + assetsVersion + ".json");
        logger->append(this->objectName(), "Reading index file: " + assetIndexFile->fileName() + "\n");

        if (!assetIndexFile->open(QIODevice::ReadOnly)) {

            logger->append(this->objectName(), "Error: can't open index file\n");
            showUpdateDialog(QString("Для запуска игры необходимо выполнить обновление! ")
                             + "Нажмите кнопку \"Проверить\", а затем \"Обновить\"");
            delete assetIndexFile;
            return;
        }

        QJsonObject assetIndex = QJsonDocument::fromJson(assetIndexFile->readAll(), &error).object()["objects"].toObject();
        assetIndexFile->close();
        delete assetIndexFile;

        if (!(error.error == QJsonParseError::NoError)) {

            QMessageBox::critical(this, "У нас проблема :(", "Не удалось разобрать индекс ресурсов...\n"
                                  + error.errorString() + "  поз. " + QString::number(error.offset));
            logger->append(this->objectName(), "JSON parse error: " + error.errorString() + " в поз. "
                           + QString::number(error.offset) + "\n");
            return;
        }

        QString assetsPrefix = settings->getAssetsDir() + "/objects/";
        foreach (QString key, assetIndex.keys()) {

            QString hash = assetIndex[key].toObject()["hash"].toString();;
            QString assetSuffix = hash.mid(0, 2);

            if (!isValidGameFile(assetsPrefix + assetSuffix + "/" + hash, hash)) {
                showUpdateDialog(QString("Для запуска игры необходимо выполнить обновление! ")
                                 + "Нажмите кнопку \"Проверить\", а затем \"Обновить\"");
                return;
            }
        }
    }

    // Setup aruments
    if (versionIndex["minecraftArguments"].isNull()) {

        QMessageBox::critical(this, "У нас проблема :(", "В конфигурационном файле не указаны аргументы запуска.");
        logger->append(this->objectName(), "Error: can't read minecraft arguments\n");
        return;

    } else {

        minecraftArguments = versionIndex["minecraftArguments"].toString();
    }

    // Crazy way, but this must work
    QStringList mcArgList;
    foreach (QString mcArg, minecraftArguments.split(" ")) {

        mcArg.replace("${auth_player_name}",  settings->loadLogin());
        mcArg.replace("${version_name}",      gameVersion);
        mcArg.replace("${game_directory}",    settings->getClientPrefix(gameVersion));
        mcArg.replace("${assets_root}",       settings->getAssetsDir());
        mcArg.replace("${assets_index_name}", assetsVersion);
        mcArg.replace("${auth_uuid}",         uuid);
        mcArg.replace("${auth_access_token}", acessToken);
        mcArg.replace("${user_properties}",   "{}");
        mcArg.replace("${user_type}",       "mojang");

        mcArgList << mcArg;
    }

    // RUN-RUN-RUN!
    logger->append(this->objectName(), "Making run string...\n");
    QStringList argList;

    // Workaround for Oracle Java + StartSSL
    argList << "-Djavax.net.ssl.trustStore=" + settings->getConfigDir() + "/keystore.ks"
            << "-Djavax.net.ssl.trustStorePassword=123456";

    // Setup user args
    QStringList userArgList;
    if (settings->loadClientJavaArgsState()) {
        userArgList = settings->loadClientJavaArgs().split(" ");
    }
    if (!userArgList.isEmpty()) argList << userArgList;

    argList << "-Djava.library.path=" + libpath
            << "-cp" << classpath
            << mainClass
            << mcArgList;

    QString stringargs = argList.join(' ');
    logger->append(this->objectName(), "Run string: " + java + " " + stringargs + "\n");

    logger->append(this->objectName(), "Try to launch game...\n");
    QProcess* minecraft = new QProcess(this);
    minecraft->setProcessChannelMode(QProcess::MergedChannels);

    // Set working directory
    QDir(settings->getClientPrefix(gameVersion)).mkpath(settings->getClientPrefix(gameVersion));
    minecraft->setWorkingDirectory(settings->getClientPrefix(gameVersion));

    minecraft->start(java, argList);

    if (!minecraft->waitForStarted()) {

        switch(minecraft->error()) {
        case QProcess::FailedToStart:
            QMessageBox::critical(this, "Проблема!",
                                  "Смерть на взлёте! Игра не запускается!\n"
                                  + minecraft->errorString());
            logger->append(this->objectName(), "Error: failed to start: "
                           + minecraft->errorString() + "\n");
            break;

        case QProcess::Crashed:
            QMessageBox::critical(this, "Проблема!",
                                  "Игра упала и не подымается :(\n"
                                  + minecraft->errorString());
            logger->append(this->objectName(), "Error: crashed: "
                           + minecraft->errorString() + "\n");
            break;

        case QProcess::Timedout:
            QMessageBox::critical(this, "Проблема!",
                                  "Что-то долго игра не может запуститься...\n"
                                  + minecraft->errorString());
            logger->append(this->objectName(), "Error: timeout: "
                           + minecraft->errorString() + "\n");
            break;

        case QProcess::WriteError:
            QMessageBox::critical(this, "Проблема!",
                                  "Игра не может писать :(\n"
                                  + minecraft->errorString());
            logger->append(this->objectName(), "Error: write error: "
                           + minecraft->errorString() + "\n");
            break;

        case QProcess::ReadError:
            QMessageBox::critical(this, "Проблема!",
                                  "Игра не может читать :(!\n"
                                  + minecraft->errorString());
            logger->append(this->objectName(), "Error: read error: "
                           + minecraft->errorString() + "\n");
            break;

        case QProcess::UnknownError:
        default:
            QMessageBox::critical(this, "Проблема!",
                                  "Произошло что-то странное и игра не запустилась!\n"
                                  + minecraft->errorString());
            logger->append(this->objectName(), "Error: "
                           + minecraft->errorString() + "\n");
            break;
        }

    } else { // Game successful started

        logger->append(this->objectName(), "Main window hidden\n");
        this->hide();

        while (minecraft->state() == QProcess::Running) {
            if (minecraft->waitForReadyRead()) {
                logger->append("Client", minecraft->readAll());
            }
        }

        logger->append(this->objectName(), "Game process finished!\n");
        if (minecraft->exitCode() != 0) {

            this->show();
            QMessageBox::critical(this, "Ну вот!",  "Кажется игра некорректно завершилась, посмотрите лог-файл.\n");
            logger->append(this->objectName(), "Error: not null game exit code: " + QString::number(minecraft->exitCode()) + "\n");
            logger->append(this->objectName(), "Main window showed\n");

        } else {

            this->close();

        }
    }

    delete minecraft;

}

bool LauncherWindow::isValidGameFile(QString fileName, QString hash) {

    logger->append(this->objectName(), "Precheck: " + fileName + "\n");

    if (!QFile::exists(fileName)) {

        logger->append(this->objectName(), "Precheck: file not exists!\n");
        return false;
    }

    if (hash == "mutable") return true;

    QFile* file = new QFile(fileName);
    if (!file->open(QIODevice::ReadOnly)) {

        logger->append(this->objectName(), "Precheck: can't read file!\n");
        delete file;
        return false;

    } else {

        QByteArray data = file->readAll();
        QString fileHash = QString(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex());
        file->close();
        delete file;

        if (fileHash != hash) {

            logger->append(this->objectName(), "Precheck: bad checksumm!\n");
            return false;
        }
    }

    return true;
}

LauncherWindow::~LauncherWindow() {

    delete ui;
    delete newsGroup;
}
