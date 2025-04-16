/****************************************************************************
** Meta object code from reading C++ file 'ftpclient.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../ftpclient.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ftpclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN9FtpClientE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN9FtpClientE = QtMocHelpers::stringData(
    "FtpClient",
    "connectionStateChanged",
    "",
    "connected",
    "directoryListingFinished",
    "entries",
    "downloadProgress",
    "bytesReceived",
    "bytesTotal",
    "uploadProgress",
    "bytesSent",
    "operationFinished",
    "success",
    "message",
    "logMessage",
    "onAuthenticationRequired",
    "QNetworkReply*",
    "reply",
    "QAuthenticator*",
    "authenticator",
    "onSslErrors",
    "QList<QSslError>",
    "errors",
    "onDownloadFinished",
    "onUploadFinished",
    "onDirectoryListingFinished",
    "onDownloadProgressUpdate",
    "onUploadProgressUpdate"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN9FtpClientE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   92,    2, 0x06,    1 /* Public */,
       4,    1,   95,    2, 0x06,    3 /* Public */,
       6,    2,   98,    2, 0x06,    5 /* Public */,
       9,    2,  103,    2, 0x06,    8 /* Public */,
      11,    2,  108,    2, 0x06,   11 /* Public */,
      14,    1,  113,    2, 0x06,   14 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      15,    2,  116,    2, 0x08,   16 /* Private */,
      20,    2,  121,    2, 0x08,   19 /* Private */,
      23,    0,  126,    2, 0x08,   22 /* Private */,
      24,    0,  127,    2, 0x08,   23 /* Private */,
      25,    0,  128,    2, 0x08,   24 /* Private */,
      26,    2,  129,    2, 0x08,   25 /* Private */,
      27,    2,  134,    2, 0x08,   28 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void, QMetaType::QStringList,    5,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong,    7,    8,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong,   10,    8,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   12,   13,
    QMetaType::Void, QMetaType::QString,   13,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 16, 0x80000000 | 18,   17,   19,
    QMetaType::Void, 0x80000000 | 16, 0x80000000 | 21,   17,   22,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong,    7,    8,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong,   10,    8,

       0        // eod
};

Q_CONSTINIT const QMetaObject FtpClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN9FtpClientE.offsetsAndSizes,
    qt_meta_data_ZN9FtpClientE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN9FtpClientE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<FtpClient, std::true_type>,
        // method 'connectionStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'directoryListingFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QStringList &, std::false_type>,
        // method 'downloadProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'uploadProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'operationFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'logMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onAuthenticationRequired'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>,
        QtPrivate::TypeAndForceComplete<QAuthenticator *, std::false_type>,
        // method 'onSslErrors'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QList<QSslError> &, std::false_type>,
        // method 'onDownloadFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onUploadFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDirectoryListingFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDownloadProgressUpdate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'onUploadProgressUpdate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>
    >,
    nullptr
} };

void FtpClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<FtpClient *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->connectionStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->directoryListingFinished((*reinterpret_cast< std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 2: _t->downloadProgress((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 3: _t->uploadProgress((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 4: _t->operationFinished((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 5: _t->logMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->onAuthenticationRequired((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QAuthenticator*>>(_a[2]))); break;
        case 7: _t->onSslErrors((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<QSslError>>>(_a[2]))); break;
        case 8: _t->onDownloadFinished(); break;
        case 9: _t->onUploadFinished(); break;
        case 10: _t->onDirectoryListingFinished(); break;
        case 11: _t->onDownloadProgressUpdate((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 12: _t->onUploadProgressUpdate((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 6:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 7:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<QSslError> >(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (FtpClient::*)(bool );
            if (_q_method_type _q_method = &FtpClient::connectionStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (FtpClient::*)(const QStringList & );
            if (_q_method_type _q_method = &FtpClient::directoryListingFinished; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (FtpClient::*)(qint64 , qint64 );
            if (_q_method_type _q_method = &FtpClient::downloadProgress; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (FtpClient::*)(qint64 , qint64 );
            if (_q_method_type _q_method = &FtpClient::uploadProgress; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (FtpClient::*)(bool , const QString & );
            if (_q_method_type _q_method = &FtpClient::operationFinished; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (FtpClient::*)(const QString & );
            if (_q_method_type _q_method = &FtpClient::logMessage; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject *FtpClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FtpClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN9FtpClientE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int FtpClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void FtpClient::connectionStateChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void FtpClient::directoryListingFinished(const QStringList & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void FtpClient::downloadProgress(qint64 _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void FtpClient::uploadProgress(qint64 _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void FtpClient::operationFinished(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void FtpClient::logMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
