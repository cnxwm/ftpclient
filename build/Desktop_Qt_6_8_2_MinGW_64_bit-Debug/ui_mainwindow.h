/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *serverEdit;
    QLabel *label_4;
    QSpinBox *portSpinBox;
    QLabel *label_2;
    QLineEdit *usernameEdit;
    QLabel *label_3;
    QLineEdit *passwordEdit;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    QHBoxLayout *horizontalLayout_2;
    QTreeView *fileTreeView;
    QVBoxLayout *verticalLayout_2;
    QPushButton *uploadButton;
    QPushButton *downloadButton;
    QSpacerItem *verticalSpacer;
    QPlainTextEdit *logTextEdit;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(centralwidget);
        label->setObjectName("label");

        horizontalLayout->addWidget(label);

        serverEdit = new QLineEdit(centralwidget);
        serverEdit->setObjectName("serverEdit");

        horizontalLayout->addWidget(serverEdit);

        label_4 = new QLabel(centralwidget);
        label_4->setObjectName("label_4");

        horizontalLayout->addWidget(label_4);

        portSpinBox = new QSpinBox(centralwidget);
        portSpinBox->setObjectName("portSpinBox");
        portSpinBox->setMinimum(1);
        portSpinBox->setMaximum(65535);
        portSpinBox->setValue(21);

        horizontalLayout->addWidget(portSpinBox);

        label_2 = new QLabel(centralwidget);
        label_2->setObjectName("label_2");

        horizontalLayout->addWidget(label_2);

        usernameEdit = new QLineEdit(centralwidget);
        usernameEdit->setObjectName("usernameEdit");

        horizontalLayout->addWidget(usernameEdit);

        label_3 = new QLabel(centralwidget);
        label_3->setObjectName("label_3");

        horizontalLayout->addWidget(label_3);

        passwordEdit = new QLineEdit(centralwidget);
        passwordEdit->setObjectName("passwordEdit");
        passwordEdit->setEchoMode(QLineEdit::Password);

        horizontalLayout->addWidget(passwordEdit);

        connectButton = new QPushButton(centralwidget);
        connectButton->setObjectName("connectButton");

        horizontalLayout->addWidget(connectButton);

        disconnectButton = new QPushButton(centralwidget);
        disconnectButton->setObjectName("disconnectButton");

        horizontalLayout->addWidget(disconnectButton);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        fileTreeView = new QTreeView(centralwidget);
        fileTreeView->setObjectName("fileTreeView");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(fileTreeView->sizePolicy().hasHeightForWidth());
        fileTreeView->setSizePolicy(sizePolicy);
        fileTreeView->setMinimumSize(QSize(0, 300));

        horizontalLayout_2->addWidget(fileTreeView);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        uploadButton = new QPushButton(centralwidget);
        uploadButton->setObjectName("uploadButton");

        verticalLayout_2->addWidget(uploadButton);

        downloadButton = new QPushButton(centralwidget);
        downloadButton->setObjectName("downloadButton");

        verticalLayout_2->addWidget(downloadButton);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);


        horizontalLayout_2->addLayout(verticalLayout_2);


        verticalLayout->addLayout(horizontalLayout_2);

        logTextEdit = new QPlainTextEdit(centralwidget);
        logTextEdit->setObjectName("logTextEdit");
        logTextEdit->setReadOnly(true);

        verticalLayout->addWidget(logTextEdit);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 21));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Simple FTP Client", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "FTP Server:", nullptr));
        serverEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "ftp://example.com", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "Port:", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Username:", nullptr));
        usernameEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "Enter username", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Password:", nullptr));
        passwordEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "Enter password", nullptr));
        connectButton->setText(QCoreApplication::translate("MainWindow", "Connect", nullptr));
        disconnectButton->setText(QCoreApplication::translate("MainWindow", "Disconnect", nullptr));
        uploadButton->setText(QCoreApplication::translate("MainWindow", "Upload", nullptr));
        downloadButton->setText(QCoreApplication::translate("MainWindow", "Download", nullptr));
        logTextEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "Log messages will appear here...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
