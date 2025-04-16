/**
 * @file mainwindow.h
 * @brief FTP客户端主窗口头文件
 * @details 使用libcurl实现FTP客户端功能
 * 
 * 本文件定义了FTP客户端的主窗口类及相关数据结构，实现了基本的FTP功能：
 * 1. 连接/断开FTP服务器
 * 2. 浏览FTP服务器目录结构
 * 3. 下载文件和目录
 * 使用libcurl库处理FTP协议通信
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUrl>
#include <QFile>
#include <curl/curl.h> // libcurl头文件，用于FTP协议处理
#include <QStandardItemModel>
#include <QStack>
#include <QQueue>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QTimer>
#include <QProgressBar>
#include <QProgressDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

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
 * @class MainWindow
 * @brief FTP客户端主窗口类
 * 
 * 实现了FTP客户端的主要功能，包括连接服务器、浏览目录、下载文件等
 * 界面通过Qt Designer设计，具体实现在mainwindow.ui文件中
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     * 
     * 初始化UI、设置文件模型、连接信号槽、初始化libcurl等
     */
    MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     * 
     * 释放资源，包括libcurl资源、文件句柄、进度对话框等
     */
    ~MainWindow();

private slots:
    /**
     * @brief 连接按钮点击事件处理
     * 
     * 当用户点击"连接"按钮时，尝试连接到指定的FTP服务器
     * 连接成功后会更新UI状态，并自动列出根目录内容
     */
    void onConnectButtonClicked();
    
    /**
     * @brief 断开连接按钮点击事件处理
     * 
     * 当用户点击"断开连接"按钮时，断开与FTP服务器的连接
     * 断开后会更新UI状态，清空文件列表
     */
    void onDisconnectButtonClicked();
    
    /**
     * @brief 文件树视图双击事件处理
     * @param index 被点击的项目索引
     * 
     * 双击目录时：进入该目录并显示其内容
     * 双击文件时：显示文件信息（大小、日期）
     */
    void onFileTreeViewDoubleClicked(const QModelIndex &index);
    
    /**
     * @brief 返回上级目录按钮点击事件处理
     * 
     * 返回到当前目录的上一级目录，并显示上级目录的内容
     */
    void onBackButtonClicked();
    
    /**
     * @brief 刷新目录按钮点击事件处理
     * 
     * 重新获取当前目录的内容，用于刷新显示
     */
    void onRefreshButtonClicked();

    /**
     * @brief 下载按钮点击事件处理
     * 
     * 下载当前选中的文件或目录到本地指定位置
     * 会弹出文件对话框让用户选择保存位置
     */
    void onDownloadButtonClicked();
    
    /**
     * @brief 处理下载队列中的下一个任务
     * 
     * 从下载队列中取出一个任务进行处理
     * 如果队列为空，停止下载流程
     */
    void processNextDownloadTask();
    
    /**
     * @brief 处理下载进度更新
     * @param bytesReceived 已接收字节数
     * @param bytesTotal 总字节数
     * 
     * 更新进度对话框，显示当前下载进度
     */
    void updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    /**
     * @brief 连接FTP服务器
     * @return 连接是否成功
     * 
     * 使用libcurl建立到FTP服务器的连接
     * 从UI获取服务器地址、端口、用户名和密码信息
     */
    bool connectToFtp();
    
    /**
     * @brief 断开FTP连接
     * 
     * 清理libcurl资源，断开与服务器的连接
     */
    void disconnectFromFtp();
    
    /**
     * @brief 列出目录内容
     * @param path 要列出的目录路径
     * @return 操作是否成功
     * 
     * 获取指定FTP目录的内容，并在文件树视图中显示
     * 处理不同FTP服务器返回的不同格式的列表数据
     */
    bool listDirectory(const QString &path = "/");
    
    /**
     * @brief 解析FTP目录列表
     * @param listData 目录列表数据
     * 
     * 解析FTP服务器返回的目录列表数据，支持多种格式：
     * 1. Unix格式（如：drwxr-xr-x 2 root root 4096 Jun 12 12:00 dirname）
     * 2. Windows格式（如：06-12-23 12:00PM <DIR> dirname）
     * 3. 简单格式（只包含文件名）
     */
    void parseFtpList(const QString &listData);
    
    /**
     * @brief 更新按钮状态
     * @param connected 是否已连接
     * 
     * 根据当前连接状态更新UI上各个按钮的启用/禁用状态
     */
    void updateButtonStates(bool connected);
    
    /**
     * @brief 添加日志信息
     * @param message 日志消息
     * 
     * 在日志文本框中添加一条带时间戳的日志信息
     */
    void appendLog(const QString &message);
    
    /**
     * @brief 更新当前路径显示
     * 
     * 在路径编辑框中显示当前FTP目录路径
     */
    void updatePathDisplay();
    
    /**
     * @brief 获取上级目录路径
     * @param path 当前路径
     * @return 上级目录路径
     * 
     * 根据当前路径计算其上级目录的路径
     * 例如：/home/user/ 的上级目录是 /home/
     */
    QString getParentDirectory(const QString &path);
    
    /**
     * @brief CURL写入回调函数
     * @param contents 接收到的数据
     * @param size 数据块大小
     * @param nmemb 数据块数量
     * @param userp 用户数据指针
     * @return 实际写入的数据大小
     * 
     * libcurl的回调函数，用于接收目录列表数据
     * 此函数将接收到的数据追加到listBuffer中
     */
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

    /**
     * @brief 下载文件回调函数
     * @param contents 接收到的数据
     * @param size 数据块大小
     * @param nmemb 数据块数量
     * @param userp 用户数据指针
     * @return 实际写入的数据大小
     * 
     * libcurl的回调函数，用于接收文件下载数据
     * 此函数将接收到的数据写入本地文件，并更新下载进度
     */
    static size_t DownloadCallback(void *contents, size_t size, size_t nmemb, void *userp);
    
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
    void addDownloadTask(const QString &remotePath, const QString &localPath, 
                         bool isDirectory, const QString &displayName = "", qint64 fileSize = 0);
    
    /**
     * @brief 下载文件
     * @param remotePath 远程文件路径
     * @param localPath 本地保存路径
     * @return 下载是否成功
     * 
     * 使用libcurl下载单个FTP文件
     * 支持中文文件名和特殊字符（通过URL编码处理）
     */
    bool downloadFile(const QString &remotePath, const QString &localPath);
    
    /**
     * @brief 下载目录
     * @param remotePath 远程目录路径
     * @param localPath 本地保存路径
     * @return 下载是否成功
     * 
     * 递归下载整个目录及其子目录、文件
     */
    bool downloadDirectory(const QString &remotePath, const QString &localPath);
    
    /**
     * @brief 列出目录内容用于下载
     * @param path 要列出的目录路径
     * @param targetDir 目标本地目录
     * @return 操作是否成功
     * 
     * 获取指定FTP目录的内容，并为每个子项添加下载任务
     * 与listDirectory不同，此函数不更新UI，只用于下载流程
     */
    bool listDirectoryForDownload(const QString &path, const QString &targetDir);
    
    /**
     * @brief 创建本地目录结构
     * @param localPath 本地目录路径
     * @return 创建是否成功
     * 
     * 创建本地目录，用于保存下载的文件
     * 递归创建多级目录（如path/to/dir）
     */
    bool createLocalDirectory(const QString &localPath);

    // ===== 类成员变量 =====
    Ui::MainWindow *ui;                    ///< UI界面指针，由Qt Designer自动生成
    CURL *curl;                           ///< CURL句柄，用于FTP通信
    struct curl_slist *headers;           ///< CURL头部列表，用于设置FTP命令
    QStandardItemModel *fileModel;        ///< 文件列表模型，用于显示FTP目录内容
    QString currentPath;                  ///< 当前目录路径，记录当前浏览的FTP目录
    QStringList listBuffer;               ///< 目录列表缓冲区，存储从FTP服务器获取的目录数据
    bool isConnected;                     ///< 连接状态标志，表示是否已连接到FTP服务器
    QStack<QString> directoryHistory;     ///< 目录浏览历史，用于实现返回上级目录功能
    
    QQueue<DownloadTask> downloadQueue;   ///< 下载任务队列，存储待下载的任务
    QMutex downloadMutex;                ///< 下载队列互斥锁，保护下载队列的线程安全
    bool isDownloading;                  ///< 是否正在下载标志，防止重复启动下载
    QFile *currentDownloadFile;          ///< 当前下载文件，指向正在下载的文件
    qint64 totalBytesReceived;           ///< 已接收总字节数，用于计算下载进度
    QProgressDialog *progressDialog;     ///< 进度对话框，显示下载进度
    QTimer *downloadTimer;               ///< 下载定时器，用于处理下载队列
    int directoryTaskCount;              ///< 目录任务计数，用于跟踪递归下载目录的进度
};
#endif // MAINWINDOW_H
