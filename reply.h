#ifndef SERVERREPLY_H
#define SERVERREPLY_H

#include <QtCore>

class Reply
{
public:
    Reply(bool state, QString errStr, QByteArray data);

    bool isOK();
    QString getErrorString();
    QByteArray reply();

private:
    bool status;
    QString errorString;
    QByteArray replyData;
};

#endif // SERVERREPLY_H
