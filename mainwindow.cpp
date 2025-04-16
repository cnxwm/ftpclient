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
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

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
    
    // 调整列宽
    ui->fileTreeView->setColumnWidth(0, 200); // 名称列
    ui->fileTreeView->setColumnWidth(1, 100); // 大小列
    ui->fileTreeView->setColumnWidth(2, 80);  // 类型列
    ui->fileTreeView->setColumnWidth(3, 150); // 日期列

    // 添加路径导航栏
    QWidget* pathWidget = new QWidget(this);
    QHBoxLayout* pathLayout = new QHBoxLayout(pathWidget);
    pathLayout->setContentsMargins(0, 0, 0, 0);
    
    QPushButton* backButton = new QPushButton("返回上级", pathWidget);
    backButton->setObjectName("backButton");
    pathLayout->addWidget(backButton);
    
    QPushButton* refreshButton = new QPushButton("刷新", pathWidget);
    refreshButton->setObjectName("refreshButton");
    pathLayout->addWidget(refreshButton);
    
    QLabel* pathLabel = new QLabel("当前路径:", pathWidget);
    pathLayout->addWidget(pathLabel);
    
    QLineEdit* pathEdit = new QLineEdit(pathWidget);
    pathEdit->setObjectName("pathEdit");
    pathEdit->setReadOnly(true);
    pathLayout->addWidget(pathEdit);
    
    // 将路径导航栏添加到布局
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(ui->centralwidget->layout());
    if (mainLayout) {
        mainLayout->insertWidget(1, pathWidget); // 插入到连接控件之后
    }

    // 连接信号
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectButtonClicked);
    connect(ui->fileTreeView, &QTreeView::doubleClicked, this, &MainWindow::onFileTreeViewDoubleClicked);
    connect(backButton, &QPushButton::clicked, this, &MainWindow::onBackButtonClicked);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshButtonClicked);

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
        currentPath = "/";
        directoryHistory.clear();
        listDirectory(currentPath); // 连接成功后立即列出根目录
    }
}

void MainWindow::onDisconnectButtonClicked()
{
    disconnectFromFtp();
    isConnected = false;
    updateButtonStates(false);
    appendLog("已断开连接");
    currentPath = "/";
    directoryHistory.clear();
    fileModel->removeRows(0, fileModel->rowCount());
    updatePathDisplay();
}

void MainWindow::onBackButtonClicked()
{
    if (!isConnected) return;
    
    QString parentDir = getParentDirectory(currentPath);
    if (parentDir != currentPath) {
        // 如果不是根目录，则导航到上级目录
        listDirectory(parentDir);
    }
}

void MainWindow::onRefreshButtonClicked()
{
    if (isConnected) {
        listDirectory(currentPath);
    }
}

QString MainWindow::getParentDirectory(const QString &path)
{
    if (path == "/" || path.isEmpty()) {
        return "/";
    }
    
    QString workPath = path;
    // 如果路径已经以/结尾，先去掉它再处理
    if (workPath.endsWith('/')) {
        workPath.chop(1);
    }
    
    int lastSlash = workPath.lastIndexOf('/');
    if (lastSlash == 0) {
        // 如果是根目录下的文件夹，返回根目录
        return "/";
    } else if (lastSlash > 0) {
        // 返回上级目录，确保以/结尾
        QString parentDir = workPath.left(lastSlash);
        if (!parentDir.endsWith('/')) {
            parentDir += '/';
        }
        return parentDir;
    }
    
    return "/";
}

void MainWindow::updatePathDisplay()
{
    QLineEdit* pathEdit = this->findChild<QLineEdit*>("pathEdit");
    if (pathEdit) {
        pathEdit->setText(currentPath);
    }
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

    // 保存当前路径到历史记录
    if (path != currentPath) {
        directoryHistory.push(currentPath);
    }

    // 清空当前列表
    fileModel->removeRows(0, fileModel->rowCount());
    listBuffer.clear();
    
    // 移除可能存在的\r字符
    QString cleanPath = path;
    cleanPath.remove('\r');
    
    currentPath = cleanPath;
    updatePathDisplay();
    appendLog(QString("浏览目录: %1").arg(cleanPath));

    // 构建完整的URL
    QString server = ui->serverEdit->text();
    if (!server.startsWith("ftp://")) {
        server = "ftp://" + server;
    }
    
    // 确保server不以/结尾，而path以/开头
    if (server.endsWith("/")) {
        server.chop(1);
    }
    
    QString normalizedPath = cleanPath;
    if (!normalizedPath.startsWith("/")) {
        normalizedPath = "/" + normalizedPath;
    }
    // 确保路径以/结尾，这对于FTP目录浏览很重要
    if (!normalizedPath.endsWith("/")) {
        normalizedPath += "/";
    }
    
    QString fullUrl = server + normalizedPath;
    appendLog(QString("URL: %1").arg(fullUrl));

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
    
    // 添加特殊目录项
    if (path != "/") {
        // 添加返回上级目录的条目
        QList<QStandardItem*> parentItems;
        parentItems << new QStandardItem("..")
                    << new QStandardItem("")
                    << new QStandardItem("Directory")
                    << new QStandardItem("");
        fileModel->insertRow(0, parentItems);
    }
    
    return true;
}

void MainWindow::parseFtpList(const QString &listData)
{
    // 尝试使用标准的Unix格式解析
    QRegularExpression unixRe("([d-])([rwx-]{9})\\s+(\\d+)\\s+(\\w+)\\s+(\\w+)\\s+(\\d+)\\s+(\\w+\\s+\\d+\\s+[\\d:]+)\\s+(.+)");
    // 尝试匹配Windows FTP服务器格式
    QRegularExpression windowsRe("(\\d{2}-\\d{2}-\\d{2})\\s+(\\d{2}:\\d{2}[AP]M)\\s+(<DIR>|\\d+)\\s+(.+)");
    // 匹配简单的文件/目录条目
    QRegularExpression simpleRe("([d-])[^\\s]+\\s+.*\\s+(.+)$");
    
    QStringList lines = listData.split("\n", Qt::SkipEmptyParts);
    appendLog(QString("收到%1行目录数据").arg(lines.count()));
    
    // 调试输出原始数据
    if (lines.count() < 50) {  // 只在条目较少时输出，避免刷屏
        appendLog("原始目录数据:");
        for (const QString &line : lines) {
            appendLog("  " + line);
        }
    }
    
    for (const QString &line : lines) {
        // 尝试Unix格式匹配
        QRegularExpressionMatch unixMatch = unixRe.match(line);
        if (unixMatch.hasMatch()) {
            QString type = unixMatch.captured(1);
            QString size = unixMatch.captured(6);
            QString date = unixMatch.captured(7);
            QString name = unixMatch.captured(8).trimmed();
            name.remove('\r');
            
            QList<QStandardItem*> items;
            items << new QStandardItem(name)
                  << new QStandardItem(size)
                  << new QStandardItem(type == "d" ? "Directory" : "File")
                  << new QStandardItem(date);
            
            fileModel->appendRow(items);
            continue;
        }
        
        // 尝试Windows格式匹配
        QRegularExpressionMatch windowsMatch = windowsRe.match(line);
        if (windowsMatch.hasMatch()) {
            QString date = windowsMatch.captured(1);
            QString time = windowsMatch.captured(2);
            QString size = windowsMatch.captured(3);
            QString name = windowsMatch.captured(4).trimmed();
            name.remove('\r');
            
            bool isDir = size == "<DIR>";
            
            QList<QStandardItem*> items;
            items << new QStandardItem(name)
                  << new QStandardItem(isDir ? "" : size)
                  << new QStandardItem(isDir ? "Directory" : "File")
                  << new QStandardItem(date + " " + time);
            
            fileModel->appendRow(items);
            continue;
        }
        
        // 尝试简单格式匹配
        QRegularExpressionMatch simpleMatch = simpleRe.match(line);
        if (simpleMatch.hasMatch()) {
            QString type = simpleMatch.captured(1);
            QString name = simpleMatch.captured(2).trimmed();
            name.remove('\r');
            
            QList<QStandardItem*> items;
            items << new QStandardItem(name)
                  << new QStandardItem("")
                  << new QStandardItem(type == "d" ? "Directory" : "File")
                  << new QStandardItem("");
            
            fileModel->appendRow(items);
            continue;
        }
        
        // 如果以上格式都不匹配，尝试简单的分割
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 1) {
            QString name = parts.last().trimmed();
            name.remove('\r');
            QString type = "Unknown";
            
            // 如果文件名中包含点，可能是文件，否则可能是目录
            if (name.contains(".")) {
                type = "File";
            } else {
                type = "Directory";
            }
            
            QList<QStandardItem*> items;
            items << new QStandardItem(name)
                  << new QStandardItem("")
                  << new QStandardItem(type)
                  << new QStandardItem("");
            
            fileModel->appendRow(items);
        }
    }
    
    appendLog(QString("已加载 %1 个项目").arg(fileModel->rowCount()));
}

void MainWindow::onFileTreeViewDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid() || index.row() >= fileModel->rowCount()) {
        return;
    }
    
    QString name = fileModel->item(index.row(), 0)->text();
    // 移除可能存在的\r字符
    name = name.trimmed();
    name.remove('\r');
    
    QString type = fileModel->item(index.row(), 2)->text();

    if (type == "Directory") {
        // 构建新路径 - 确保路径格式正确
        QString newPath;
        
        // 特殊处理返回上级目录
        if (name == "..") {
            newPath = getParentDirectory(currentPath);
        } else if (name == ".") {
            // 当前目录，不变
            newPath = currentPath;
        } else {
            // 普通目录
            if (currentPath == "/") {
                //末尾加上 /
                newPath = "/" + name + "/";
            } else {
                if (currentPath.endsWith("/")) {
                    newPath = currentPath + name + "/";
                } else {
                    newPath = currentPath + "/" + name+ "/";
                }
            }
        }
        
        // 确保路径格式正确
        if (!newPath.startsWith("/")) {
            newPath = "/" + newPath;
        }
        
        // 移除可能存在的\r字符
        newPath.remove('\r');
        
        appendLog(QString("准备进入目录: %1 (原始名称: '%2')").arg(newPath).arg(name));
        
        listDirectory(newPath);
    } else {
        // 如果是文件，可以显示文件信息
        QString size = fileModel->item(index.row(), 1)->text();
        QString date = fileModel->item(index.row(), 3)->text();
        appendLog(QString("文件: %1, 大小: %2, 日期: %3").arg(name).arg(size).arg(date));
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
    
    // 同步更新导航按钮状态
    QWidget* backButton = this->findChild<QWidget*>("backButton");
    QWidget* refreshButton = this->findChild<QWidget*>("refreshButton");
    
    if (backButton) backButton->setEnabled(connected);
    if (refreshButton) refreshButton->setEnabled(connected);
}

void MainWindow::appendLog(const QString &message)
{
    ui->logTextEdit->appendPlainText(QString("[%1] %2")
        .arg(QTime::currentTime().toString("hh:mm:ss"))
        .arg(message));
}
