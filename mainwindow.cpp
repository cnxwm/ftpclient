/**
 * @file mainwindow.cpp
 * @brief FTP客户端主窗口实现文件
 */

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <QStandardItemModel>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , curl(nullptr)
    , headers(nullptr)
    , fileModel(new QStandardItemModel(this))
    , currentPath("/")
    , isConnected(false)
{
    ui->setupUi(this);

    // 初始化 libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl) {
        appendLog("Failed to initialize CURL");
        return;
    }

    // 设置文件树视图的模型
    fileModel->setHorizontalHeaderLabels(QStringList() << "Name" << "Size" << "Type" << "Date");
    ui->fileTreeView->setModel(fileModel);
    ui->fileTreeView->setHeaderHidden(false);
    ui->fileTreeView->setAlternatingRowColors(true);

    // 连接信号
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectButtonClicked);
    connect(ui->fileTreeView, &QTreeView::doubleClicked, this, &MainWindow::onFileTreeViewDoubleClicked);

    // 初始化按钮状态
    updateButtonStates(false);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (headers) {
        curl_slist_free_all(headers);
    }
    if (curl) {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

void MainWindow::onConnectButtonClicked()
{
    if (connectToFtp()) {
        isConnected = true;
        updateButtonStates(true);
        appendLog("连接成功！");
        listDirectory(); // 连接成功后立即列出根目录
    }
}

void MainWindow::onDisconnectButtonClicked()
{
    disconnectFromFtp();
    isConnected = false;
    updateButtonStates(false);
    appendLog("已断开连接");
}

bool MainWindow::connectToFtp()
{
    QString server = ui->serverEdit->text();
    if (server.isEmpty()) {
        appendLog("请输入FTP服务器地址");
        return false;
    }

    // 构建FTP URL
    QString ftpUrl = server;
    if (!ftpUrl.startsWith("ftp://")) {
        ftpUrl = "ftp://" + ftpUrl;
    }

    // 设置CURL选项
    curl_easy_setopt(curl, CURLOPT_URL, ftpUrl.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_PORT, ui->portSpinBox->value());
    curl_easy_setopt(curl, CURLOPT_USERNAME, ui->usernameEdit->text().toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, ui->passwordEdit->text().toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

    // 执行连接测试
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        appendLog(QString("连接失败: %1").arg(curl_easy_strerror(res)));
        return false;
    }

    return true;
}

void MainWindow::disconnectFromFtp()
{
    if (curl) {
        curl_easy_cleanup(curl);
        curl = curl_easy_init();
    }
}

bool MainWindow::listDirectory(const QString &path)
{
    if (!curl || !isConnected) return false;

    // 清空当前列表
    fileModel->removeRows(0, fileModel->rowCount());
    listBuffer.clear();
    currentPath = path;

    // 构建完整的URL
    QString server = ui->serverEdit->text();
    if (!server.startsWith("ftp://")) {
        server = "ftp://" + server;
    }
    QString fullUrl = server + path;

    // 设置CURL选项
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_USERNAME, ui->usernameEdit->text().toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, ui->passwordEdit->text().toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_PORT, ui->portSpinBox->value());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 0L);

    // 执行列表命令
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        appendLog(QString("获取目录列表失败: %1").arg(curl_easy_strerror(res)));
        return false;
    }

    // 解析目录列表
    parseFtpList(listBuffer.join("\n"));
    return true;
}

void MainWindow::parseFtpList(const QString &listData)
{
    QRegularExpression re("([d-])([rwx-]{9})\\s+(\\d+)\\s+(\\w+)\\s+(\\w+)\\s+(\\d+)\\s+(\\w+\\s+\\d+\\s+[\\d:]+)\\s+(.+)");
    QStringList lines = listData.split("\n", Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            QString type = match.captured(1);
            QString size = match.captured(3);
            QString date = match.captured(7);
            QString name = match.captured(8);

            QList<QStandardItem*> items;
            items << new QStandardItem(name)
                  << new QStandardItem(size)
                  << new QStandardItem(type == "d" ? "Directory" : "File")
                  << new QStandardItem(date);

            fileModel->appendRow(items);
        }
    }
}

void MainWindow::onFileTreeViewDoubleClicked(const QModelIndex &index)
{
    QString name = fileModel->item(index.row(), 0)->text();
    QString type = fileModel->item(index.row(), 2)->text();

    if (type == "Directory") {
        QString newPath = currentPath;
        if (newPath.endsWith("/")) {
            newPath += name;
        } else {
            newPath += "/" + name;
        }
        listDirectory(newPath);
    }
}

size_t MainWindow::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    MainWindow *window = static_cast<MainWindow*>(userp);
    size_t realsize = size * nmemb;
    QString data = QString::fromUtf8(static_cast<char*>(contents), realsize);
    window->listBuffer.append(data);
    return realsize;
}

void MainWindow::updateButtonStates(bool connected)
{
    ui->connectButton->setEnabled(!connected);
    ui->disconnectButton->setEnabled(connected);
    ui->serverEdit->setEnabled(!connected);
    ui->usernameEdit->setEnabled(!connected);
    ui->passwordEdit->setEnabled(!connected);
    ui->portSpinBox->setEnabled(!connected);
}

void MainWindow::appendLog(const QString &message)
{
    ui->logTextEdit->appendPlainText(QString("[%1] %2")
        .arg(QTime::currentTime().toString("hh:mm:ss"))
        .arg(message));
}
