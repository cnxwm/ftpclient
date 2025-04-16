/**
 * @file mainwindow.h
 * @brief FTP客户端主窗口头文件
 * @details 使用libcurl实现FTP客户端功能
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUrl>
#include <QFile>
#include <curl/curl.h>
#include <QStandardItemModel>
#include <QStack>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * @class MainWindow
 * @brief FTP客户端主窗口类
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~MainWindow();

private slots:
    /**
     * @brief 连接按钮点击事件处理
     */
    void onConnectButtonClicked();
    
    /**
     * @brief 断开连接按钮点击事件处理
     */
    void onDisconnectButtonClicked();
    
    /**
     * @brief 文件树视图双击事件处理
     * @param index 被点击的项目索引
     */
    void onFileTreeViewDoubleClicked(const QModelIndex &index);
    
    /**
     * @brief 返回上级目录按钮点击事件处理
     */
    void onBackButtonClicked();
    
    /**
     * @brief 刷新目录按钮点击事件处理
     */
    void onRefreshButtonClicked();

private:
    /**
     * @brief 连接FTP服务器
     * @return 连接是否成功
     */
    bool connectToFtp();
    
    /**
     * @brief 断开FTP连接
     */
    void disconnectFromFtp();
    
    /**
     * @brief 列出目录内容
     * @param path 要列出的目录路径
     * @return 操作是否成功
     */
    bool listDirectory(const QString &path = "/");
    
    /**
     * @brief 解析FTP目录列表
     * @param listData 目录列表数据
     */
    void parseFtpList(const QString &listData);
    
    /**
     * @brief 更新按钮状态
     * @param connected 是否已连接
     */
    void updateButtonStates(bool connected);
    
    /**
     * @brief 添加日志信息
     * @param message 日志消息
     */
    void appendLog(const QString &message);
    
    /**
     * @brief 更新当前路径显示
     */
    void updatePathDisplay();
    
    /**
     * @brief 获取上级目录路径
     * @param path 当前路径
     * @return 上级目录路径
     */
    QString getParentDirectory(const QString &path);
    
    /**
     * @brief CURL写入回调函数
     * @param contents 接收到的数据
     * @param size 数据块大小
     * @param nmemb 数据块数量
     * @param userp 用户数据指针
     * @return 实际写入的数据大小
     */
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

    Ui::MainWindow *ui;                    ///< UI界面指针
    CURL *curl;                           ///< CURL句柄
    struct curl_slist *headers;           ///< CURL头部列表
    QStandardItemModel *fileModel;        ///< 文件列表模型
    QString currentPath;                  ///< 当前目录路径
    QStringList listBuffer;               ///< 目录列表缓冲区
    bool isConnected;                     ///< 连接状态标志
    QStack<QString> directoryHistory;     ///< 目录浏览历史
};
#endif // MAINWINDOW_H
