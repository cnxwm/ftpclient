/**
 * @file ftpclient.h
 * @brief FTP客户端封装类
 * @details 使用libcurl实现FTP客户端功能的封装
 * 
 * 本文件封装了FTP客户端的基本功能：
 * 1. 连接/断开FTP服务器
 * 2. 浏览FTP服务器目录结构
 * 3. 下载文件和目录
 * 使用libcurl库处理FTP协议通信
 */

#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <QString>
#include <QStringList>
#include <QQueue>
#include <QMutex>
#include <QFile>
#include <curl/curl.h> // libcurl头文件，用于FTP协议处理

/**
 * @struct DownloadTask
 * @brief 下载任务结构体
 * 
 * 用于存储单个下载任务的信息，包括本地路径、远程路径、文件大小等
 * 这些任务会进入下载队列，由程序依次处理
 */
struct DownloadTask {
    QString remotePath;      ///< 远程文件路径（FTP服务器上的路径）
    QString localPath;       ///< 本地文件保存路径（用户电脑上的路径）
    bool isDirectory;        ///< 是否是目录（true表示目录，false表示文件）
    qint64 fileSize;         ///< 文件大小（字节数）
    QString displayName;     ///< 显示名称（用于进度对话框显示）
};

/**
 * @class FtpClient
 * @brief FTP客户端封装类
 * 
 * 封装了FTP客户端的基本功能，包括连接服务器、浏览目录、下载文件等
 */
class FtpClient
{
public:
    /**
     * @brief 构造函数
     * 
     * 初始化CURL和相关资源
     */
    FtpClient();
    
    /**
     * @brief 析构函数
     * 
     * 释放资源，关闭CURL连接
     */
    ~FtpClient();

    /**
     * @brief 连接FTP服务器
     * @param server 服务器地址
     * @param port 端口号
     * @param username 用户名
     * @param password 密码
     * @return 连接是否成功
     */
    bool connect(const QString &server, int port, const QString &username, const QString &password);
    
    /**
     * @brief 断开FTP连接
     */
    void disconnect();
    
    /**
     * @brief 列出目录内容
     * @param path 要列出的目录路径
     * @return 目录内容列表(每行一条目录项信息)
     */
    QStringList listDirectory(const QString &path = "/");
    
    /**
     * @brief 下载文件
     * @param remotePath 远程文件路径
     * @param localPath 本地保存路径
     * @param progressCallback 进度回调函数(可选)
     * @return 下载是否成功
     */
    bool downloadFile(const QString &remotePath, const QString &localPath, 
                      std::function<void(qint64, qint64)> progressCallback = nullptr);
    
    /**
     * @brief 下载目录
     * @param remotePath 远程目录路径
     * @param localPath 本地保存路径
     * @param progressCallback 进度回调函数
     * @param taskQueue 下载任务队列，用于存储待下载的文件任务
     * @return 下载是否成功
     */
    bool downloadDirectory(const QString &remotePath, const QString &localPath,
                          std::function<void(qint64, qint64)> progressCallback = nullptr,
                          QQueue<DownloadTask> *taskQueue = nullptr);
    
    /**
     * @brief 获取当前连接状态
     * @return 是否已连接
     */
    bool isConnected() const { return m_isConnected; }
    
    /**
     * @brief 获取上一个错误消息
     * @return 错误信息
     */
    QString lastError() const { return m_lastError; }
    
    /**
     * @brief 获取上级目录路径
     * @param path 当前路径
     * @return 上级目录路径
     */
    QString getParentDirectory(const QString &path);
    
private:
    /**
     * @brief 列出目录内容用于下载
     * @param path 要列出的目录路径
     * @param targetDir 本地目标目录
     * @param progressCallback 进度回调函数
     * @param taskQueue 下载任务队列，用于存储待下载的文件任务
     * @return 操作是否成功
     */
    bool listDirectoryForDownload(const QString &path, const QString &targetDir,
                                 std::function<void(qint64, qint64)> progressCallback = nullptr,
                                 QQueue<DownloadTask> *taskQueue = nullptr);
    
    /**
     * @brief 创建本地目录
     * @param localPath 本地目录路径
     * @return 操作是否成功
     */
    bool createLocalDirectory(const QString &localPath);
    
    /**
     * @brief CURL写入回调函数
     * @param contents 接收到的数据
     * @param size 数据块大小
     * @param nmemb 数据块数量
     * @param userp 用户数据指针
     * @return 实际写入的数据大小
     */
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

    /**
     * @brief 下载文件回调函数
     * @param contents 接收到的数据
     * @param size 数据块大小
     * @param nmemb 数据块数量
     * @param userp 用户数据指针
     * @return 实际写入的数据大小
     */
    static size_t DownloadCallback(void *contents, size_t size, size_t nmemb, void *userp);

private:
    CURL* m_curl;                           ///< CURL句柄
    struct curl_slist *m_headers;           ///< CURL头部列表
    bool m_isConnected;                     ///< 连接状态
    QString m_server;                       ///< 服务器地址
    int m_port;                             ///< 端口号
    QString m_username;                     ///< 用户名
    QString m_password;                     ///< 密码
    QString m_lastError;                    ///< 最后一个错误消息
    QStringList m_listBuffer;               ///< 列表数据缓冲区
    
    // 下载相关变量
    QFile* m_currentDownloadFile;           ///< 当前下载文件
    qint64 m_totalBytesReceived;            ///< 已接收字节总数
    std::function<void(qint64, qint64)> m_progressCallback; ///< 进度回调函数
};

#endif // FTPCLIENT_H 