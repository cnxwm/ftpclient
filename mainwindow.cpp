/**
 * @file mainwindow.cpp
 * @brief FTP客户端主窗口实现文件
 * 
 * 本文件实现了FTP客户端的核心功能，包括界面初始化、FTP连接、
 * 目录浏览、文件下载等。使用FtpClient类作为底层FTP协议实现。
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
    , ftpClient(new FtpClient())      // 创建FTP客户端对象
    , fileModel(new QStandardItemModel(this))  // 创建文件列表模型
    , currentPath("/")                // 初始化当前路径为根目录
    , isConnected(false)              // 初始连接状态为未连接
    , isDownloading(false)            // 初始下载状态为未下载
    , progressDialog(nullptr)         // 初始进度对话框为空
    , directoryTaskCount(0)           // 初始目录任务计数为0
    , downloadQueue(QQueue<DownloadTask>())  // 初始化下载队列
    , downloadMutex(QMutex())         // 初始化互斥锁
{
    ui->setupUi(this);  // 设置UI，加载由Qt Designer生成的界面

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
 * 清理下载相关资源，关闭打开的文件，释放资源
 */
MainWindow::~MainWindow()
{
    // 清理下载相关资源
    if (progressDialog) {
        progressDialog->close();       // 关闭进度对话框
        delete progressDialog;         // 释放对话框对象
        progressDialog = nullptr;
    }
    
    if (downloadTimer) {
        downloadTimer->stop();         // 停止下载定时器
    }

    delete ui;                         // 释放UI资源
    delete ftpClient;                  // 释放FTP客户端对象
}

/**
 * @brief 连接按钮点击事件处理函数
 * 
 * 尝试连接到FTP服务器，成功后更新UI状态并显示根目录内容
 */
void MainWindow::onConnectButtonClicked()
{
    QString server = ui->serverEdit->text();
    int port = ui->portSpinBox->value();
    QString username = ui->usernameEdit->text();
    QString password = ui->passwordEdit->text();
    
    if (server.isEmpty()) {
        appendLog("请输入FTP服务器地址");
        return;
    }
    
    // 调用FtpClient连接FTP服务器
    if (ftpClient->connect(server, port, username, password)) {
        isConnected = true;                  // 设置连接标志为true
        updateButtonStates(true);            // 更新按钮状态为已连接
        appendLog("连接成功！");              // 添加成功日志
        currentPath = "/";                   // 设置当前路径为根目录
        directoryHistory.clear();            // 清空目录历史记录
        listDirectory(currentPath);          // 列出根目录内容
    } else {
        // 连接失败，显示错误信息
        appendLog(QString("连接失败: %1").arg(ftpClient->lastError()));
    }
}

/**
 * @brief 断开连接按钮点击事件处理函数
 * 
 * 断开与FTP服务器的连接，更新UI状态
 */
void MainWindow::onDisconnectButtonClicked()
{
    ftpClient->disconnect();                 // 调用断开连接函数
    isConnected = false;                     // 设置连接标志为false
    updateButtonStates(false);               // 更新按钮状态为未连接
    appendLog("已断开连接");                  // 添加断开连接日志
    currentPath = "/";                       // 重置当前路径
    directoryHistory.clear();                // 清空目录历史记录
    fileModel->removeRows(0, fileModel->rowCount()); // 清空文件列表
    updatePathDisplay();                     // 更新路径显示
}

void MainWindow::onBackButtonClicked()
{
    if (!isConnected) return;
    
    if (!directoryHistory.isEmpty()) {
        // 从历史记录中获取上一级目录
        QString prevPath = directoryHistory.pop();
        listDirectory(prevPath);
    } else {
        // 如果历史记录为空，尝试计算上级目录
        QString parentDir = ftpClient->getParentDirectory(currentPath);
        if (parentDir != currentPath) {
            // 如果不是根目录，则导航到上级目录
            listDirectory(parentDir);
        }
    }
}

void MainWindow::onRefreshButtonClicked()
{
    if (isConnected) {
        listDirectory(currentPath);
    }
}

/**
 * @brief 更新路径显示
 * 
 * 在路径编辑框中显示当前FTP目录路径
 */
void MainWindow::updatePathDisplay()
{
    QLineEdit* pathEdit = this->findChild<QLineEdit*>("pathEdit");
    if (pathEdit) {
        pathEdit->setText(currentPath);
    }
}

/**
 * @brief 添加日志信息
 * @param message 日志消息
 * 
 * 在日志文本框中添加一条带时间戳的日志信息
 */
void MainWindow::appendLog(const QString &message)
{
    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
    ui->logTextEdit->appendPlainText(QString("[%1] %2").arg(timeStr).arg(message));
}

/**
 * @brief 更新按钮状态
 * @param connected 是否已连接
 * 
 * 根据当前连接状态更新UI上各个按钮的启用/禁用状态
 */
void MainWindow::updateButtonStates(bool connected)
{
    // 连接按钮和断开连接按钮的状态相反
    ui->connectButton->setEnabled(!connected);
    ui->disconnectButton->setEnabled(connected);
    
    // 连接时启用服务器设置控件，断开时禁用
    ui->serverEdit->setEnabled(!connected);
    ui->portSpinBox->setEnabled(!connected);
    ui->usernameEdit->setEnabled(!connected);
    ui->passwordEdit->setEnabled(!connected);
    
    // 浏览和下载相关按钮仅在连接后启用
    ui->downloadButton->setEnabled(connected);
    
    // 查找返回上级和刷新按钮并设置状态
    QPushButton* backButton = this->findChild<QPushButton*>("backButton");
    QPushButton* refreshButton = this->findChild<QPushButton*>("refreshButton");
    
    if (backButton) {
        backButton->setEnabled(connected);
    }
    
    if (refreshButton) {
        refreshButton->setEnabled(connected);
    }
}

/**
 * @brief 文件树视图双击事件处理
 * @param index 被点击的项目索引
 */
void MainWindow::onFileTreeViewDoubleClicked(const QModelIndex &index)
{
    if (!isConnected) return;
    
    // 获取点击的项目
    QStandardItem* item = fileModel->itemFromIndex(index);
    if (!item) return;
    
    // 获取名称列的项目（第一列）
    QStandardItem* nameItem = fileModel->item(item->row(), 0);
    if (!nameItem) return;
    
    QString name = nameItem->text();
    
    // 获取类型列的项目（第三列）
    QStandardItem* typeItem = fileModel->item(item->row(), 2);
    if (!typeItem) return;
    
    QString type = typeItem->text();
    
    // 如果是目录，则进入该目录
    if (type == "Directory" || name == "..") {
        QString newPath;
        
        if (name == "..") {
            // 返回上级目录
            if (!directoryHistory.isEmpty()) {
                newPath = directoryHistory.pop();
            } else {
                newPath = ftpClient->getParentDirectory(currentPath);
            }
        } else {
            // 进入子目录
            if (currentPath.endsWith("/")) {
                newPath = currentPath + name;
            } else {
                newPath = currentPath + "/" + name;
            }
        }
        
        // 列出新目录的内容
        listDirectory(newPath);
    } else {
        // 如果是文件，显示文件信息
        QStandardItem* sizeItem = fileModel->item(item->row(), 1);
        QString size = sizeItem ? sizeItem->text() : "未知大小";
        
        QStandardItem* dateItem = fileModel->item(item->row(), 3);
        QString date = dateItem ? dateItem->text() : "未知日期";
        
        QString info = QString("文件: %1\n大小: %2\n日期: %3").arg(name).arg(size).arg(date);
        appendLog(info);
    }
}

/**
 * @brief 列出目录内容
 * @param path 要列出的目录路径
 * @return 操作是否成功
 */
bool MainWindow::listDirectory(const QString &path)
{
    if (!isConnected) return false;

    // 保存当前路径到历史记录，用于实现返回功能
    if (path != currentPath) {
        directoryHistory.push(currentPath);
    }

    // 清空当前列表，准备加载新目录内容
    fileModel->removeRows(0, fileModel->rowCount());
    
    // 更新当前路径
    currentPath = path;
    updatePathDisplay();
    appendLog(QString("浏览目录: %1").arg(path));

    // 使用FtpClient列出目录内容
    QStringList listData = ftpClient->listDirectory(path);
    if (listData.isEmpty() && !ftpClient->lastError().isEmpty()) {
        appendLog(QString("获取目录列表失败: %1").arg(ftpClient->lastError()));
        return false;
    }

    // 解析目录列表，转换为文件模型数据
    parseFtpList(listData);
    
    // 添加特殊目录项，便于目录导航
    if (path != "/") {
        // 如果不是根目录，添加返回上级目录的条目".."
        QList<QStandardItem*> parentItems;
        parentItems << new QStandardItem("..")
                    << new QStandardItem("")
                    << new QStandardItem("Directory")
                    << new QStandardItem("");
        fileModel->insertRow(0, parentItems);
    }
    
    return true;
}

/**
 * @brief 解析FTP目录列表
 * @param listData 目录列表数据
 */
void MainWindow::parseFtpList(const QStringList &listData)
{
    // 创建用于匹配不同格式的正则表达式
    // 尝试使用标准的Unix格式解析，如：drwxr-xr-x 2 root root 4096 Jun 12 12:00 dirname
    QRegularExpression unixRe("([d-])([rwx-]{9})\\s+(\\d+)\\s+(\\w+)\\s+(\\w+)\\s+(\\d+)\\s+(\\w+\\s+\\d+\\s+[\\d:]+)\\s+(.+)");
    
    // 尝试匹配Windows FTP服务器格式，如：06-12-23 12:00PM <DIR> dirname
    QRegularExpression windowsRe("(\\d{2}-\\d{2}-\\d{2})\\s+(\\d{2}:\\d{2}[AP]M)\\s+(<DIR>|\\d+)\\s+(.+)");
    
    // 尝试匹配简单的Unix格式，只有权限和文件名
    QRegularExpression simpleRe("([d-])[^\\s]+\\s+.*\\s+(.+)$");
    
    for (const QString &line : listData) {
        // 每行数据尝试使用不同的正则表达式解析
        bool isDir = false;  // 是否为目录
        QString name;        // 名称
        QString size = "";   // 大小
        QString date = "";   // 日期
        
        // 尝试Unix格式匹配
        QRegularExpressionMatch unixMatch = unixRe.match(line);
        if (unixMatch.hasMatch()) {
            QString type = unixMatch.captured(1);  // d表示目录，-表示文件
            isDir = (type == "d");  // 判断是否为目录
            
            size = unixMatch.captured(6);  // 文件大小
            date = unixMatch.captured(7);  // 日期时间
            name = unixMatch.captured(8);  // 文件名
            name.remove('\r');  // 移除可能存在的回车符
            
            // 跳过当前目录和上级目录的特殊标记
            if (name == "." || name == "..") continue;
        } else {
        // 尝试Windows格式匹配
        QRegularExpressionMatch windowsMatch = windowsRe.match(line);
        if (windowsMatch.hasMatch()) {
                QString sizeOrDir = windowsMatch.captured(3);  // <DIR>或者文件大小
                isDir = (sizeOrDir == "<DIR>");  // 判断是否为目录
                
                if (!isDir) size = sizeOrDir;  // 如果不是目录，设置文件大小
                date = windowsMatch.captured(1) + " " + windowsMatch.captured(2);  // 组合日期和时间
                name = windowsMatch.captured(4);  // 文件名
                name.remove('\r');  // 移除可能存在的回车符
                
                // 跳过当前目录和上级目录的特殊标记
                if (name == "." || name == "..") continue;
            } else {
        // 尝试简单格式匹配
        QRegularExpressionMatch simpleMatch = simpleRe.match(line);
        if (simpleMatch.hasMatch()) {
            QString type = simpleMatch.captured(1);  // d表示目录，-表示文件
                    isDir = (type == "d");  // 判断是否为目录
                    name = simpleMatch.captured(2);  // 文件名
                    name.remove('\r');  // 移除可能存在的回车符
                    
                    // 跳过当前目录和上级目录的特殊标记
                    if (name == "." || name == "..") continue;
                } else {
                    // 如果以上格式都无法匹配，尝试按空格分割
                    QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    if (!parts.isEmpty()) {
                        // 简单假设最后一部分是文件名
                        name = parts.last();
                        name.remove('\r');  // 移除可能存在的回车符
                        
                        // 检查名称的第一个字符来判断是否为目录
                        // 这是一个简单的启发式方法，可能不适用于所有FTP服务器
                        if (!parts.first().isEmpty() && parts.first()[0] == 'd') {
                            isDir = true;
                        }
                        
                        // 跳过当前目录和上级目录的特殊标记
                        if (name == "." || name == "..") continue;
            } else {
                        // 无法解析的行，跳过
                        continue;
                    }
                }
            }
        }
        
        // 创建文件列表项
            QList<QStandardItem*> items;
        
        // 名称列
        QStandardItem* nameItem = new QStandardItem(name);
        // 根据是否为目录设置不同的图标
        if (isDir) {
            nameItem->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
        } else {
            nameItem->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
        }
        items << nameItem;
        
        // 大小列，目录显示为空
        if (isDir) {
            items << new QStandardItem("");
        } else {
            // 对文件大小进行格式化，使其更易读
            qint64 sizeVal = size.toLongLong();
            QString sizeStr;
            if (sizeVal < 1024) {
                sizeStr = QString("%1 B").arg(sizeVal);
            } else if (sizeVal < 1024 * 1024) {
                sizeStr = QString("%1 KB").arg(sizeVal / 1024.0, 0, 'f', 2);
            } else if (sizeVal < 1024 * 1024 * 1024) {
                sizeStr = QString("%1 MB").arg(sizeVal / (1024.0 * 1024.0), 0, 'f', 2);
            } else {
                sizeStr = QString("%1 GB").arg(sizeVal / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
            }
            items << new QStandardItem(sizeStr);
        }
        
        // 类型列
        items << new QStandardItem(isDir ? "Directory" : "File");
        
        // 日期列
        items << new QStandardItem(date);
        
        // 添加到模型
        fileModel->appendRow(items);
    }
}

/**
 * @brief 添加下载任务
 * @param remotePath 远程文件/目录路径
 * @param localPath 本地保存路径
 * @param isDirectory 是否是目录
 * @param displayName 显示名称
 * @param fileSize 文件大小
 * 
 * 创建下载任务并添加到下载队列中
 */
void MainWindow::addDownloadTask(const QString &remotePath, const QString &localPath, 
                               bool isDirectory, const QString &displayName, qint64 fileSize)
{
    // 创建下载任务
    DownloadTask task;
    task.remotePath = remotePath;
    task.localPath = localPath;
    task.isDirectory = isDirectory;
    task.fileSize = fileSize;
    
    // 设置显示名称，如果没有提供，则使用路径
    if (displayName.isEmpty()) {
        QFileInfo fileInfo(remotePath);
        task.displayName = fileInfo.fileName();
    } else {
        task.displayName = displayName;
    }
    
    // 添加到下载队列
    QMutexLocker locker(&downloadMutex);
    downloadQueue.enqueue(task);
    
    // 记录日志
    appendLog(QString("添加%1任务: %2").arg(isDirectory ? "目录" : "文件").arg(task.displayName));
}

/**
 * @brief 下载按钮点击事件处理函数
 * 
 * 下载当前选中的文件或目录到本地指定位置
 */
void MainWindow::onDownloadButtonClicked()
{
    if (!isConnected) return;
    
    // 获取当前选中的项目
    QModelIndex index = ui->fileTreeView->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "警告", "请先选择要下载的文件或目录");
        return;
    }
    
    // 获取选中项的所在行
    int row = index.row();
    
    // 获取名称和类型
    QString name = fileModel->item(row, 0)->text();
    QString type = fileModel->item(row, 2)->text();
    
    // 如果是特殊目录项，不允许下载
    if (name == ".." || name == ".") {
        QMessageBox::warning(this, "警告", "不能下载特殊目录");
        return;
    }
    
    // 构建远程路径
    QString remotePath;
                if (currentPath.endsWith("/")) {
        remotePath = currentPath + name;
                } else {
        remotePath = currentPath + "/" + name;
    }
    
    // 弹出文件对话框，让用户选择保存位置
    QString saveDir;
    if (type == "Directory") {
        // 如果是目录，让用户选择保存的目录
        saveDir = QFileDialog::getExistingDirectory(this, "选择保存目录", QDir::homePath());
    } else {
        // 如果是文件，让用户选择保存的文件名
        saveDir = QFileDialog::getSaveFileName(this, "保存文件", QDir::homePath() + "/" + name);
    }
    
    // 如果用户取消了选择，直接返回
    if (saveDir.isEmpty()) {
        return;
    }
    
    // 获取文件大小（如果有）
    qint64 fileSize = 0;
    QStandardItem* sizeItem = fileModel->item(row, 1);
    if (sizeItem) {
        QString sizeStr = sizeItem->text();
        // 尝试解析大小字符串，如"1.2 KB"
        QRegularExpression sizeRe("([\\d\\.]+)\\s*([KMGB]+)?");
        QRegularExpressionMatch match = sizeRe.match(sizeStr);
        if (match.hasMatch()) {
            double size = match.captured(1).toDouble();
            QString unit = match.captured(2);
            
            if (unit == "KB") {
                fileSize = static_cast<qint64>(size * 1024);
            } else if (unit == "MB") {
                fileSize = static_cast<qint64>(size * 1024 * 1024);
            } else if (unit == "GB") {
                fileSize = static_cast<qint64>(size * 1024 * 1024 * 1024);
    } else {
                fileSize = static_cast<qint64>(size);
            }
        }
    }
    
    // 添加下载任务
    bool isDir = (type == "Directory");
    
    // 确保目录路径以/结尾
    if (isDir && !remotePath.endsWith("/")) {
        remotePath += "/";
    }
    
    // 当下载目录时，在保存路径后添加目录名，确保创建同名文件夹
    QString localPath = saveDir;
    if (isDir) {
        // 将目标路径设置为: 用户选择的目录 + 远程目录名
        localPath = QDir::cleanPath(saveDir + "/" + name);
        appendLog(QString("准备下载目录: %1 -> %2").arg(remotePath).arg(localPath));
    } else {
        appendLog(QString("准备下载文件: %1 -> %2").arg(remotePath).arg(localPath));
    }
    
    // 添加任务到队列
    addDownloadTask(remotePath, localPath, isDir, name, fileSize);
    
    // 如果当前没有下载任务在进行，启动下载定时器
    if (!isDownloading) {
        isDownloading = true;
        downloadTimer->start(100); // 100毫秒后开始处理队列
    }
}

/**
 * @brief 处理下载队列中的下一个任务
 * 
 * 从下载队列中取出一个任务进行处理
 */
void MainWindow::processNextDownloadTask()
{
    // 检查队列是否为空
    QMutexLocker locker(&downloadMutex);
    if (downloadQueue.isEmpty()) {
        // 队列为空，停止定时器，更新状态
        downloadTimer->stop();
        isDownloading = false;
        
        // 如果有进度对话框，关闭它
        if (progressDialog) {
            progressDialog->close();
            delete progressDialog;
            progressDialog = nullptr;
        }
        
        appendLog("所有下载任务已完成");
        return;
    }
    
    // 取出队列中的第一个任务
    DownloadTask task = downloadQueue.dequeue();
    locker.unlock(); // 解锁互斥锁，允许其他线程访问队列
    
    // 创建或更新进度对话框
    if (!progressDialog) {
        progressDialog = new QProgressDialog("正在下载...", "取消", 0, 100, this);
        progressDialog->setWindowTitle("下载进度");
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(nullptr); // 不允许取消
        progressDialog->setAutoClose(false);
        progressDialog->show();
    }
    
    // 设置进度对话框标签
    progressDialog->setLabelText(QString("正在下载: %1").arg(task.displayName));
    progressDialog->setValue(0);
    
    appendLog(QString("开始下载: %1").arg(task.displayName));
    
    // 如果是目录，使用递归方法下载
    if (task.isDirectory) {
        // 调用FtpClient下载目录
        bool success = ftpClient->downloadDirectory(task.remotePath, task.localPath, 
                                               [this](qint64 bytesReceived, qint64 bytesTotal) {
                                                   this->updateDownloadProgress(bytesReceived, bytesTotal);
                                               });
        
        if (success) {
            appendLog(QString("目录下载完成: %1").arg(task.displayName));
        } else {
            appendLog(QString("目录下载失败: %1，错误: %2").arg(task.displayName).arg(ftpClient->lastError()));
        }
    } else {
        // 下载文件
        bool success = ftpClient->downloadFile(task.remotePath, task.localPath, 
                                          [this](qint64 bytesReceived, qint64 bytesTotal) {
                                              this->updateDownloadProgress(bytesReceived, bytesTotal);
                                          });
        
        if (success) {
            appendLog(QString("文件下载完成: %1").arg(task.displayName));
        } else {
            appendLog(QString("文件下载失败: %1，错误: %2").arg(task.displayName).arg(ftpClient->lastError()));
        }
    }
    
    // 处理下一个任务
    QTimer::singleShot(100, this, &MainWindow::processNextDownloadTask);
}

/**
 * @brief 更新下载进度
 * @param bytesReceived 已接收字节数
 * @param bytesTotal 总字节数
 */
void MainWindow::updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (!progressDialog) return;
    
    // 计算百分比
    int percent = 0;
    if (bytesTotal > 0) {
        percent = static_cast<int>((bytesReceived * 100) / bytesTotal);
        } else {
        // 如果不知道总大小，根据已接收量显示进度
        percent = qMin(99, static_cast<int>(bytesReceived / 1024)); // 限制最大为99%
    }
    
    // 更新进度对话框
    progressDialog->setValue(percent);
    
    // 更新大小标签
    QString sizeText;
    if (bytesReceived < 1024) {
        sizeText = QString("%1 / %2 B").arg(bytesReceived).arg(bytesTotal > 0 ? QString::number(bytesTotal) : "?");
    } else if (bytesReceived < 1024 * 1024) {
        sizeText = QString("%1 / %2 KB").arg(bytesReceived / 1024.0, 0, 'f', 2)
                        .arg(bytesTotal > 0 ? QString::number(bytesTotal / 1024.0, 'f', 2) : "?");
    } else if (bytesReceived < 1024 * 1024 * 1024) {
        sizeText = QString("%1 / %2 MB").arg(bytesReceived / (1024.0 * 1024.0), 0, 'f', 2)
                        .arg(bytesTotal > 0 ? QString::number(bytesTotal / (1024.0 * 1024.0), 'f', 2) : "?");
                } else {
        sizeText = QString("%1 / %2 GB").arg(bytesReceived / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2)
                        .arg(bytesTotal > 0 ? QString::number(bytesTotal / (1024.0 * 1024.0 * 1024.0), 'f', 2) : "?");
    }
    
    progressDialog->setLabelText(QString("正在下载: %1\n%2").arg(
        downloadQueue.isEmpty() ? "" : downloadQueue.head().displayName).arg(sizeText));
}
