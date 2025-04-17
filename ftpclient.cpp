/**
 * @file ftpclient.cpp
 * @brief FTP客户端封装类实现文件
 * 
 * 本文件实现了FTP客户端的核心功能，包括FTP连接、
 * 目录浏览、文件下载等。使用libcurl库作为底层FTP协议实现。
 */

#include "ftpclient.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

/**
 * @brief 构造函数，初始化资源
 */
FtpClient::FtpClient()
    : m_curl(nullptr)
    , m_headers(nullptr)
    , m_isConnected(false)
    , m_port(21)
    , m_currentDownloadFile(nullptr)
    , m_totalBytesReceived(0)
{
    // 初始化 libcurl 全局环境
    curl_global_init(CURL_GLOBAL_ALL);
    m_curl = curl_easy_init();
}

/**
 * @brief 析构函数，释放资源
 */
FtpClient::~FtpClient()
{
    // 清理下载相关资源
    if (m_currentDownloadFile) {
        m_currentDownloadFile->close();
        delete m_currentDownloadFile;
        m_currentDownloadFile = nullptr;
    }
    
    if (m_headers) {
        curl_slist_free_all(m_headers);
    }
    
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }
    
    curl_global_cleanup();
}

/**
 * @brief 连接FTP服务器
 * @param server 服务器地址
 * @param port 端口号
 * @param username 用户名
 * @param password 密码
 * @return 连接是否成功
 */
bool FtpClient::connect(const QString &server, int port, const QString &username, const QString &password)
{
    if (!m_curl) {
        m_lastError = "CURL未初始化";
        return false;
    }
    
    // 如果已经连接，先断开
    if (m_isConnected) {
        disconnect();
    }
    
    // 保存连接信息
    m_server = server;
    m_port = port;
    m_username = username;
    m_password = password;
    
    // 构建FTP URL
    QString ftpUrl = m_server;
    if (!ftpUrl.startsWith("ftp://")) {
        ftpUrl = "ftp://" + ftpUrl;
    }

    // 设置CURL选项
    curl_easy_setopt(m_curl, CURLOPT_URL, ftpUrl.toUtf8().constData());
    curl_easy_setopt(m_curl, CURLOPT_PORT, m_port);
    curl_easy_setopt(m_curl, CURLOPT_USERNAME, m_username.toUtf8().constData());
    curl_easy_setopt(m_curl, CURLOPT_PASSWORD, m_password.toUtf8().constData());
    curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);

    // 执行连接测试
    m_listBuffer.clear();
    CURLcode res = curl_easy_perform(m_curl);
    if (res != CURLE_OK) {
        m_lastError = QString("连接失败: %1").arg(curl_easy_strerror(res));
        return false;
    }

    m_isConnected = true;
    return true;
}

/**
 * @brief 断开FTP连接
 */
void FtpClient::disconnect()
{
    if (m_curl) {
        curl_easy_cleanup(m_curl);
        m_curl = curl_easy_init();
    }
    
    m_isConnected = false;
}

/**
 * @brief 列出目录内容
 * @param path 要列出的目录路径
 * @return 目录内容列表
 */
QStringList FtpClient::listDirectory(const QString &path)
{
    if (!m_curl || !m_isConnected) {
        m_lastError = "未连接到FTP服务器";
        return QStringList();
    }

    // 清空接收缓冲区
    m_listBuffer.clear();
    
    // 移除可能存在的\r字符，确保路径格式正确
    QString cleanPath = path;
    cleanPath.remove('\r');
    
    // 构建完整的URL，用于FTP请求
    QString server = m_server;
    if (!server.startsWith("ftp://")) {
        server = "ftp://" + server;
    }
    
    // 确保server不以/结尾，而path以/开头
    if (server.endsWith("/")) {
        server.chop(1);
    }
    
    // 规范化路径格式
    QString normalizedPath = cleanPath;
    if (!normalizedPath.startsWith("/")) {
        normalizedPath = "/" + normalizedPath;
    }
    // 确保路径以/结尾，这对于FTP目录浏览很重要
    if (!normalizedPath.endsWith("/")) {
        normalizedPath += "/";
    }
    
    // 对URL进行编码处理
    QByteArray pathUtf8 = normalizedPath.toUtf8();
    char *escapedPath = curl_easy_escape(m_curl, pathUtf8.constData(), pathUtf8.length());
    
    // 替换掉编码后的斜杠，因为我们需要保留路径结构
    QString encodedPath = QString(escapedPath);
    encodedPath.replace("%2F", "/");
    
    QString fullUrl = server + encodedPath;
    
    // 释放CURL分配的内存
    curl_free(escapedPath);
    
    // 设置CURL选项
    curl_easy_setopt(m_curl, CURLOPT_URL, fullUrl.toUtf8().constData());
    curl_easy_setopt(m_curl, CURLOPT_USERNAME, m_username.toUtf8().constData());
    curl_easy_setopt(m_curl, CURLOPT_PASSWORD, m_password.toUtf8().constData());
    curl_easy_setopt(m_curl, CURLOPT_PORT, m_port);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_DIRLISTONLY, 0L);

    // 执行列表命令
    CURLcode res = curl_easy_perform(m_curl);
    if (res != CURLE_OK) {
        m_lastError = QString("获取目录列表失败: %1").arg(curl_easy_strerror(res));
        return QStringList();
    }

    // 返回目录列表数据
    return m_listBuffer;
}

/**
 * @brief 下载文件
 * @param remotePath 远程文件路径
 * @param localPath 本地保存路径
 * @param progressCallback 进度回调函数
 * @return 下载是否成功
 */
bool FtpClient::downloadFile(const QString &remotePath, const QString &localPath, 
                            std::function<void(qint64, qint64)> progressCallback)
{
    if (!m_curl || !m_isConnected) {
        m_lastError = "未连接到FTP服务器";
        return false;
    }
    
    // 保存回调函数
    m_progressCallback = progressCallback;
    
    // 创建本地文件
    m_currentDownloadFile = new QFile(localPath);
    if (!m_currentDownloadFile->open(QIODevice::WriteOnly)) {
        m_lastError = QString("无法创建本地文件: %1").arg(localPath);
        delete m_currentDownloadFile;
        m_currentDownloadFile = nullptr;
        return false;
    }
    
    // 重置已接收字节数
    m_totalBytesReceived = 0;
    
    // 构建完整的URL
    QString server = m_server;
    if (!server.startsWith("ftp://")) {
        server = "ftp://" + server;
    }
    
    // 确保server不以/结尾
    if (server.endsWith("/")) {
        server.chop(1);
    }
    
    // 确保remotePath以/开头
    QString normalizedPath = remotePath;
    if (!normalizedPath.startsWith("/")) {
        normalizedPath = "/" + normalizedPath;
    }
    
    // 对URL进行编码处理
    QByteArray pathUtf8 = normalizedPath.toUtf8();
    char *escapedPath = curl_easy_escape(m_curl, pathUtf8.constData(), pathUtf8.length());
    
    // 替换掉编码后的斜杠，因为我们需要保留路径结构
    QString encodedPath = QString(escapedPath);
    encodedPath.replace("%2F", "/");
    
    QString fullUrl = server + encodedPath;
    
    // 释放CURL分配的内存
    curl_free(escapedPath);
    
    // 设置CURL选项
    curl_easy_setopt(m_curl, CURLOPT_URL, fullUrl.toUtf8().constData());
    curl_easy_setopt(m_curl, CURLOPT_USERNAME, m_username.toUtf8().constData());
    curl_easy_setopt(m_curl, CURLOPT_PASSWORD, m_password.toUtf8().constData());
    curl_easy_setopt(m_curl, CURLOPT_PORT, m_port);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, DownloadCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
    
    // 执行下载
    CURLcode res = curl_easy_perform(m_curl);
    
    // 关闭文件
    m_currentDownloadFile->close();
    delete m_currentDownloadFile;
    m_currentDownloadFile = nullptr;
    
    if (res != CURLE_OK) {
        m_lastError = QString("下载文件失败: %1").arg(curl_easy_strerror(res));
        return false;
    }
    
    return true;
}

/**
 * @brief 下载目录
 * @param remotePath 远程目录路径
 * @param localPath 本地保存路径
 * @param progressCallback 进度回调函数
 * @param taskQueue 下载任务队列，用于存储待下载的文件任务
 * @return 下载是否成功
 */
bool FtpClient::downloadDirectory(const QString &remotePath, const QString &localPath,
                                 std::function<void(qint64, qint64)> progressCallback,
                                 QQueue<DownloadTask> *taskQueue)
{
    // 创建本地目录
    if (!createLocalDirectory(localPath)) {
        m_lastError = QString("无法创建本地目录: %1").arg(localPath);
        return false;
    }
    
    // 列出目录内容并创建目录结构、添加文件到下载队列
    return listDirectoryForDownload(remotePath, localPath, progressCallback, taskQueue);
}

/**
 * @brief 列出目录内容用于下载
 * @param path 要列出的目录路径
 * @param targetDir 本地目标目录
 * @param progressCallback 进度回调函数
 * @param taskQueue 下载任务队列，用于存储待下载的文件任务
 * @return 操作是否成功
 */
bool FtpClient::listDirectoryForDownload(const QString &path, const QString &targetDir,
                                        std::function<void(qint64, qint64)> progressCallback,
                                        QQueue<DownloadTask> *taskQueue)
{
    if (!m_curl || !m_isConnected) {
        m_lastError = "未连接到FTP服务器";
        return false;
    }
    
    // 清空列表缓冲区
    m_listBuffer.clear();
    
    // 构建完整的URL
    QString server = m_server;
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
    
    // 确保路径以/结尾
    if (!normalizedPath.endsWith("/")) {
        normalizedPath += "/";
    }
    
    // 对URL进行编码处理
    QByteArray pathUtf8 = normalizedPath.toUtf8();
    char *escapedPath = curl_easy_escape(m_curl, pathUtf8.constData(), pathUtf8.length());
    
    // 替换编码后的斜杠
    QString encodedPath = QString(escapedPath);
    encodedPath.replace("%2F", "/");
    
    QString fullUrl = server + encodedPath;
    
    // 释放CURL分配的内存
    curl_free(escapedPath);
    
    // 设置CURL选项
    CURL *listHandle = curl_easy_init();
    if (!listHandle) {
        m_lastError = "无法初始化CURL列表句柄";
        return false;
    }
    
    curl_easy_setopt(listHandle, CURLOPT_URL, fullUrl.toUtf8().constData());
    curl_easy_setopt(listHandle, CURLOPT_USERNAME, m_username.toUtf8().constData());
    curl_easy_setopt(listHandle, CURLOPT_PASSWORD, m_password.toUtf8().constData());
    curl_easy_setopt(listHandle, CURLOPT_PORT, m_port);
    curl_easy_setopt(listHandle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(listHandle, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(listHandle, CURLOPT_DIRLISTONLY, 0L);
    
    // 执行列表命令
    CURLcode res = curl_easy_perform(listHandle);
    
    // 清理CURL句柄
    curl_easy_cleanup(listHandle);
    
    if (res != CURLE_OK) {
        m_lastError = QString("获取目录列表失败: %1").arg(curl_easy_strerror(res));
        return false;
    }
    
    // 解析目录列表
    QString listData = m_listBuffer.join("\n");
    QStringList lines = listData.split("\n", Qt::SkipEmptyParts);
    
    // 使用正则表达式解析文件列表
    QRegularExpression unixRe("([d-])([rwx-]{9})\\s+(\\d+)\\s+(\\w+)\\s+(\\w+)\\s+(\\d+)\\s+(\\w+\\s+\\d+\\s+[\\d:]+)\\s+(.+)");
    QRegularExpression windowsRe("(\\d{2}-\\d{2}-\\d{2})\\s+(\\d{2}:\\d{2}[AP]M)\\s+(<DIR>|\\d+)\\s+(.+)");
    QRegularExpression simpleRe("([d-])[^\\s]+\\s+.*\\s+(.+)$");
    
    bool success = true;
    
    for (const QString &line : lines) {
        bool isDir = false;
        QString name;
        qint64 fileSize = 0;
        
        // 尝试Unix格式匹配
        QRegularExpressionMatch unixMatch = unixRe.match(line);
        if (unixMatch.hasMatch()) {
            QString type = unixMatch.captured(1);
            name = unixMatch.captured(8).trimmed();
            name.remove('\r');
            isDir = (type == "d");
            
            // 尝试获取文件大小
            if (!isDir) {
                fileSize = unixMatch.captured(6).toLongLong();
            }
            
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
                
                // 尝试获取文件大小
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
                    // 简单分割
                    QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    if (parts.size() >= 1) {
                        name = parts.last().trimmed();
                        name.remove('\r');
                        
                        // 跳过特殊目录
                        if (name == "." || name == "..") {
                            continue;
                        }
                        
                        // 通过其他方法判断是否为目录
                        QString testPath = normalizedPath + name;
                        QStringList testResult = listDirectory(testPath);
                        isDir = !testResult.isEmpty();
                    } else {
                        continue;
                    }
                }
            }
        }
        
        // 处理找到的项目
        if (isDir) {
            // 如果是目录，先创建本地目录，然后递归处理
            QString newRemotePath = normalizedPath + name;
            QString newLocalPath = targetDir + "/" + name;
            
            if (!createLocalDirectory(newLocalPath)) {
                m_lastError = QString("无法创建本地目录: %1").arg(newLocalPath);
                success = false;
                continue;
            }
            
            // 递归处理子目录，继续使用相同的任务队列
            success = listDirectoryForDownload(newRemotePath, newLocalPath, progressCallback, taskQueue) && success;
        } else {
            // 如果是文件，添加到任务队列而不是直接下载
            if (taskQueue) {
                // 创建下载任务并添加到队列
                DownloadTask task;
                task.remotePath = normalizedPath + name;
                task.localPath = targetDir + "/" + name;
                task.isDirectory = false;
                task.fileSize = fileSize;
                task.displayName = name;
                
                // 将任务添加到队列
                taskQueue->enqueue(task);
            } else {
                // 如果没有提供队列，则使用传统方式直接下载
                QString remoteFilePath = normalizedPath + name;
                QString localFilePath = targetDir + "/" + name;
                
                if (!downloadFile(remoteFilePath, localFilePath, progressCallback)) {
                    success = false;
                }
            }
        }
    }
    
    return success;
}

/**
 * @brief 创建本地目录
 * @param localPath 本地目录路径
 * @return 操作是否成功
 */
bool FtpClient::createLocalDirectory(const QString &localPath)
{
    QDir dir;
    return dir.mkpath(localPath);
}

/**
 * @brief 获取上级目录路径
 * @param path 当前路径
 * @return 上级目录路径
 */
QString FtpClient::getParentDirectory(const QString &path)
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

/**
 * @brief CURL写入回调函数
 * @param contents 接收到的数据
 * @param size 数据块大小
 * @param nmemb 数据块数量
 * @param userp 用户数据指针
 * @return 实际写入的数据大小
 */
size_t FtpClient::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    FtpClient *client = static_cast<FtpClient*>(userp);
    
    if (client) {
        // 将接收到的数据转换为字符串并添加到buffer
        QByteArray data(static_cast<const char*>(contents), realsize);
        QString line = QString::fromUtf8(data);
        client->m_listBuffer.append(line.split("\n", Qt::SkipEmptyParts));
    }
    
    return realsize;
}

/**
 * @brief 下载文件回调函数
 * @param contents 接收到的数据
 * @param size 数据块大小
 * @param nmemb 数据块数量
 * @param userp 用户数据指针
 * @return 实际写入的数据大小
 */
size_t FtpClient::DownloadCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    FtpClient *client = static_cast<FtpClient*>(userp);
    size_t realsize = size * nmemb;
    
    if (client && client->m_currentDownloadFile) {
        // 将数据写入文件
        qint64 written = client->m_currentDownloadFile->write(static_cast<char*>(contents), realsize);
        
        // 更新下载进度
        client->m_totalBytesReceived += written;
        
        // 如果有回调函数，调用它更新进度
        if (client->m_progressCallback) {
            client->m_progressCallback(client->m_totalBytesReceived, client->m_totalBytesReceived);
        }
        
        return written;
    }
    
    return 0;
} 