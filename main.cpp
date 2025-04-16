/**
 * @file main.cpp
 * @brief 程序入口点
 * 
 * 这是FTP客户端程序的主入口点。
 * 创建QApplication实例并启动主窗口。
 */

#include "mainwindow.h"

#include <QApplication>

/**
 * @brief 主函数
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出代码
 * 
 * 初始化Qt应用程序，创建并显示主窗口，
 * 然后进入Qt事件循环。
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);  // 创建Qt应用程序实例
    MainWindow w;                // 创建主窗口实例
    w.show();                    // 显示主窗口
    return a.exec();             // 进入Qt事件循环，直到应用程序退出
}
