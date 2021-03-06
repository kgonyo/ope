#ifndef CM_HTTPSERVER_H
#define CM_HTTPSERVER_H

#include <QObject>
#include <QRegExp>
#include <QStringList>
#include <QDateTime>
#include <QList>
#include <QDebug>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>

// Socket Includes
#include <QTcpServer>
#include <QTcpSocket>
#include <QSslSocket>
#include <QNetworkProxy>

#include <QSsl>
#include <QSslKey>
#include <QSslCipher>
#include <QSslCertificate>

#include "cm_mimetypes.h"


// Early Declare
class CM_HTTPRequest;
class CM_HTTPResponse;

class CM_HTTPServer : public QTcpServer
{
    Q_OBJECT
public:
    quint64 static totalBytesRead;
    quint64 static totalBytesWritten;

    explicit CM_HTTPServer(QObject *parent = 0);
    
    // Start a new HTTP Server
    bool Start(qint16 port = 80, bool use_ssl = false);

    bool registerPathHandler(QString path, qintptr handler);
    bool registerExtentionHandler(QString extention, qintptr handler);
    bool registerHandler(qintptr handler);

    // New connection Request on the server socket
    //void incomingConnection(int socket);
    void incomingConnection(qintptr socket);

#ifndef CM_DISABLE_SSL
    // Set specific SSL cert
    void setCACert(char pem_data[]);
    void setSSLCert(char pem_data[]);
    void setSSLKey(char pem_data[]);
    void setSSLCert(char ca_data[], char cert_data[], char key_data[]);
#endif

signals:
    void HTTPRequestArrived(CM_HTTPRequest *request, CM_HTTPResponse *response);

public slots:

    // Client socket signals
    void slot_clientAboutToClose();
    void slot_clientBytesWritten(qint64);
    void slot_clientConnected();
    void slot_clientDestroyed();
    void slot_clientDestroyed(QObject*);
    void slot_clientDisconnected();
    void slot_clientHostFound();
    void slot_clientObjectNameChanged(QString, QPrivateSignal);
    void slot_clientProxyAuthenticationRequest(QNetworkProxy, QAuthenticator *);
    void slot_clientReadChannelFinished();
    void slot_clientReadyRead();
    void slot_clientStateChanged(QAbstractSocket::SocketState);
    void slot_clientError(QAbstractSocket::SocketError);

#ifndef CM_DISABLE_SSL
    void slot_clientEncrypted();
    void slot_clientEncryptedBytesWritten(qint64);
    void slot_clientModeChanged(QSslSocket::SslMode);
    void slot_clientPeerVerifyError(QSslError);
    void slot_clientSslErrors(QList<QSslError>);

#endif

private:

    quint64 bytesRead;
    quint64 bytesWritten;

    quint64 encryptedBytesRead;
    quint64 encryptedBytesWritten;



    bool disabled;
    qint16 http_port;
    bool ssl_on;

    QHash<QString, qintptr> path_handlers;
    QHash<QString, qintptr> extention_handlers;
    QList<qintptr> handlers;

#ifndef CM_DISABLE_SSL
    QList<QSslCertificate> ca_certs;
    QSslCertificate ca_cert;
    QSslCertificate server_cert;
    QSslKey server_key;
#endif
    
};


/// Class that contains the request information for the current
/// web connection
class CM_HTTPRequest : public QObject
{
    Q_OBJECT
public:

    QHash<QString, QString> headers;
    QString body;

    explicit CM_HTTPRequest(QObject *parent);

    QString GetProtocol() { return headers["Protocol"]; }

    void ParsePacket(QString packet);

    QString toString();
signals:
public slots:
private:
};


class CM_HTTPResponse : public QObject
{
    Q_OBJECT

public:

    explicit CM_HTTPResponse(QObject *parent);

    QHash<QString, QString> response_result;
    QHash<QString, QString> headers;
    QByteArray body;

signals:
public slots:
    void SetProtocol(QString protocol) { response_result["Protocol"] = protocol; }

    void SetBody(QString html)
    {
        QByteArray arr = html.toUtf8().constData();
        body = arr;
    }

    void AddHeader(QString header_name, QString header_value)
    {
        headers[header_name] = header_value;
    }

    void RespondWith404Error(QString file_path)
    {
        response_result["Protocol"] = "HTTP/1.0";
        response_result["CODE"] = "404";
        response_result["Message"] = "Not Found";
        SetBody("<B>404 File Not Found!</B> - " + file_path);
    }

    void RespondWithFile(QString file_path)
    {
        // Open the file and attach it as the body
        QFile f(file_path);
        f.open(QFile::ReadOnly);


        if (f.isOpen())
        {
            QString content_type = GetContentType(file_path);
            body = f.readAll();
            /*if (!content_type.contains("text"))
            {
                //body = f.readAll().toBase64();

            } else {
                QTextStream s(&f);
                body = s.readAll();
            }*/

            // Set the content type
            AddHeader("Content-Type", content_type);

            //response_result["Protocol"] = "HTTP/1.0";
            response_result["CODE"] = "200";
            response_result["Message"] = "Ok";

            //qDebug() << "Reply with file: " << file_path;


        } else {
            // Couldn't find the file
            qDebug() << "Couldn't open file: " << file_path;
            RespondWith404Error(file_path);
        }
    }



    QString GetContentType(QString file_name)
    {
        QString ext;
        QString ret = "text/html";
        int pindex = file_name.lastIndexOf(".");
        if (pindex == -1) { return ret; }
        ext = file_name.mid(pindex+1).toLower();

        ret = CM_MimeTypes::GetMimeType(ext);

        return ret;
    }

    QString toString();

    QByteArray GetResponse();

private:
};


#endif // CM_HTTPSERVER_H
