#ifndef CAFFEINE_PLUS_H
#define CAFFEINE_PLUS_H

#include <QObject>

using InhibitionInfo = QPair<QString, QString>;

class CaffeinePlus : public QObject
{
    Q_OBJECT

public:
	CaffeinePlus(QObject *parent = 0);
    ~CaffeinePlus();

public Q_SLOTS:
	void init();
	QString checkName();


    uint AddInhibition(uint in0, const QString &in1, const QString &in2);
    bool HasInhibition(uint in0);
    QList<InhibitionInfo> ListInhibitions();
    void ReleaseInhibition(uint in0);
Q_SIGNALS: // SIGNALS
    void InhibitionsChanged(const QList<InhibitionInfo> &in0, const QStringList &in1);

private:
	QStringList _apps;
	QStringList _cookies;
	QStringList _inhibitors;
	QStringList _windows;
};

#endif // CAFFEINE_PLUS_H
