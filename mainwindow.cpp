/**
 * @file mainwindow.cpp
 * @brief FTP客户端主窗口实现文件
 * 
 * 本文件实现了FTP客户端的核心功能，包括界面初始化、FTP连接、
 * 目录浏览、文件下载等。使用libcurl库作为底层FTP协议实现。
 */

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>  // 提供文件选择对话框
#include <QMessageBox>  // 提供消息对话框
#include <QTime>        // 用于日志时间戳
#include <QStandardItemModel>  // 用于文件列表的数据模型
#include <QRegularExpression>  // 用于正则表达式解析FTP目录列表
#include <QLabel>       // 用于UI标签
#include <QLineEdit>    // 用于路径输入框
#include <QPushButton>  // 用于按钮
#include <QHBoxLayout>  // 用于水平布局
#include <QTimer>       // 用于下载队列处理
#include <QDir>         // 用于本地目录操作
#include <QProgressDialog>  // 用于显示下载进度
#include <QMutex>       // 用于线程同步

/**
 * @brief 构造函数，初始化UI和各种资源
 * @param parent 父窗口指针
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , curl(nullptr)           // CURL句柄初始化为空
    , headers(nullptr)        // CURL头部列表初始化为空
    , fileModel(new QStandardItemModel(this))  // 创建文件列表模型
    , currentPath("/")        // 初始化当前路径为根目录
    , isConnected(false)      // 初始连接状态为未连接
    , isDownloading(false)    // 初始下载状态为未下载
    , currentDownloadFile(nullptr)  // 当前下载文件初始为空
    , totalBytesReceived(0)   // 初始已接收字节数为0
    , progressDialog(nullptr)  // 初始进度对话框为空
    , directoryTaskCount(0)   // 初始目录任务计数为0
    , downloadQueue(QQueue<DownloadTask>())  // 初始化下载队列
    , downloadMutex(QMutex())  // 初始化互斥锁
{
    ui->setupUi(this);  // 设置UI，加载由Qt Designer生成的界面

    // 初始化 libcurl 全局环境
    // CURL_GLOBAL_ALL表示初始化所有可能的子系统
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();  // 初始化一个CURL句柄
    if (!curl) {
        appendLog("Failed to initialize CURL");  // 如果初始化失败，记录日志
        return;
    }

    // 设置文件树视图的模型
    // 设置表头标题，用于显示文件名、大小、类型和日期
    fileModel->setHorizontalHeaderLabels(QStringList() << "Name" << "Size" << "Type" << "Date");
    ui->fileTreeView->setModel(fileModel);  // 将模型设置到视图
    ui->fileTreeView->setHeaderHidden(false);  // 显示表头
    ui->fileTreeView->setAlternatingRowColors(true);  // 设置行交替颜色，提高可读性
    
    // 调整列宽，优化显示效果
    ui->fileTreeView->setColumnWidth(0, 200); // 名称列宽度
    ui->fileTreeView->setColumnWidth(1, 100); // 大小列宽度
    ui->fileTreeView->setColumnWidth(2, 80);  // 类型列宽度
    ui->fileTreeView->setColumnWidth(3, 150); // 日期列宽度

    // 添加路径导航栏，方便用户查看当前路径和进行导航操作
    QWidget* pathWidget = new QWidget(this);  // 创建路径导航控件容器
    QHBoxLayout* pathLayout = new QHBoxLayout(pathWidget);  // 水平布局
    pathLayout->setContentsMargins(0, 0, 0, 0);  // 设置无边距
    
    // 创建返回上级目录按钮
    QPushButton* backButton = new QPushButton("返回上级", pathWidget);
    backButton->setObjectName("backButton");  // 设置对象名，便于后续查找
    pathLayout->addWidget(backButton);  // 添加到布局
    
    // 创建刷新按钮
    QPushButton* refreshButton = new QPushButton("刷新", pathWidget);
    refreshButton->setObjectName("refreshButton");  // 设置对象名
    pathLayout->addWidget(refreshButton);  // 添加到布局
    
    // 创建路径标签
    QLabel* pathLabel = new QLabel("当前路径:", pathWidget);
    pathLayout->addWidget(pathLabel);  // 添加到布局
    
    // 创建路径显示文本框，用于显示当前FTP路径
    QLineEdit* pathEdit = new QLineEdit(pathWidget);
    pathEdit->setObjectName("pathEdit");  // 设置对象名
    pathEdit->setReadOnly(true);  // 设为只读，防止用户直接编辑
    pathLayout->addWidget(pathEdit);  // 添加到布局
    
    // 将路径导航栏添加到主布局
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(ui->centralwidget->layout());
    if (mainLayout) {
        mainLayout->insertWidget(1, pathWidget); // 插入到连接控件之后的位置
    }

    // 连接信号与槽，建立UI控件与功能函数的关联
    // 当点击连接按钮时，调用onConnectButtonClicked函数
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);
    // 当点击断开连接按钮时，调用onDisconnectButtonClicked函数
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectButtonClicked);
    // 当双击文件树视图中的项时，调用onFileTreeViewDoubleClicked函数
    connect(ui->fileTreeView, &QTreeView::doubleClicked, this, &MainWindow::onFileTreeViewDoubleClicked);
    // 当点击返回上级按钮时，调用onBackButtonClicked函数
    connect(backButton, &QPushButton::clicked, this, &MainWindow::onBackButtonClicked);
    // 当点击刷新按钮时，调用onRefreshButtonClicked函数
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshButtonClicked);
    // 当点击下载按钮时，调用onDownloadButtonClicked函数
    connect(ui->downloadButton, &QPushButton::clicked, this, &MainWindow::onDownloadButtonClicked);

    // 初始化下载定时器，用于异步处理下载队列
    downloadTimer = new QTimer(this);
    // 当定时器触发时，调用processNextDownloadTask函数处理下一个下载任务
    connect(downloadTimer, &QTimer::timeout, this, &MainWindow::processNextDownloadTask);

    // 初始化按钮状态，禁用需要连接后才能使用的按钮
    updateButtonStates(false);  // 传入false表示未连接状态
}

/**
 * @brief 析构函数，释放资源
 * 
 * 清理下载相关资源，关闭打开的文件，释放CURL资源
 */
MainWindow::~MainWindow()
{
    // 清理下载相关资源
    if (currentDownloadFile) {
        currentDownloadFile->close();  // 关闭当前下载文件
        delete currentDownloadFile;    // 释放文件对象
        currentDownloadFile = nullptr;
    }
    
    if (progressDialog) {
        progressDialog->close();       // 关闭进度对话框
        delete progressDialog;         // 释放对话框对象
        progressDialog = nullptr;
    }
    
    if (downloadTimer) {
        downloadTimer->stop();         // 停止下载定时器
    }

    delete ui;                         // 释放UI资源
    if (headers) {
        curl_slist_free_all(headers);  // 释放CURL头部列表
    }
    if (curl) {
        curl_easy_cleanup(curl);       // 清理CURL句柄
    }
    curl_global_cleanup();             // 清理CURL全局环境
}

/**
 * @brief 连接按钮点击事件处理函数
 * 
 * 尝试连接到FTP服务器，成功后更新UI状态并显示根目录内容
 */
void MainWindow::onConnectButtonClicked()
{
    // 调用connectToFtp函数尝试连接FTP服务器
    if (connectToFtp()) {
        isConnected = true;                  // 设置连接标志为true
        updateButtonStates(true);            // 更新按钮状态为已连接
        appendLog("连接成功！");              // 添加成功日志
        currentPath = "/";                   // 设置当前路径为根目录
        directoryHistory.clear();            // 清空目录历史记录
        listDirectory(currentPath);          // 列出根目录内容
    }
}

/**
 * @brief 断开连接按钮点击事件处理函数
 * 
 * 断开与FTP服务器的连接，更新UI状态
 */
void MainWindow::onDisconnectButtonClicked()
{
    disconnectFromFtp();                     // 调用断开连接函数
    isConnected = false;                     // 设置连接标志为false
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

/**
 * @brief 列出目录内容
 * @param path 要列出的目录路径
 * @return 操作是否成功
 * 
 * 获取指定FTP目录的内容，并在文件树视图中显示
 */
bool MainWindow::listDirectory(const QString &path)
{
    if (!curl || !isConnected) return false;  // 如果未连接或CURL句柄为空，返回失败

    // 保存当前路径到历史记录，用于实现返回功能
    if (path != currentPath) {
        directoryHistory.push(currentPath);  // 将当前路径压入历史栈
    }

    // 清空当前列表，准备加载新目录内容
    fileModel->removeRows(0, fileModel->rowCount());  // 清空文件模型
    listBuffer.clear();  // 清空接收缓冲区
    
    // 移除可能存在的\r字符，确保路径格式正确
    QString cleanPath = path;
    cleanPath.remove('\r');  // 移除回车符，避免路径问题
    
    currentPath = cleanPath;  // 更新当前路径
    updatePathDisplay();  // 更新路径显示
    appendLog(QString("浏览目录: %1").arg(cleanPath));  // 记录日志

    // 构建完整的URL，用于FTP请求
    QString server = ui->serverEdit->text();  // 获取服务器地址
    if (!server.startsWith("ftp://")) {
        server = "ftp://" + server;  // 确保URL以ftp://开头
    }
    
    // 确保server不以/结尾，而path以/开头，避免路径问题
    if (server.endsWith("/")) {
        server.chop(1);  // 删除服务器地址末尾的斜杠
    }
    
    // 规范化路径格式
    QString normalizedPath = cleanPath;
    if (!normalizedPath.startsWith("/")) {
        normalizedPath = "/" + normalizedPath;  // 确保路径以/开头
    }
    // 确保路径以/结尾，这对于FTP目录浏览很重要
    if (!normalizedPath.endsWith("/")) {
        normalizedPath += "/";  // 添加路径末尾的斜杠
    }
    
    // 对URL进行编码处理，特别是处理路径中的非ASCII字符（如中文）和特殊字符
    QByteArray pathUtf8 = normalizedPath.toUtf8();  // 转换为UTF-8编码
    char *escapedPath = curl_easy_escape(curl, pathUtf8.constData(), pathUtf8.length());  // URL编码
    
    // 替换掉编码后的斜杠，因为我们需要保留路径结构
    QString encodedPath = QString(escapedPath);
    encodedPath.replace("%2F", "/"); // 把转义的斜杠替换回来，保留路径结构
    
    QString fullUrl = server + encodedPath;  // 构建完整URL
    appendLog(QString("编码后的目录URL: %1").arg(fullUrl));  // 记录URL
    
    // 释放CURL分配的内存，避免内存泄漏
    curl_free(escapedPath);
    
    // 设置CURL选项，准备执行FTP列表请求
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.toUtf8().constData());  // 设置URL
    curl_easy_setopt(curl, CURLOPT_USERNAME, ui->usernameEdit->text().toUtf8().constData());  // 设置用户名
    curl_easy_setopt(curl, CURLOPT_PASSWORD, ui->passwordEdit->text().toUtf8().constData());  // 设置密码
    curl_easy_setopt(curl, CURLOPT_PORT, ui->portSpinBox->value());  // 设置端口
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);  // 设置数据接收回调
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);  // 设置回调用户数据
    curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 0L);  // 获取完整列表，不仅仅是文件名

    // 执行列表命令，获取目录内容
    CURLcode res = curl_easy_perform(curl);  // 执行CURL请求
    if (res != CURLE_OK) {
        // 如果执行失败，记录错误并返回失败
        appendLog(QString("获取目录列表失败: %1").arg(curl_easy_strerror(res)));
        return false;
    }

    // 解析目录列表，转换为文件模型数据
    parseFtpList(listBuffer.join("\n"));  // 合并接收到的数据并解析
    
    // 添加特殊目录项，便于目录导航
    if (path != "/") {
        // 如果不是根目录，添加返回上级目录的条目".."
        QList<QStandardItem*> parentItems;
        parentItems << new QStandardItem("..")
                    << new QStandardItem("")
                    << new QStandardItem("Directory")
                    << new QStandardItem("");
        fileModel->insertRow(0, parentItems);  // 插入到列表最前面
    }
    
    return true;  // 操作成功
}

/**
 * @brief 解析FTP目录列表
 * @param listData 目录列表数据
 * 
 * 解析FTP服务器返回的目录列表数据，处理多种格式
 */
void MainWindow::parseFtpList(const QString &listData)
{
    // 创建用于匹配不同格式的正则表达式
    // 尝试使用标准的Unix格式解析，如：drwxr-xr-x 2 root root 4096 Jun 12 12:00 dirname
    QRegularExpression unixRe("([d-])([rwx-]{9})\\s+(\\d+)\\s+(\\w+)\\s+(\\w+)\\s+(\\d+)\\s+(\\w+\\s+\\d+\\s+[\\d:]+)\\s+(.+)");
    
    // 尝试匹配Windows FTP服务器格式，如：06-12-23 12:00PM <DIR> dirname
    QRegularExpression windowsRe("(\\d{2}-\\d{2}-\\d{2})\\s+(\\d{2}:\\d{2}[AP]M)\\s+(<DIR>|\\d+)\\s+(.+)");
    
    // 匹配简单的文件/目录条目，不包含太多信息
    QRegularExpression simpleRe("([d-])[^\\s]+\\s+.*\\s+(.+)$");
    
    // 按行分割目录列表数据
    QStringList lines = listData.split("\n", Qt::SkipEmptyParts);  // 忽略空行
    appendLog(QString("收到%1行目录数据").arg(lines.count()));  // 记录接收到的数据行数
    
    // 调试输出原始数据，便于排查问题
    if (lines.count() < 50) {  // 只在条目较少时输出，避免刷屏
        appendLog("原始目录数据:");
        for (const QString &line : lines) {
            appendLog("  " + line);  // 输出每一行
        }
    }
    
    // 逐行解析目录列表数据
    for (const QString &line : lines) {
        // 尝试Unix格式匹配
        QRegularExpressionMatch unixMatch = unixRe.match(line);
        if (unixMatch.hasMatch()) {
            // 从匹配结果中提取信息
            QString type = unixMatch.captured(1);  // d表示目录，-表示文件
            QString size = unixMatch.captured(6);  // 文件大小
            QString date = unixMatch.captured(7);  // 日期和时间
            QString name = unixMatch.captured(8).trimmed();  // 文件/目录名
            name.remove('\r');  // 移除可能的回车符
            
            // 创建列表项并添加到模型
            QList<QStandardItem*> items;
            items << new QStandardItem(name)  // 名称列
                  << new QStandardItem(size)  // 大小列
                  << new QStandardItem(type == "d" ? "Directory" : "File")  // 类型列
                  << new QStandardItem(date);  // 日期列
            
            fileModel->appendRow(items);  // 添加到文件模型
            continue;  // 处理下一行
        }
        
        // 尝试Windows格式匹配
        QRegularExpressionMatch windowsMatch = windowsRe.match(line);
        if (windowsMatch.hasMatch()) {
            // 从匹配结果中提取信息
            QString date = windowsMatch.captured(1);  // 日期
            QString time = windowsMatch.captured(2);  // 时间
            QString size = windowsMatch.captured(3);  // 大小或<DIR>
            QString name = windowsMatch.captured(4).trimmed();  // 文件/目录名
            name.remove('\r');  // 移除可能的回车符
            
            bool isDir = size == "<DIR>";  // 判断是否是目录
            
            // 创建列表项并添加到模型
            QList<QStandardItem*> items;
            items << new QStandardItem(name)  // 名称列
                  << new QStandardItem(isDir ? "" : size)  // 大小列，目录显示为空
                  << new QStandardItem(isDir ? "Directory" : "File")  // 类型列
                  << new QStandardItem(date + " " + time);  // 日期列
            
            fileModel->appendRow(items);  // 添加到文件模型
            continue;  // 处理下一行
        }
        
        // 尝试简单格式匹配
        QRegularExpressionMatch simpleMatch = simpleRe.match(line);
        if (simpleMatch.hasMatch()) {
            // 从匹配结果中提取信息
            QString type = simpleMatch.captured(1);  // d表示目录，-表示文件
            QString name = simpleMatch.captured(2).trimmed();  // 文件/目录名
            name.remove('\r');  // 移除可能的回车符
            
            // 创建列表项并添加到模型
            QList<QStandardItem*> items;
            items << new QStandardItem(name)  // 名称列
                  << new QStandardItem("")    // 大小列，无信息
                  << new QStandardItem(type == "d" ? "Directory" : "File")  // 类型列
                  << new QStandardItem("");   // 日期列，无信息
            
            fileModel->appendRow(items);  // 添加到文件模型
            continue;  // 处理下一行
        }
        
        // 如果以上格式都不匹配，尝试简单的分割
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 1) {
            // 尝试从分割结果中提取文件名
            QString name = parts.last().trimmed();  // 取最后一部分作为文件名
            name.remove('\r');  // 移除可能的回车符
            QString type = "Unknown";  // 默认类型为未知
            
            // 根据文件名判断类型，如果包含点，很可能是文件
            if (name.contains(".")) {
                type = "File";  // 设置为文件
            } else {
                type = "Directory";  // 设置为目录
            }
            
            // 创建列表项并添加到模型
            QList<QStandardItem*> items;
            items << new QStandardItem(name)  // 名称列
                  << new QStandardItem("")    // 大小列，无信息
                  << new QStandardItem(type)  // 类型列
                  << new QStandardItem("");   // 日期列，无信息
            
            fileModel->appendRow(items);  // 添加到文件模型
        }
    }
    
    // 记录加载的项目数
    appendLog(QString("已加载 %1 个项目").arg(fileModel->rowCount()));
}

/**
 * @brief 文件树视图双击事件处理函数
 * @param index 被双击的项目索引
 * 
 * 处理用户在文件列表中双击项目的操作：
 * 1. 如果双击目录，进入该目录
 * 2. 如果双击文件，显示文件信息
 */
void MainWindow::onFileTreeViewDoubleClicked(const QModelIndex &index)
{
    // 检查索引是否有效
    if (!index.isValid() || index.row() >= fileModel->rowCount()) {
        return;  // 索引无效，直接返回
    }
    
    // 获取文件/目录名称，并清理可能的特殊字符
    QString name = fileModel->item(index.row(), 0)->text();
    name = name.trimmed();  // 去除首尾空白
    name.remove('\r');  // 移除回车符
    
    // 获取类型（文件或目录）
    QString type = fileModel->item(index.row(), 2)->text();

    // 如果是目录，则导航到该目录
    if (type == "Directory") {
        // 构建新路径 - 确保路径格式正确
        QString newPath;
        
        // 特殊处理返回上级目录
        if (name == "..") {
            // 如果是".."，获取上级目录路径
            newPath = getParentDirectory(currentPath);
        } else if (name == ".") {
            // 当前目录，不变
            newPath = currentPath;
        } else {
            // 普通目录，拼接路径
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
        
        // 确保路径格式正确，以/开头
        if (!newPath.startsWith("/")) {
            newPath = "/" + newPath;
        }
        
        // 移除可能存在的\r字符
        newPath.remove('\r');
        
        // 记录日志并导航到新目录
        appendLog(QString("准备进入目录: %1 (原始名称: '%2')").arg(newPath).arg(name));
        listDirectory(newPath);  // 列出新目录的内容
    } else {
        // 如果是文件，显示文件信息
        QString size = fileModel->item(index.row(), 1)->text();  // 获取文件大小
        QString date = fileModel->item(index.row(), 3)->text();  // 获取文件日期
        appendLog(QString("文件: %1, 大小: %2, 日期: %3").arg(name).arg(size).arg(date));
    }
}

/**
 * @brief CURL写入回调函数
 * @param contents 接收到的数据
 * @param size 数据块大小
 * @param nmemb 数据块数量
 * @param userp 用户数据指针（指向MainWindow实例）
 * @return 实际处理的数据大小
 * 
 * 这是libcurl的数据接收回调函数，当服务器返回数据时被调用
 * 主要用于接收目录列表数据，将数据追加到listBuffer
 */
size_t MainWindow::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    // 计算实际数据大小（字节数）
    size_t realsize = size * nmemb;
    
    // 将userp转换为MainWindow指针
    MainWindow *window = static_cast<MainWindow*>(userp);
    
    // 将接收到的数据转换为QString并追加到listBuffer
    QString data = QString::fromUtf8(static_cast<char*>(contents), realsize);
    window->listBuffer.append(data);
    
    // 返回实际处理的数据大小
    return realsize;
}

/**
 * @brief 更新按钮状态
 * @param connected 是否已连接到FTP服务器
 * 
 * 根据连接状态启用或禁用界面上的各个按钮
 * 连接时：启用下载、断开连接、导航按钮；禁用连接按钮和连接设置
 * 未连接时：禁用下载、断开连接、导航按钮；启用连接按钮和连接设置
 */
void MainWindow::updateButtonStates(bool connected)
{
    // 更新主要按钮状态
    ui->connectButton->setEnabled(!connected);    // 连接按钮在已连接时禁用
    ui->disconnectButton->setEnabled(connected);  // 断开连接按钮在已连接时启用
    
    // 更新连接设置控件状态
    ui->serverEdit->setEnabled(!connected);       // 服务器地址在已连接时禁用
    ui->usernameEdit->setEnabled(!connected);     // 用户名在已连接时禁用
    ui->passwordEdit->setEnabled(!connected);     // 密码在已连接时禁用
    ui->portSpinBox->setEnabled(!connected);      // 端口在已连接时禁用
    
    // 更新功能按钮状态
    ui->downloadButton->setEnabled(connected);    // 下载按钮在已连接时启用
    
    // 同步更新导航按钮状态
    QWidget* backButton = this->findChild<QWidget*>("backButton");
    QWidget* refreshButton = this->findChild<QWidget*>("refreshButton");
    
    // 如果找到这些按钮，更新它们的状态
    if (backButton) backButton->setEnabled(connected);
    if (refreshButton) refreshButton->setEnabled(connected);
}

/**
 * @brief 添加日志信息
 * @param message 日志消息
 * 
 * 在日志文本框中添加一条带时间戳的日志信息
 * 用于记录程序的操作过程和状态变化
 */
void MainWindow::appendLog(const QString &message)
{
    // 在日志文本框中添加带有时间戳的日志信息
    // 格式为：[时间] 消息内容
    ui->logTextEdit->appendPlainText(QString("[%1] %2")
        .arg(QTime::currentTime().toString("hh:mm:ss"))  // 当前时间，格式为小时:分钟:秒
        .arg(message));  // 消息内容
}

/**
 * @brief 下载按钮点击事件处理函数
 * 
 * 当用户点击下载按钮时执行，负责:
 * 1. 获取选中的文件/目录
 * 2. 让用户选择本地保存位置
 * 3. 添加下载任务到队列
 * 4. 启动下载流程
 */
void MainWindow::onDownloadButtonClicked()
{
    if (!isConnected) {
        appendLog("请先连接到FTP服务器");  // 未连接时提示用户
        return;
    }
    
    // 获取选中的项
    QModelIndex currentIndex = ui->fileTreeView->currentIndex();
    if (!currentIndex.isValid()) {
        appendLog("请选择要下载的文件或目录");  // 未选中项时提示用户
        return;
    }
    
    // 获取选中项的信息（名称、类型、大小）
    QString name = fileModel->item(currentIndex.row(), 0)->text().trimmed();
    name.remove('\r');  // 移除回车符
    QString type = fileModel->item(currentIndex.row(), 2)->text();  // 获取类型（文件或目录）
    QString size = fileModel->item(currentIndex.row(), 1)->text();  // 获取大小
    
    // 特殊目录不能下载
    if (name == "." || name == "..") {
        appendLog("不能下载特殊目录");
        return;
    }
    
    // 构建远程路径（FTP服务器上的完整路径）
    QString remotePath;
    if (currentPath == "/") {
        remotePath = "/" + name;  // 根目录下直接拼接
    } else {
        if (currentPath.endsWith("/")) {
            remotePath = currentPath + name;  // 已有斜杠，直接拼接
        } else {
            remotePath = currentPath + "/" + name;  // 添加斜杠再拼接
        }
    }
    
    // 选择本地保存目录，打开文件对话框让用户选择
    QString defaultDir = QDir::homePath() + "/Downloads";  // 默认下载目录
    QString targetDir = QFileDialog::getExistingDirectory(this, 
                                                         "选择保存位置", 
                                                         defaultDir,
                                                         QFileDialog::ShowDirsOnly);
    if (targetDir.isEmpty()) {
        appendLog("取消下载");  // 用户取消选择，终止下载
        return;
    }
    
    // 构建本地保存路径
    QString localPath = targetDir + "/" + name;
    
    // 判断下载类型（文件或目录）
    bool isDir = (type == "Directory");
    
    // 确保目录路径以/结尾，这对于FTP目录很重要
    if (isDir && !remotePath.endsWith("/")) {
        remotePath += "/";
    }
    
    // 记录下载信息
    appendLog(QString("准备下载: %1 -> %2").arg(remotePath).arg(localPath));
    
    // 创建进度对话框，显示下载进度
    if (!progressDialog) {
        progressDialog = new QProgressDialog("准备下载...", "取消", 0, 100, this);
        progressDialog->setWindowModality(Qt::WindowModal);  // 模态对话框
        progressDialog->setAutoClose(false);  // 不自动关闭
        progressDialog->setAutoReset(false);  // 不自动重置
        
        // 连接取消按钮的信号，实现取消下载功能
        connect(progressDialog, &QProgressDialog::canceled, [this]() {
            appendLog("下载已取消");
            if (currentDownloadFile) {
                currentDownloadFile->close();  // 关闭当前下载文件
                delete currentDownloadFile;    // 释放资源
                currentDownloadFile = nullptr;
            }
            downloadQueue.clear();     // 清空下载队列
            isDownloading = false;     // 重置下载状态
            downloadTimer->stop();     // 停止下载定时器
            progressDialog->close();   // 关闭进度对话框
        });
    }
    
    // 显示进度对话框
    progressDialog->setLabelText("准备下载...");
    progressDialog->setValue(0);
    progressDialog->show();
    
    // 添加下载任务，区分文件和目录
    if (isDir) {
        // 如果是目录，先创建本地目录结构
        if (!createLocalDirectory(localPath)) {
            appendLog(QString("无法创建本地目录: %1").arg(localPath));
            return;
        }
        // 下载整个目录及其内容
        directoryTaskCount = 0;  // 重置目录任务计数
        downloadDirectory(remotePath, localPath);  // 递归下载目录
    } else {
        // 如果是文件，直接添加下载任务
        qint64 fileSize = size.toLongLong();  // 获取文件大小
        addDownloadTask(remotePath, localPath, false, name, fileSize);  // 添加文件下载任务
    }
    
    // 启动下载队列处理
    if (!isDownloading) {
        isDownloading = true;  // 标记为正在下载
        downloadTimer->start(100);  // 每100毫秒处理一次队列，实现异步下载
    }
}

/**
 * @brief 处理下载队列中的下一个任务
 * 
 * 从下载队列中取出一个任务进行处理
 * 如果是目录，创建目录结构
 * 如果是文件，下载文件内容
 */
void MainWindow::processNextDownloadTask()
{
    // 如果队列为空，停止下载
    if (downloadQueue.isEmpty()) {
        downloadTimer->stop();     // 停止定时器
        isDownloading = false;     // 重置下载状态
        
        if (progressDialog) {
            progressDialog->setLabelText("下载完成");  // 更新进度对话框
            progressDialog->setValue(100);             // 进度设为100%
        }
        
        appendLog("所有下载任务完成");  // 记录完成信息
        return;
    }
    
    // 获取下一个任务
    QMutexLocker locker(&downloadMutex);  // 加锁保护队列访问
    DownloadTask task = downloadQueue.dequeue();  // 取出下一个任务
    locker.unlock();  // 解锁
    
    // 更新进度对话框显示当前任务
    if (progressDialog) {
        progressDialog->setLabelText(QString("正在下载: %1").arg(task.displayName));
    }
    
    // 根据任务类型处理
    if (task.isDirectory) {
        // 如果是目录任务，创建本地目录
        createLocalDirectory(task.localPath);
        // 目录创建后立即处理下一个任务，不需要等待
        QTimer::singleShot(0, this, &MainWindow::processNextDownloadTask);
    } else {
        // 如果是文件任务，下载文件内容
        downloadFile(task.remotePath, task.localPath);
    }
}

/**
 * @brief 更新下载进度显示
 * @param bytesReceived 已接收字节数
 * @param bytesTotal 总字节数
 * 
 * 根据下载进度更新进度对话框
 */
void MainWindow::updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (!progressDialog) {
        return;  // 如果进度对话框不存在，直接返回
    }
    
    // 计算下载百分比
    int percent = 0;
    if (bytesTotal > 0) {
        percent = static_cast<int>((bytesReceived * 100) / bytesTotal);  // 计算百分比
    }
    
    // 更新进度条显示
    progressDialog->setValue(percent);
}

/**
 * @brief 创建本地目录
 * @param localPath 要创建的本地目录路径
 * @return 是否创建成功
 * 
 * 创建本地目录，包括多级目录结构
 */
bool MainWindow::createLocalDirectory(const QString &localPath)
{
    QDir dir;
    if (!dir.exists(localPath)) {
        // 如果目录不存在，递归创建
        if (!dir.mkpath(localPath)) {
            appendLog(QString("创建目录失败: %1").arg(localPath));
            return false;  // 创建失败
        }
        appendLog(QString("创建目录: %1").arg(localPath));  // 记录创建成功
    }
    return true;  // 目录已存在或创建成功
}

/**
 * @brief 下载文件
 * @param remotePath 远程文件路径
 * @param localPath 本地保存路径
 * @return 是否下载成功
 * 
 * 从FTP服务器下载单个文件
 */
bool MainWindow::downloadFile(const QString &remotePath, const QString &localPath)
{
    if (!curl || !isConnected) {
        appendLog("未连接到FTP服务器");
        return false;  // 未连接，无法下载
    }
    
    appendLog(QString("下载文件: %1 -> %2").arg(remotePath).arg(localPath));
    
    // 创建并打开本地文件，准备写入数据
    currentDownloadFile = new QFile(localPath);
    if (!currentDownloadFile->open(QIODevice::WriteOnly)) {
        appendLog(QString("无法创建本地文件: %1").arg(localPath));
        delete currentDownloadFile;
        currentDownloadFile = nullptr;
        return false;  // 无法创建本地文件
    }
    
    // 重置已接收字节计数
    totalBytesReceived = 0;
    
    // 构建完整的FTP URL
    QString server = ui->serverEdit->text();
    if (!server.startsWith("ftp://")) {
        server = "ftp://" + server;  // 确保URL以ftp://开头
    }
    
    // 确保服务器地址不以斜杠结尾
    if (server.endsWith("/")) {
        server.chop(1);  // 删除末尾斜杠
    }
    
    // 确保远程路径以斜杠开头
    QString normalizedPath = remotePath;
    if (!normalizedPath.startsWith("/")) {
        normalizedPath = "/" + normalizedPath;
    }
    
    // 对URL进行编码处理，特别是处理文件名中的非ASCII字符（如中文）和特殊字符
    QByteArray pathUtf8 = normalizedPath.toUtf8();
    char *escapedPath = curl_easy_escape(curl, pathUtf8.constData(), pathUtf8.length());
    
    // 替换掉编码后的斜杠，因为我们需要保留路径结构
    QString encodedPath = QString(escapedPath);
    encodedPath.replace("%2F", "/"); // 把转义的斜杠替换回来，保留路径结构
    
    QString fullUrl = server + encodedPath;  // 构建完整URL
    appendLog(QString("编码后的URL: %1").arg(fullUrl));  // 记录URL
    
    // 释放CURL分配的内存
    curl_free(escapedPath);
    
    // 设置CURL选项，准备下载
    CURL *downloadHandle = curl_easy_init();
    if (!downloadHandle) {
        appendLog("无法初始化CURL下载句柄");
        currentDownloadFile->close();
        delete currentDownloadFile;
        currentDownloadFile = nullptr;
        return false;  // CURL初始化失败
    }
    
    // 配置CURL下载选项
    curl_easy_setopt(downloadHandle, CURLOPT_URL, fullUrl.toUtf8().constData());  // 设置URL
    curl_easy_setopt(downloadHandle, CURLOPT_PORT, ui->portSpinBox->value());     // 设置端口
    curl_easy_setopt(downloadHandle, CURLOPT_USERNAME, ui->usernameEdit->text().toUtf8().constData());  // 设置用户名
    curl_easy_setopt(downloadHandle, CURLOPT_PASSWORD, ui->passwordEdit->text().toUtf8().constData());  // 设置密码
    curl_easy_setopt(downloadHandle, CURLOPT_WRITEFUNCTION, DownloadCallback);    // 设置数据接收回调
    curl_easy_setopt(downloadHandle, CURLOPT_WRITEDATA, this);                    // 设置回调用户数据
    curl_easy_setopt(downloadHandle, CURLOPT_VERBOSE, 1L);                        // 启用详细输出，便于调试
    
    // 执行下载
    CURLcode res = curl_easy_perform(downloadHandle);  // 执行CURL请求
    
    // 关闭文件和清理资源
    currentDownloadFile->close();      // 关闭文件
    delete currentDownloadFile;        // 释放文件对象
    currentDownloadFile = nullptr;     // 重置指针
    
    // 清理CURL句柄
    curl_easy_cleanup(downloadHandle);
    
    // 处理下载结果
    if (res != CURLE_OK) {
        // 下载失败
        appendLog(QString("下载失败: %1").arg(curl_easy_strerror(res)));
        return false;
    }
    
    // 下载成功
    appendLog(QString("下载完成: %1").arg(localPath));
    
    // 处理下一个任务
    QTimer::singleShot(0, this, &MainWindow::processNextDownloadTask);
    
    return true;  // 下载成功
}

/**
 * @brief 下载文件回调函数
 * @param contents 接收到的数据
 * @param size 数据块大小
 * @param nmemb 数据块数量
 * @param userp 用户数据指针（指向MainWindow实例）
 * @return 实际写入的数据大小
 * 
 * 这是libcurl的下载数据接收回调函数
 * 当下载数据到达时，将数据写入本地文件并更新进度
 */
size_t MainWindow::DownloadCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    // 将userp转换为MainWindow指针
    MainWindow *window = static_cast<MainWindow*>(userp);
    size_t realsize = size * nmemb;  // 计算实际数据大小
    
    if (window && window->currentDownloadFile) {
        // 如果有效，写入文件
        qint64 written = window->currentDownloadFile->write(static_cast<char*>(contents), realsize);
        
        // 更新下载进度
        window->totalBytesReceived += written;  // 累加已接收字节数
        window->updateDownloadProgress(window->totalBytesReceived, window->currentDownloadFile->size());
        
        return written;  // 返回实际写入的字节数
    }
    
    return 0;  // 无法写入，返回0
}

bool MainWindow::downloadDirectory(const QString &remotePath, const QString &localPath)
{
    appendLog(QString("下载目录: %1 -> %2").arg(remotePath).arg(localPath));
    
    // 添加目录任务，用于创建目录结构
    addDownloadTask(remotePath, localPath, true);
    
    directoryTaskCount++; // 增加目录计数
    
    // 列出目录内容并添加到下载队列
    return listDirectoryForDownload(remotePath, localPath);
}

bool MainWindow::listDirectoryForDownload(const QString &path, const QString &targetDir)
{
    if (!curl || !isConnected) return false;
    
    // 清空列表缓冲区
    listBuffer.clear();
    
    // 构建完整的URL
    QString server = ui->serverEdit->text();
    if (!server.startsWith("ftp://")) {
        server = "ftp://" + server;
    }
    
    // 确保server不以/结尾，而path以/开头
    if (server.endsWith("/")) {
        server.chop(1);
    }
    
    QString normalizedPath = path;
    if (!normalizedPath.startsWith("/")) {
        normalizedPath = "/" + normalizedPath;
    }
    
    // 确保路径以/结尾，这对于FTP目录浏览很重要
    if (!normalizedPath.endsWith("/")) {
        normalizedPath += "/";
    }
    
    // 对URL进行编码处理，特别是处理路径中的非ASCII字符（如中文）和特殊字符
    QByteArray pathUtf8 = normalizedPath.toUtf8();
    char *escapedPath = curl_easy_escape(curl, pathUtf8.constData(), pathUtf8.length());
    
    // 替换掉编码后的斜杠，因为我们需要保留路径结构
    QString encodedPath = QString(escapedPath);
    encodedPath.replace("%2F", "/"); // 把转义的斜杠替换回来，保留路径结构
    
    QString fullUrl = server + encodedPath;
    appendLog(QString("编码后的目录URL: %1").arg(fullUrl));
    
    // 释放CURL分配的内存
    curl_free(escapedPath);
    
    // 设置CURL选项
    CURL *listHandle = curl_easy_init();
    if (!listHandle) {
        appendLog("无法初始化CURL列表句柄");
        return false;
    }
    
    curl_easy_setopt(listHandle, CURLOPT_URL, fullUrl.toUtf8().constData());
    curl_easy_setopt(listHandle, CURLOPT_USERNAME, ui->usernameEdit->text().toUtf8().constData());
    curl_easy_setopt(listHandle, CURLOPT_PASSWORD, ui->passwordEdit->text().toUtf8().constData());
    curl_easy_setopt(listHandle, CURLOPT_PORT, ui->portSpinBox->value());
    curl_easy_setopt(listHandle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(listHandle, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(listHandle, CURLOPT_DIRLISTONLY, 0L); // 获取详细列表
    
    // 执行列表命令
    CURLcode res = curl_easy_perform(listHandle);
    
    // 清理CURL句柄
    curl_easy_cleanup(listHandle);
    
    if (res != CURLE_OK) {
        appendLog(QString("获取目录列表失败: %1").arg(curl_easy_strerror(res)));
        return false;
    }
    
    // 解析目录列表并添加到下载队列
    QString listData = listBuffer.join("\n");
    QStringList lines = listData.split("\n", Qt::SkipEmptyParts);
    
    // 使用与parseFtpList相似的正则表达式解析文件列表
    QRegularExpression unixRe("([d-])([rwx-]{9})\\s+(\\d+)\\s+(\\w+)\\s+(\\w+)\\s+(\\d+)\\s+(\\w+\\s+\\d+\\s+[\\d:]+)\\s+(.+)");
    QRegularExpression windowsRe("(\\d{2}-\\d{2}-\\d{2})\\s+(\\d{2}:\\d{2}[AP]M)\\s+(<DIR>|\\d+)\\s+(.+)");
    QRegularExpression simpleRe("([d-])[^\\s]+\\s+.*\\s+(.+)$");
    
    for (const QString &line : lines) {
        bool isDir = false;
        QString name;
        qint64 fileSize = 0;
        
        // 尝试Unix格式匹配
        QRegularExpressionMatch unixMatch = unixRe.match(line);
        if (unixMatch.hasMatch()) {
            QString type = unixMatch.captured(1);
            fileSize = unixMatch.captured(6).toLongLong();
            name = unixMatch.captured(8).trimmed();
            name.remove('\r');
            isDir = (type == "d");
            
            // 跳过特殊目录
            if (name == "." || name == "..") {
                continue;
            }
        } else {
            // 尝试Windows格式匹配
            QRegularExpressionMatch windowsMatch = windowsRe.match(line);
            if (windowsMatch.hasMatch()) {
                QString size = windowsMatch.captured(3);
                name = windowsMatch.captured(4).trimmed();
                name.remove('\r');
                isDir = (size == "<DIR>");
                
                if (!isDir) {
                    fileSize = size.toLongLong();
                }
                
                // 跳过特殊目录
                if (name == "." || name == "..") {
                    continue;
                }
            } else {
                // 尝试简单格式匹配
                QRegularExpressionMatch simpleMatch = simpleRe.match(line);
                if (simpleMatch.hasMatch()) {
                    QString type = simpleMatch.captured(1);
                    name = simpleMatch.captured(2).trimmed();
                    name.remove('\r');
                    isDir = (type == "d");
                    
                    // 跳过特殊目录
                    if (name == "." || name == "..") {
                        continue;
                    }
                } else {
                    // 如果以上格式都不匹配，尝试简单的分割
                    QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    if (parts.size() >= 1) {
                        name = parts.last().trimmed();
                        name.remove('\r');
                        
                        // 如果文件名中包含点，可能是文件，否则可能是目录
                        isDir = !name.contains(".");
                        
                        // 跳过特殊目录
                        if (name == "." || name == "..") {
                            continue;
                        }
                    } else {
                        continue; // 无法解析的行
                    }
                }
            }
        }
        
        // 跳过空名称
        if (name.isEmpty()) {
            continue;
        }
        
        // 构建远程路径和本地路径
        QString itemRemotePath = normalizedPath + name;
        if (isDir && !itemRemotePath.endsWith("/")) {
            itemRemotePath += "/";
        }
        
        QString itemLocalPath = targetDir + "/" + name;
        
        if (isDir) {
            // 递归添加目录任务
            directoryTaskCount++; // 增加目录计数
            createLocalDirectory(itemLocalPath);
            addDownloadTask(itemRemotePath, itemLocalPath, true, name);
            
            // 递归处理子目录
            listDirectoryForDownload(itemRemotePath, itemLocalPath);
        } else {
            // 添加文件下载任务
            addDownloadTask(itemRemotePath, itemLocalPath, false, name, fileSize);
        }
    }
    
    // 减少目录计数
    directoryTaskCount--;
    appendLog(QString("剩余目录任务: %1").arg(directoryTaskCount));
    
    return true;
}

void MainWindow::addDownloadTask(const QString &remotePath, const QString &localPath, 
                               bool isDirectory, const QString &displayName, qint64 fileSize)
{
    QMutexLocker locker(&downloadMutex);
    
    DownloadTask task;
    task.remotePath = remotePath;
    task.localPath = localPath;
    task.isDirectory = isDirectory;
    task.fileSize = fileSize;
    
    // 如果没有提供显示名称，从路径中提取
    if (displayName.isEmpty()) {
        QFileInfo fileInfo(remotePath);
        task.displayName = fileInfo.fileName();
    } else {
        task.displayName = displayName;
    }
    
    downloadQueue.enqueue(task);
    
    appendLog(QString("添加%1任务: %2").arg(isDirectory ? "目录" : "文件").arg(task.displayName));
}
